#include "Rte_Signal.h"

#include "Can_Cfg.h"
#include "Rte_Cfg.h"

static Rte_DashboardDataType Rte_DashboardData;
static Rte_EnvironmentDataType Rte_EnvironmentData;
static RtcIf_TimeType Rte_RtcTime;
static uint8_t Rte_RtcValid;
static Rte_KeyEventType Rte_KeyEvent;
static uint8_t Rte_BacklightLevel;
static uint32_t Rte_LastPowertrainRxTick;
static uint32_t Rte_LastBodyRxTick;

void Rte_Signal_Init(void)
{
    Rte_DashboardData.vehicle_speed_kph_x10 = 0u;
    Rte_DashboardData.engine_rpm = 0u;
    Rte_DashboardData.fuel_percent = 0u;
    Rte_DashboardData.coolant_temp_c = 0;
    Rte_DashboardData.outdoor_temp_c = 0;
    Rte_DashboardData.battery_mv = 0u;
    Rte_DashboardData.ignition_status = 0u;
    Rte_DashboardData.gear_position = 0u;
    Rte_DashboardData.warning_flags = 0u;
    Rte_DashboardData.can_ems_valid = 0u;
    Rte_DashboardData.can_body_valid = 0u;
    Rte_DashboardData.alarm_active = 0u;
    Rte_DashboardData.buzzer_muted = 0u;
    Rte_DashboardData.simulation_mode = 0u;

    Rte_EnvironmentData.temperature_c_x100 = 0;
    Rte_EnvironmentData.humidity_rh_x100 = 0;
    Rte_EnvironmentData.valid = 0u;

    Rte_RtcTime.year = 2026u;
    Rte_RtcTime.month = 6u;
    Rte_RtcTime.date = 1u;
    Rte_RtcTime.weekday = 1u;
    Rte_RtcTime.hour = 0u;
    Rte_RtcTime.minute = 0u;
    Rte_RtcTime.second = 0u;
    Rte_RtcValid = 0u;

    Rte_KeyEvent = RTE_KEY_EVENT_NONE;
    Rte_BacklightLevel = RTE_CFG_DEFAULT_BACKLIGHT_LEVEL;
    Rte_LastPowertrainRxTick = 0u;
    Rte_LastBodyRxTick = 0u;
}

Std_ReturnType Rte_Write_Powertrain(uint16_t speed_kph_x10,
                                    uint16_t rpm,
                                    uint8_t fuel_percent,
                                    int16_t coolant_temp_c,
                                    int16_t outdoor_temp_c,
                                    uint16_t battery_mv,
                                    uint32_t tick_ms)
{
    Rte_DashboardData.vehicle_speed_kph_x10 = speed_kph_x10;
    Rte_DashboardData.engine_rpm = rpm;
    Rte_DashboardData.fuel_percent = fuel_percent;
    Rte_DashboardData.coolant_temp_c = coolant_temp_c;
    Rte_DashboardData.outdoor_temp_c = outdoor_temp_c;
    Rte_DashboardData.battery_mv = battery_mv;
    Rte_DashboardData.can_ems_valid = 1u;
    Rte_LastPowertrainRxTick = tick_ms;
    return E_OK;
}

Std_ReturnType Rte_Write_BodyStatus(uint8_t ignition_status,
                                    uint8_t gear_position,
                                    uint8_t warning_flags,
                                    uint32_t tick_ms)
{
    Rte_DashboardData.ignition_status = ignition_status;
    Rte_DashboardData.gear_position = gear_position;
    Rte_DashboardData.warning_flags = warning_flags;
    Rte_DashboardData.can_body_valid = 1u;
    Rte_LastBodyRxTick = tick_ms;
    return E_OK;
}

Std_ReturnType Rte_Write_Environment(const Rte_EnvironmentDataType *environment)
{
    if (environment == 0)
    {
        return E_NOT_OK;
    }

    Rte_EnvironmentData = *environment;
    return E_OK;
}

