#include "App_Dashboard.h"

#include "App_Cfg.h"
#include "LogM.h"
#include "Rte_Service.h"
#include "Rte_Signal.h"

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

    buzzer_on = ((tick_ms % 500u) < 80u) ? 1u : 0u;
    Rte_Call_Buzzer_Set(buzzer_on);
}

void App_Dashboard_MainFunction(uint32_t tick_ms)
{
    App_Dashboard_HandleKey(tick_ms);
    App_Dashboard_UpdateSimulation(tick_ms);
    App_Dashboard_UpdateAlarm(tick_ms);
}
#include "App_Dashboard.h"

#include "App_Cfg.h"
#include "BuzzerIf.h"
#include "Rte_Event.h"
#include "Rte_Signal.h"

static uint8_t App_DashboardSimMode;
static uint8_t App_DashboardSimUp;
static uint8_t App_DashboardMute;
static uint16_t App_DashboardSimTimerMs;

static void App_Dashboard_HandleKey(uint8_t key_event)
{
    if (key_event == RTE_KEY_EVENT_KEY1_SHORT)
    {
        App_DashboardSimMode = (App_DashboardSimMode == 0u) ? 1u : 0u;
    }
    else if (key_event == RTE_KEY_EVENT_KEY2_SHORT)
    {
        App_DashboardMute = (App_DashboardMute == 0u) ? 1u : 0u;
    }
    else if (key_event == RTE_KEY_EVENT_KEY3_SHORT)
    {
        App_DashboardSimMode = 0u;
        (void)Rte_Write_VehicleSpeed(0u);
        (void)Rte_Write_EngineRpm(0u);
    }
}

static void App_Dashboard_UpdateSimulation(uint16_t elapsed_ms)
{
    uint16_t speed;
    uint16_t rpm;

    if (App_DashboardSimMode == 0u)
    {
        return;
    }

    App_DashboardSimTimerMs = (uint16_t)(App_DashboardSimTimerMs + elapsed_ms);
    if (App_DashboardSimTimerMs < 100u)
    {
        return;
    }
    App_DashboardSimTimerMs = 0u;

    (void)Rte_Read_VehicleSpeed(&speed);
    if (App_DashboardSimUp != 0u)
    {
        if (speed < 1600u)
        {
            speed = (uint16_t)(speed + 20u);
        }
        else
        {
            App_DashboardSimUp = 0u;
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
            App_DashboardSimUp = 1u;
        }
    }

    rpm = (uint16_t)(800u + ((uint32_t)speed * 3u));
    if (rpm > APP_CFG_RPM_MAX)
    {
        rpm = APP_CFG_RPM_MAX;
    }

    (void)Rte_Write_VehicleSpeed(speed);
    (void)Rte_Write_EngineRpm(rpm);
    Rte_MarkCanEmsReceived();
}

static void App_Dashboard_UpdateAlarm(void)
{
    uint16_t speed;
    uint16_t rpm;
    uint8_t alarm;

    (void)Rte_Read_VehicleSpeed(&speed);
    (void)Rte_Read_EngineRpm(&rpm);

    alarm = ((speed >= APP_CFG_SPEED_ALARM_KPH_X10) ||
             (rpm >= APP_CFG_RPM_ALARM)) ? 1u : 0u;
    (void)Rte_Write_BuzzerAlarm(alarm);

    if ((alarm != 0u) && (App_DashboardMute == 0u))
    {
        BuzzerIf_On();
    }
    else
    {
        BuzzerIf_Off();
    }
}

void App_Dashboard_Init(void)
{
    App_DashboardSimMode = 0u;
    App_DashboardSimUp = 1u;
    App_DashboardMute = 0u;
    App_DashboardSimTimerMs = 0u;
    BuzzerIf_Init();
}

void App_Dashboard_MainFunction(uint16_t elapsed_ms)
{
    uint8_t key_event;

    key_event = Rte_EventPopKey();
    if (key_event != RTE_KEY_EVENT_NONE)
    {
        App_Dashboard_HandleKey(key_event);
    }

    App_Dashboard_UpdateSimulation(elapsed_ms);
    App_Dashboard_UpdateAlarm();
}
