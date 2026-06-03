#ifndef APP_POWER_H
#define APP_POWER_H

#include <stdint.h>

/*
 * 电源按键运行期管理。
 *
 * 上电自锁由 EcuM/PowerIf 在启动阶段完成；本模块只负责 RUN 状态下的长按关机请求。
 * 为了避免“开机时用户还没松手”被误判为关机，必须先检测到一次释放，
 * 后续再次长按才会置位 shutdown request。
 */
void App_Power_Init(void);
void App_Power_MainFunction(uint32_t tick_ms);

/*
 * 查询/清除关机请求。
 *
 * EcuM_MainFunction 轮询到请求后会保存 NvM/Dem，然后调用 PowerIf_Shutdown() 断电。
 */
uint8_t App_Power_IsShutdownRequested(void);
void App_Power_ClearShutdownRequest(void);

#endif /* APP_POWER_H */
