#include "Os.h"

#include "App_Cfg.h"
#include "EcuM.h"
#include "LogM.h"

#include "gd32f4xx.h"

#if APP_CFG_USE_FREERTOS != 0u
#include "FreeRTOS.h"
#include "task.h"
#endif

static void Os_BusyDelayMs(uint32_t ms)
{
    uint32_t index;

    /*
     * 忙等延时只用于调度器未启动或裸机模式。
     * 这里的循环系数来自板级粗略校准，不适合作为精密计时源。
     */
    while (ms-- != 0u)
    {
        for (index = 0u; index < 20000u; index++)
        {
            __NOP();
        }
    }
}

void Os_DelayMs(uint32_t ms)
{
#if APP_CFG_USE_FREERTOS != 0u
    /*
     * 调度器启动后，普通等待应该让出 CPU；
     * 调度器启动前，例如电源按键去抖，只能使用忙等。
     */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        vTaskDelay(pdMS_TO_TICKS(ms));
        return;
    }
#endif

    Os_BusyDelayMs(ms);
}

uint32_t Os_GetTickMs(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
    return EcuM_GetTickMs();
#endif
}

#if APP_CFG_USE_FREERTOS != 0u
static void Os_EcuMMainTask(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        /*
         * 第一版 FreeRTOS 移植先用单个 EcuM 主任务承载原 10ms 调度。
         * 这样不会引入 RTE/LCD/I2C/EEPROM 的并发访问风险；
         * 等板上确认稳定后，再把 CAN、显示、传感器、NvM 拆成独立任务。
         */
        EcuM_MainFunction();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CFG_MAIN_LOOP_MS));
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    while (1)
    {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;

    taskDISABLE_INTERRUPTS();
    while (1)
    {
    }
}

void vApplicationIdleHook(void)
{
}
#endif

void Os_Start(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    LogM_Info("Os creating FreeRTOS EcuM task");

    (void)xTaskCreate(Os_EcuMMainTask,
                      "EcuM",
                      APP_CFG_FREERTOS_ECUM_STACK,
                      0,
                      APP_CFG_FREERTOS_PRIO_ECUM,
                      0);

    /*
     * 第一版没有保存任务句柄，因为暂不做运行期挂起/删除。
     * 如果 xTaskCreate 失败，vTaskStartScheduler() 通常会返回并进入下面死循环。
     */
    vTaskStartScheduler();

    /*
     * 如果堆太小或任务创建失败，调度器会返回。
     * 正常情况下不会走到这里。
     */
    while (1)
    {
    }
#else
    while (1)
    {
        /*
         * 裸机模式保持和 FreeRTOS 任务相同的 10ms 调度节拍：
         * 先跑 EcuM，再延时，再推进软件 tick。
         */
        EcuM_MainFunction();
        Os_DelayMs(APP_CFG_MAIN_LOOP_MS);
        EcuM_AdvanceTick(APP_CFG_MAIN_LOOP_MS);
    }
#endif
}
