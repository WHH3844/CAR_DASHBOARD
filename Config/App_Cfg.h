#ifndef APP_CFG_H
#define APP_CFG_H

/*
 * 裸机 super loop 的应用周期。
 * 这些周期由 EcuM 调度，不依赖 RTOS。
 */
#define APP_CFG_MAIN_LOOP_MS                10u
#define APP_CFG_USE_FREERTOS                0u

/*
 * APP_CFG_USE_FREERTOS = 0：使用当前已验证的裸机 super loop。
 * APP_CFG_USE_FREERTOS = 1：Os_Start() 创建 FreeRTOS 任务。
 * 注意：打开前需要把 FreeRTOS 内核源码和 portable 文件加入 Keil 工程。
 */
#define APP_CFG_FREERTOS_SYSTEM_STACK       256u
#define APP_CFG_FREERTOS_CAN_STACK          256u
#define APP_CFG_FREERTOS_APP_STACK          384u
#define APP_CFG_FREERTOS_DISPLAY_STACK      512u
#define APP_CFG_FREERTOS_DIAG_NVM_STACK     384u

#define APP_CFG_FREERTOS_PRIO_SYSTEM        4u
#define APP_CFG_FREERTOS_PRIO_CAN           3u
#define APP_CFG_FREERTOS_PRIO_APP           2u
#define APP_CFG_FREERTOS_PRIO_DISPLAY       2u
#define APP_CFG_FREERTOS_PRIO_DIAG_NVM      1u

#define APP_CFG_DASHBOARD_PERIOD_MS         10u
#define APP_CFG_DISPLAY_PERIOD_MS           500u
#define APP_CFG_SENSOR_PERIOD_MS            1000u
#define APP_CFG_KEY_PERIOD_MS               10u
#define APP_CFG_NVM_PERIOD_MS               100u

/* 仪表报警阈值，单位和 RTE 内部信号一致。 */
#define APP_CFG_SPEED_ALARM_KPH_X10         1200u
#define APP_CFG_RPM_ALARM                   5000u
#define APP_CFG_SPEED_MAX_KPH_X10           2200u
#define APP_CFG_RPM_MAX                     8000u

#endif /* APP_CFG_H */
