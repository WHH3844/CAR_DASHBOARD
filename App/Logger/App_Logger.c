#include "App_Logger.h"

#include "LogM.h"

/* 下一次 heartbeat 输出时间，避免每个主循环周期都刷日志。 */
static uint32_t App_LoggerNextHeartbeatMs;

void App_Logger_Init(void)
{
    App_LoggerNextHeartbeatMs = 1000u;
}

void App_Logger_MainFunction(uint32_t tick_ms)
{
    if (tick_ms < App_LoggerNextHeartbeatMs)
    {
        return;
    }

    /*
     * heartbeat 是最轻量的“系统仍在运行”信号。
     * 如果现场只能看串口日志，1s 一条能快速判断主循环是否卡死。
     */
    App_LoggerNextHeartbeatMs = tick_ms + 1000u;
    LogM_Info("runtime heartbeat");
}