Std_ReturnType Rte_Write_RtcTime(const RtcIf_TimeType *time, uint8_t valid)
{
    if (time == 0)
    {
        Rte_RtcValid = 0u;
        return E_NOT_OK;
    }

    Rte_RtcTime = *time;
    Rte_RtcValid = valid;
    return E_OK;
}

Std_ReturnType Rte_Write_KeyEvent(Rte_KeyEventType event)
{
    Rte_KeyEvent = event;
    return E_OK;
}

Std_ReturnType Rte_Write_BacklightLevel(uint8_t level)
{
    if (level > 100u)
    {
        level = 100u;
    }

    Rte_BacklightLevel = level;
    return E_OK;
}

Std_ReturnType Rte_Write_BuzzerMuted(uint8_t muted)
{
    Rte_DashboardData.buzzer_muted = (muted != 0u) ? 1u : 0u;
    return E_OK;
}

Std_ReturnType Rte_Write_SimulationMode(uint8_t enabled)
{
    Rte_DashboardData.simulation_mode = (enabled != 0u) ? 1u : 0u;
    return E_OK;
}

Std_ReturnType Rte_Write_AlarmActive(uint8_t active)
{
    Rte_DashboardData.alarm_active = (active != 0u) ? 1u : 0u;
    return E_OK;
}

void Rte_Clear_DashboardValues(void)
{
    Rte_DashboardData.vehicle_speed_kph_x10 = 0u;
    Rte_DashboardData.engine_rpm = 0u;
    Rte_DashboardData.can_ems_valid = 0u;
}

void Rte_Update_CanValidity(uint32_t tick_ms)
{
    if ((Rte_DashboardData.can_ems_valid != 0u) &&
        ((tick_ms - Rte_LastPowertrainRxTick) > CAN_CFG_EMS_POWERTRAIN_TIMEOUT_MS))
    {
        Rte_DashboardData.can_ems_valid = 0u;
    }

    if ((Rte_DashboardData.can_body_valid != 0u) &&
        ((tick_ms - Rte_LastBodyRxTick) > CAN_CFG_BCM_BODYSTATUS_TIMEOUT_MS))
    {
        Rte_DashboardData.can_body_valid = 0u;
    }
}

Std_ReturnType Rte_Read_DashboardData(Rte_DashboardDataType *data)
{
    if (data == 0)
    {
        return E_NOT_OK;
    }

    *data = Rte_DashboardData;
    return E_OK;
}

Std_ReturnType Rte_Read_Environment(Rte_EnvironmentDataType *environment)
{
    if (environment == 0)
    {
        return E_NOT_OK;
    }

    *environment = Rte_EnvironmentData;
    return E_OK;
}

Std_ReturnType Rte_Read_RtcTime(RtcIf_TimeType *time, uint8_t *valid)
{
    if ((time == 0) || (valid == 0))
    {
        return E_NOT_OK;
    }

    *time = Rte_RtcTime;
    *valid = Rte_RtcValid;
    return E_OK;
}

Std_ReturnType Rte_Read_KeyEvent(Rte_KeyEventType *event)
{
    if (event == 0)
    {
        return E_NOT_OK;
    }

    *event = Rte_KeyEvent;
    return E_OK;
}

Std_ReturnType Rte_Take_KeyEvent(Rte_KeyEventType *event)
{
    if (event == 0)
    {
        return E_NOT_OK;
    }

    *event = Rte_KeyEvent;
    Rte_KeyEvent = RTE_KEY_EVENT_NONE;
    return E_OK;
}

Std_ReturnType Rte_Read_BacklightLevel(uint8_t *level)
{
    if (level == 0)
    {
        return E_NOT_OK;
    }

    *level = Rte_BacklightLevel;
    return E_OK;
}

uint32_t Rte_GetLastPowertrainRxTick(void)
{
    return Rte_LastPowertrainRxTick;
}

uint8_t Rte_IsPowertrainValid(void)
{
    return Rte_DashboardData.can_ems_valid;
}
#include "Rte_Signal.h"

#include "Can_Cfg.h"
#include "Rte_Cfg.h"

static Rte_DashboardDataType Rte_Data;
static uint16_t Rte_CanEmsAgeMs;
static uint16_t Rte_SensorAgeMs;
static uint16_t Rte_RtcAgeMs;

