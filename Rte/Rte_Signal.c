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
    /*
     * 所有 RTE 信号都初始化为“安全且可显示”的默认值：
     * 数值为 0，有效位为 0，背光使用配置默认值，RTC 给一个固定日期但标记 invalid。
     */
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
    /*
     * tick_ms 是这帧动力域数据进入 RTE 的时间，后续超时判断只看这个时间戳，
     * 不依赖 Com 的内部调度状态。
     */
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
    /* 车身报文单独记录接收时间，避免动力域和车身域互相影响有效位判断。 */
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
        /* RTE 层先做一次钳位，避免异常配置继续向 BacklightIf 传播。 */
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
        /*
         * 使用无符号减法计算超时，tick_ms 回绕时仍能得到正确的时间差语义。
         * 超时只清 valid 位，不清最后一次数值，显示层可继续显示旧值并提示 CAN LOST。
         */
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
    /*
     * Take 语义用于边沿事件：读出后立即清空。
     * 如果业务只想观察当前槽位而不消费，应调用 Rte_Read_KeyEvent()。
     */
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
