#ifndef OS_H
#define OS_H

#include <stdint.h>

/*
 * OS 抽象层。
 *
 * 对 EcuM 屏蔽 FreeRTOS 与裸机 super loop 的差异。
 * APP_CFG_USE_FREERTOS=1 时创建 FreeRTOS 任务；为 0 时进入 while(1) 轮询调度。
 */
void Os_Start(void);

/*
 * 毫秒延时。
 *
 * FreeRTOS 调度器启动后使用 vTaskDelay 让出 CPU；启动前或裸机模式下退化为忙等。
 */
void Os_DelayMs(uint32_t ms);

/* 返回统一毫秒时间基准，FreeRTOS 模式来自 tick，裸机模式来自 EcuM 软件计数。 */
uint32_t Os_GetTickMs(void);

#endif /* OS_H */