static uint16_t Rte_AddAge(uint16_t age, uint16_t elapsed_ms)
{
    uint32_t next;

    next = (uint32_t)age + elapsed_ms;
    if (next > RTE_CFG_SIGNAL_AGE_SATURATION_MS)
    {
        next = RTE_CFG_SIGNAL_AGE_SATURATION_MS;
    }

    return (uint16_t)next;
}

void Rte_SignalInit(void)
{
    Rte_Data.speed_kph_x10 = 0u;
    Rte_Data.engine_rpm = 0u;
    Rte_Data.fuel_percent = 0u;
    Rte_Data.coolant_temp_c = 0;
    Rte_Data.outdoor_temp_c = 0;
    Rte_Data.battery_mv = 0u;
    Rte_Data.ignition_status = 0u;
    Rte_Data.gear_position = 0u;
    Rte_Data.warning_flags = 0u;
    Rte_Data.door_flags = 0u;
    Rte_Data.config_theme = 0u;
    Rte_Data.backlight_level = RTE_CFG_DEFAULT_BACKLIGHT_LEVEL;
    Rte_Data.sht30_temp_c_x10 = 0;
    Rte_Data.sht30_humidity_x10 = 0u;
    Rte_Data.rtc_time.year = 2026u;
    Rte_Data.rtc_time.month = 6u;
    Rte_Data.rtc_time.date = 1u;
    Rte_Data.rtc_time.weekday = 3u;
    Rte_Data.rtc_time.hour = 0u;
    Rte_Data.rtc_time.minute = 0u;
    Rte_Data.rtc_time.second = 0u;
    Rte_Data.key_event = 0u;
    Rte_Data.buzzer_enable = RTE_CFG_DEFAULT_BUZZER_ENABLE;
    Rte_Data.buzzer_alarm = 0u;
    Rte_Data.can_ems_valid = 0u;
    Rte_Data.sensor_valid = 0u;
    Rte_Data.rtc_valid = 0u;
    Rte_CanEmsAgeMs = RTE_CFG_SIGNAL_AGE_SATURATION_MS;
    Rte_SensorAgeMs = RTE_CFG_SIGNAL_AGE_SATURATION_MS;
    Rte_RtcAgeMs = RTE_CFG_SIGNAL_AGE_SATURATION_MS;
}

void Rte_SignalMainFunction(uint16_t elapsed_ms)
{
    Rte_CanEmsAgeMs = Rte_AddAge(Rte_CanEmsAgeMs, elapsed_ms);
    Rte_SensorAgeMs = Rte_AddAge(Rte_SensorAgeMs, elapsed_ms);
    Rte_RtcAgeMs = Rte_AddAge(Rte_RtcAgeMs, elapsed_ms);

    Rte_Data.can_ems_valid = (Rte_CanEmsAgeMs <= CAN_CFG_EMS_POWERTRAIN_TIMEOUT_MS) ? 1u : 0u;
    Rte_Data.sensor_valid = (Rte_SensorAgeMs <= 3000u) ? 1u : 0u;
    Rte_Data.rtc_valid = (Rte_RtcAgeMs <= 3000u) ? 1u : 0u;
}

Std_ReturnType Rte_Write_VehicleSpeed(uint16_t speed_kph_x10)
{
    Rte_Data.speed_kph_x10 = speed_kph_x10;
    return E_OK;
}

Std_ReturnType Rte_Write_EngineRpm(uint16_t rpm)
{
    Rte_Data.engine_rpm = rpm;
    return E_OK;
}

Std_ReturnType Rte_Write_FuelPercent(uint8_t fuel_percent)
{
    Rte_Data.fuel_percent = (fuel_percent > 100u) ? 100u : fuel_percent;
    return E_OK;
}

Std_ReturnType Rte_Write_CoolantTemp(int16_t temp_c)
{
    Rte_Data.coolant_temp_c = temp_c;
    return E_OK;
}

Std_ReturnType Rte_Write_OutdoorTemp(int16_t temp_c)
{
    Rte_Data.outdoor_temp_c = temp_c;
    return E_OK;
}

