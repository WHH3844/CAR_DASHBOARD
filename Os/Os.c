#include "Os.h"

#include "App_Cfg.h"
#include "EcuM.h"
#include "LogM.h"

#include "gd32f4xx.h"

#if APP_CFG_USE_FREERTOS != 0u
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif

#if APP_CFG_USE_FREERTOS != 0u
static SemaphoreHandle_t Os_I2c0Mutex;
static SemaphoreHandle_t Os_NvMMutex;
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

void Os_Init(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    if (Os_I2c0Mutex == 0)
    {
        Os_I2c0Mutex = xSemaphoreCreateMutex();
    }

    if (Os_NvMMutex == 0)
    {
        Os_NvMMutex = xSemaphoreCreateMutex();
    }
#endif
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

void Os_EnterCritical(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        taskENTER_CRITICAL();
        return;
    }
#endif

    __disable_irq();
}

void Os_ExitCritical(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        taskEXIT_CRITICAL();
        return;
    }
#endif

    __enable_irq();
}

#if APP_CFG_USE_FREERTOS != 0u
static uint8_t Os_TakeMutex(SemaphoreHandle_t mutex)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        return 1u;
    }

    if (mutex == 0)
    {
        return 0u;
    }

    return (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) ? 1u : 0u;
}

static void Os_GiveMutex(SemaphoreHandle_t mutex)
{
    if ((mutex != 0) && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
    {
        (void)xSemaphoreGive(mutex);
    }
}
#endif

uint8_t Os_I2c0Lock(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    return Os_TakeMutex(Os_I2c0Mutex);
#else
    return 1u;
#endif
}

void Os_I2c0Unlock(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    Os_GiveMutex(Os_I2c0Mutex);
#endif
}

uint8_t Os_NvMLock(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    return Os_TakeMutex(Os_NvMMutex);
#else
    return 1u;
#endif
}

void Os_NvMUnlock(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    Os_GiveMutex(Os_NvMMutex);
#endif
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
#if APP_CFG_FREERTOS_SPLIT_TASKS != 0u
static void Os_CanTask(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        EcuM_ComMainFunction();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CFG_MAIN_LOOP_MS));
    }
}

static void Os_AppFastTask(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        EcuM_AppFastMainFunction();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CFG_DASHBOARD_PERIOD_MS));
    }
}

static void Os_DisplayTask(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        EcuM_DisplayMainFunction();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CFG_DISPLAY_PERIOD_MS));
    }
}

static void Os_SensorTask(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        EcuM_SensorMainFunction();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CFG_SENSOR_PERIOD_MS));
    }
}

static void Os_NvMTask(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        EcuM_NvMMainFunction();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CFG_NVM_PERIOD_MS));
    }
}

static void Os_LoggerTask(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        EcuM_LoggerMainFunction();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CFG_LOGGER_PERIOD_MS));
    }
}

static void Os_EcuMManagerTask(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        EcuM_LifecycleMainFunction();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CFG_MAIN_LOOP_MS));
    }
}
#else
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
#endif

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

static void Os_CheckTaskCreate(BaseType_t result, const char *task_name)
{
    if (result == pdPASS)
    {
        return;
    }

    LogM_Error("FreeRTOS task create failed");
    LogM_Error(task_name);
    taskDISABLE_INTERRUPTS();
    while (1)
    {
    }
}
#endif

void Os_Start(void)
{
#if APP_CFG_USE_FREERTOS != 0u
#if APP_CFG_FREERTOS_SPLIT_TASKS != 0u
    LogM_Info("Os creating split FreeRTOS tasks");

    Os_CheckTaskCreate(xTaskCreate(Os_CanTask,
                                   "Can",
                                   APP_CFG_FREERTOS_CAN_STACK,
                                   0,
                                   APP_CFG_FREERTOS_PRIO_CAN,
                                   0),
                       "Can");

    Os_CheckTaskCreate(xTaskCreate(Os_AppFastTask,
                                   "AppFast",
                                   APP_CFG_FREERTOS_APP_STACK,
                                   0,
                                   APP_CFG_FREERTOS_PRIO_APP,
                                   0),
                       "AppFast");

    Os_CheckTaskCreate(xTaskCreate(Os_DisplayTask,
                                   "Display",
                                   APP_CFG_FREERTOS_DISPLAY_STACK,
                                   0,
                                   APP_CFG_FREERTOS_PRIO_DISPLAY,
                                   0),
                       "Display");

    Os_CheckTaskCreate(xTaskCreate(Os_SensorTask,
                                   "Sensor",
                                   APP_CFG_FREERTOS_SENSOR_STACK,
                                   0,
                                   APP_CFG_FREERTOS_PRIO_SENSOR,
                                   0),
                       "Sensor");

    Os_CheckTaskCreate(xTaskCreate(Os_NvMTask,
                                   "NvM",
                                   APP_CFG_FREERTOS_NVM_STACK,
                                   0,
                                   APP_CFG_FREERTOS_PRIO_NVM,
                                   0),
                       "NvM");

    Os_CheckTaskCreate(xTaskCreate(Os_LoggerTask,
                                   "Logger",
                                   APP_CFG_FREERTOS_LOGGER_STACK,
                                   0,
                                   APP_CFG_FREERTOS_PRIO_LOGGER,
                                   0),
                       "Logger");

    Os_CheckTaskCreate(xTaskCreate(Os_EcuMManagerTask,
                                   "EcuM",
                                   APP_CFG_FREERTOS_ECUM_STACK,
                                   0,
                                   APP_CFG_FREERTOS_PRIO_ECUM,
                                   0),
                       "EcuM");
#else
    LogM_Info("Os creating FreeRTOS EcuM task");

    Os_CheckTaskCreate(xTaskCreate(Os_EcuMMainTask,
                                   "EcuM",
                                   APP_CFG_FREERTOS_ECUM_STACK,
                                   0,
                                   APP_CFG_FREERTOS_PRIO_ECUM,
                                   0),
                       "EcuM");
#endif

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
