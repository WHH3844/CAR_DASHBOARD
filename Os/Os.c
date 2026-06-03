#include "Os.h"

#include "gd32f4xx.h"

void Os_DelayMs(uint32_t ms)
{
    uint32_t index;

    while (ms-- != 0u)
    {
        for (index = 0u; index < 20000u; index++)
        {
            __NOP();
        }
    }
}
#include "Os.h"

#include "App_Cfg.h"
#include "EcuM.h"

#if APP_CFG_USE_FREERTOS != 0u
#include "FreeRTOS.h"
#include "task.h"

static void Os_TaskSystem(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();
    while (1)
    {
        EcuM_RunSystem10ms();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10u));
    }
}

static void Os_TaskCan(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();
    while (1)
    {
        EcuM_RunCan10ms();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10u));
    }
}

static void Os_TaskApp(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();
    while (1)
    {
        EcuM_RunApp10ms();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10u));
    }
}

static void Os_TaskDisplay(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();
    while (1)
    {
        EcuM_RunDisplay500ms();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(500u));
    }
}

static void Os_TaskDiagNvM(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();
    while (1)
    {
        EcuM_RunDiagNvM100ms();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100u));
    }
}
#endif

void Os_Start(void)
{
#if APP_CFG_USE_FREERTOS != 0u
    (void)xTaskCreate(Os_TaskSystem,
                      "System",
                      APP_CFG_FREERTOS_SYSTEM_STACK,
                      0,
                      APP_CFG_FREERTOS_PRIO_SYSTEM,
                      0);
    (void)xTaskCreate(Os_TaskCan,
                      "CAN",
                      APP_CFG_FREERTOS_CAN_STACK,
                      0,
                      APP_CFG_FREERTOS_PRIO_CAN,
                      0);
    (void)xTaskCreate(Os_TaskApp,
                      "APP",
                      APP_CFG_FREERTOS_APP_STACK,
                      0,
                      APP_CFG_FREERTOS_PRIO_APP,
                      0);
    (void)xTaskCreate(Os_TaskDisplay,
                      "Display",
                      APP_CFG_FREERTOS_DISPLAY_STACK,
                      0,
                      APP_CFG_FREERTOS_PRIO_DISPLAY,
                      0);
    (void)xTaskCreate(Os_TaskDiagNvM,
                      "DiagNvM",
                      APP_CFG_FREERTOS_DIAG_NVM_STACK,
                      0,
                      APP_CFG_FREERTOS_PRIO_DIAG_NVM,
                      0);
    vTaskStartScheduler();

    while (1)
    {
    }
#else
    while (1)
    {
        EcuM_MainFunction();
    }
#endif
}
