#ifndef APP_LOGGER_H
#define APP_LOGGER_H

#include <stdint.h>

/*
 * 应用运行日志任务。
 *
 * 第一版只输出 heartbeat，用于确认主循环/FreeRTOS 任务仍在运行。
 * 后续可以在这里扩展周期状态摘要，例如 CAN 计数、DTC 数量或电源状态。
 */
void App_Logger_Init(void);
void App_Logger_MainFunction(uint32_t tick_ms);

#endif /* APP_LOGGER_H */
