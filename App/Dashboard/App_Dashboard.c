#include "App_Dashboard.h"

#include "App_Cfg.h"
#include "LogM.h"
#include "Rte_Service.h"
#include "Rte_Signal.h"

/*
 * App_Dashboard 维护的是“仪表本地业务状态”，不是 CAN 原始数据。
 * 这些状态最终会写回 RTE，让 Display、Com 和诊断都看到同一份快照。
 */
static uint8_t App_Dashboard_SimMode;
static uint8_t App_Dashboard_SimDirUp;
static uint8_t App_Dashboard_BuzzerMuted;
static uint32_t App_Dashboard_NextSimMs;
static uint32_t App_Dashboard_BeepUntilMs;

void App_Dashboard_Init(void)
{
    const NvM_SystemConfigType *config;

    config = Rte_Call_NvM_GetSystemConfig();
    App_Dashboard_SimMode = 0u;
    App_Dashboard_SimDirUp = 1u;
    App_Dashboard_BuzzerMuted = (config->buzzer_enable == 0u) ? 1u : 0u;
    App_Dashboard_NextSimMs = 0u;
    App_Dashboard_BeepUntilMs = 0u;
    (void)Rte_Write_BuzzerMuted(App_Dashboard_BuzzerMuted);
    (void)Rte_Write_BacklightLevel(config->backlight_level);
}

static void App_Dashboard_HandleKey(uint32_t tick_ms)
{
    Rte_KeyEventType event;

    /*
     * Rte_Take_KeyEvent() 是“取走即清除”的消费语义。
     * 这样一次按键只会触发一次业务动作，不会在后续 10ms 周期里反复切换。
     */
    if (Rte_Take_KeyEvent(&event) != E_OK)
    {
        return;
    }

    if (event == RTE_KEY_EVENT_KEY1_SHORT)
    {
        App_Dashboard_SimMode = (App_Dashboard_SimMode == 0u) ? 1u : 0u;
        App_Dashboard_NextSimMs = tick_ms;
        (void)Rte_Write_SimulationMode(App_Dashboard_SimMode);
        App_Dashboard_BeepUntilMs = tick_ms + 80u;
        LogM_Info((App_Dashboard_SimMode != 0u) ? "KEY1 simulation on" : "KEY1 simulation off");
    }
    else if (event == RTE_KEY_EVENT_KEY2_SHORT)
    {
        App_Dashboard_BuzzerMuted = (App_Dashboard_BuzzerMuted == 0u) ? 1u : 0u;
        (void)Rte_Write_BuzzerMuted(App_Dashboard_BuzzerMuted);
        App_Dashboard_BeepUntilMs = tick_ms + 80u;
        LogM_Info((App_Dashboard_BuzzerMuted != 0u) ? "KEY2 buzzer muted" : "KEY2 buzzer enabled");
    }
    else if (event == RTE_KEY_EVENT_KEY3_SHORT)
    {
        /*
         * KEY3 清零只清动力域显示值，不清 RTC、温湿度、背光等其它状态。
         * 同时关闭模拟模式，避免下一次仿真刷新又立刻把车速/转速写回来。
         */
        Rte_Clear_DashboardValues();
        App_Dashboard_SimMode = 0u;
        (void)Rte_Write_SimulationMode(0u);
        App_Dashboard_BeepUntilMs = tick_ms + 80u;
        LogM_Info("KEY3 dashboard values cleared");
    }
}

static void App_Dashboard_UpdateSimulation(uint32_t tick_ms)
{
    Rte_DashboardDataType data;
    uint16_t speed;
    uint16_t rpm;

    if ((App_Dashboard_SimMode == 0u) || (tick_ms < App_Dashboard_NextSimMs))
    {
        return;
    }

    /*
     * 模拟数据按 100ms 刷新，速度在 0~160km/h 之间往返。
     * 速度单位沿用 RTE 的 0.1km/h，因此每次 +20 表示 +2.0km/h。
     */
    App_Dashboard_NextSimMs = tick_ms + 100u;
    (void)Rte_Read_DashboardData(&data);
    speed = data.vehicle_speed_kph_x10;

    if (App_Dashboard_SimDirUp != 0u)
    {
        if (speed < 1600u)
        {
            speed = (uint16_t)(speed + 20u);
        }
        else
        {
            App_Dashboard_SimDirUp = 0u;
        }
    }
    else
    {
        if (speed > 20u)
        {
            speed = (uint16_t)(speed - 20u);
        }
        else
        {
            App_Dashboard_SimDirUp = 1u;
        }
    }

    /*
     * 转速用速度派生，保留一个 800rpm 怠速基线。
     * 这里不是物理模型，只是为了让 UI、报警和 CAN 状态报文在无实车输入时可观察。
     */
    rpm = (uint16_t)(800u + ((uint32_t)speed * 3u));
    if (rpm > APP_CFG_RPM_MAX)
    {
        rpm = APP_CFG_RPM_MAX;
    }

    (void)Rte_Write_Powertrain(speed,
                               rpm,
                               data.fuel_percent,
                               data.coolant_temp_c,
                               data.outdoor_temp_c,
                               data.battery_mv,
                               tick_ms);
}

static void App_Dashboard_UpdateAlarm(uint32_t tick_ms)
{
    Rte_DashboardDataType data;
    uint8_t alarm;
    uint8_t buzzer_on;

    (void)Rte_Read_DashboardData(&data);
    alarm = ((data.vehicle_speed_kph_x10 >= APP_CFG_SPEED_ALARM_KPH_X10) ||
             (data.engine_rpm >= APP_CFG_RPM_ALARM)) ? 1u : 0u;
    (void)Rte_Write_AlarmActive(alarm);

    /*
     * 短促提示音优先级高于持续报警音：
     * 用户按键后即使蜂鸣器静音，也会给一个 80ms 的确认音，方便板上调试。
     */
    if (tick_ms < App_Dashboard_BeepUntilMs)
    {
        Rte_Call_Buzzer_Set(1u);
        return;
    }

    if ((alarm == 0u) || (App_Dashboard_BuzzerMuted != 0u))
    {
        Rte_Call_Buzzer_Set(0u);
        return;
    }

    /*
     * 报警蜂鸣采用 500ms 周期、80ms 占空的断续音。
     * 这种节拍比常开更容易听出“报警仍存在”，也不至于太吵。
     */
    buzzer_on = ((tick_ms % 500u) < 80u) ? 1u : 0u;
    Rte_Call_Buzzer_Set(buzzer_on);
}

void App_Dashboard_MainFunction(uint32_t tick_ms)
{
    App_Dashboard_HandleKey(tick_ms);
    App_Dashboard_UpdateSimulation(tick_ms);
    App_Dashboard_UpdateAlarm(tick_ms);
}
