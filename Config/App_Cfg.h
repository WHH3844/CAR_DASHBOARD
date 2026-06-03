#ifndef APP_CFG_H
#define APP_CFG_H

/*
 * 调度模式：
 * 1：默认启用 FreeRTOS，Os_Start() 创建 EcuM 主任务。
 * 0：回退到裸机 super loop，适合排查 RTOS tick 或任务栈问题。
 */
#define APP_CFG_USE_FREERTOS                1u
/*
 * EcuM 主调度周期。
 * 10ms 是应用层折中值：按键去抖、关机长按、CAN 超时都能及时响应，
 * LCD/传感器/NvM 等慢任务再由 EcuM 内部分频调度。
 */
#define APP_CFG_MAIN_LOOP_MS                10u

/*
 * 第一版 FreeRTOS 移植先用单任务承载原 10ms 主调度。
 * 栈单位是 FreeRTOS StackType_t，不是字节；ARMCC 下通常 1 个单位 = 4 字节。
 */
#define APP_CFG_FREERTOS_ECUM_STACK         1024u
#define APP_CFG_FREERTOS_PRIO_ECUM          3u

/*
 * 各业务任务的目标周期。
 * 当前 EcuM 每 10ms 调一次 App_Dashboard/App_Key/App_Power；
 * Display/NvM/Logger 等由 EcuM_NextXxxMs 做软件定时分频。
 */
#define APP_CFG_DASHBOARD_PERIOD_MS         10u
#define APP_CFG_DISPLAY_PERIOD_MS           500u
#define APP_CFG_SENSOR_PERIOD_MS            1000u
#define APP_CFG_KEY_PERIOD_MS               10u
#define APP_CFG_NVM_PERIOD_MS               100u

/*
 * 仪表报警阈值，单位和 RTE 内部信号一致。
 * 车速使用 0.1km/h，1200 表示 120.0km/h；转速单位 rpm。
 */
#define APP_CFG_SPEED_ALARM_KPH_X10         1200u
#define APP_CFG_RPM_ALARM                   5000u
#define APP_CFG_SPEED_MAX_KPH_X10           2200u
#define APP_CFG_RPM_MAX                     8000u

#endif /* APP_CFG_H */
