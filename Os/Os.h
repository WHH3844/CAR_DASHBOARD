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

/* OS 抽象层初始化：创建共享资源 mutex，必须在 BSW/APP 初始化前调用。 */
void Os_Init(void);

/*
 * 毫秒延时。
 *
 * FreeRTOS 调度器启动后使用 vTaskDelay 让出 CPU；启动前或裸机模式下退化为忙等。
 */
void Os_DelayMs(uint32_t ms);

/* 返回统一毫秒时间基准，FreeRTOS 模式来自 tick，裸机模式来自 EcuM 软件计数。 */
uint32_t Os_GetTickMs(void);

/*
 * RTE 这类短临界区使用关中断/调度临界区保护。
 * 临界区内只做内存拷贝，不允许放 I2C、LCD、EEPROM 等耗时操作。
 */
void Os_EnterCritical(void);
void Os_ExitCritical(void);

/*
 * I2C0 总线锁。
 * RTC、SHT30、EEPROM 等设备共用 I2C0，拆成多个任务后必须串行访问。
 */
uint8_t Os_I2c0Lock(void);
void Os_I2c0Unlock(void);

/*
 * NvM/Dem 周期维护和关机保存共用同一份 RAM 状态，关机写入前需要先阻止
 * NvMTask 继续执行，避免 EEPROM 写入和状态机更新交叠。
 */
uint8_t Os_NvMLock(void);
void Os_NvMUnlock(void);

#endif /* OS_H */