Std_ReturnType Rte_Write_BatteryVoltage(uint16_t battery_mv)
{
    Rte_Data.battery_mv = battery_mv;
    return E_OK;
}

Std_ReturnType Rte_Write_IgnitionStatus(uint8_t ignition_status)
{
    Rte_Data.ignition_status = ignition_status;
    return E_OK;
}

Std_ReturnType Rte_Write_GearPosition(uint8_t gear_position)
{
    Rte_Data.gear_position = gear_position;
    return E_OK;
}

Std_ReturnType Rte_Write_WarningFlags(uint8_t warning_flags)
{
    Rte_Data.warning_flags = warning_flags;
    return E_OK;
}

Std_ReturnType Rte_Write_DoorFlags(uint8_t door_flags)
{
    Rte_Data.door_flags = door_flags;
    return E_OK;
}

Std_ReturnType Rte_Write_ConfigTheme(uint8_t theme)
{
    Rte_Data.config_theme = theme;
    return E_OK;
}

Std_ReturnType Rte_Write_BacklightLevel(uint8_t level)
{
    Rte_Data.backlight_level = (level > 100u) ? 100u : level;
    return E_OK;
}

Std_ReturnType Rte_Write_Sht30(int16_t temp_c_x10, uint16_t humidity_x10)
{
    Rte_Data.sht30_temp_c_x10 = temp_c_x10;
    Rte_Data.sht30_humidity_x10 = humidity_x10;
    Rte_Data.sensor_valid = 1u;
    Rte_SensorAgeMs = 0u;
    return E_OK;
}

Std_ReturnType Rte_Write_RtcTime(const RtcIf_TimeType *time)
{
    if (time == 0)
    {
        return E_NOT_OK;
    }

    Rte_Data.rtc_time = *time;
    Rte_Data.rtc_valid = 1u;
    Rte_RtcAgeMs = 0u;
    return E_OK;
}

Std_ReturnType Rte_Write_KeyEvent(uint8_t key_event)
{
    Rte_Data.key_event = key_event;
    return E_OK;
}

Std_ReturnType Rte_Write_BuzzerEnable(uint8_t enable)
{
    Rte_Data.buzzer_enable = (enable != 0u) ? 1u : 0u;
    return E_OK;
}

Std_ReturnType Rte_Write_BuzzerAlarm(uint8_t alarm)
{
    Rte_Data.buzzer_alarm = (alarm != 0u) ? 1u : 0u;
    return E_OK;
}

void Rte_MarkCanEmsReceived(void)
{
    Rte_CanEmsAgeMs = 0u;
    Rte_Data.can_ems_valid = 1u;
}

Std_ReturnType Rte_Read_DashboardData(Rte_DashboardDataType *data)
{
    if (data == 0)
    {
        return E_NOT_OK;
    }

    *data = Rte_Data;
    return E_OK;
}

Std_ReturnType Rte_Read_VehicleSpeed(uint16_t *speed_kph_x10)
{
    if (speed_kph_x10 == 0)
    {
        return E_NOT_OK;
    }

    *speed_kph_x10 = Rte_Data.speed_kph_x10;
    return E_OK;
}

Std_ReturnType Rte_Read_EngineRpm(uint16_t *rpm)
{
    if (rpm == 0)
    {
        return E_NOT_OK;
    }

    *rpm = Rte_Data.engine_rpm;
    return E_OK;
}

Std_ReturnType Rte_Read_BatteryVoltage(uint16_t *battery_mv)
{
    if (battery_mv == 0)
    {
        return E_NOT_OK;
    }

    *battery_mv = Rte_Data.battery_mv;
    return E_OK;
}

Std_ReturnType Rte_Read_RtcTime(RtcIf_TimeType *time)
{
    if (time == 0)
    {
        return E_NOT_OK;
    }

    *time = Rte_Data.rtc_time;
    return E_OK;
}

uint16_t Rte_GetCanEmsAgeMs(void)
{
    return Rte_CanEmsAgeMs;
}

uint8_t Rte_IsCanEmsValid(void)
{
    return Rte_Data.can_ems_valid;
}
