#include "Rte_Signal.h"

#include "Can_Cfg.h"
#include "Os.h"
#include "Rte_Cfg.h"

static Rte_DashboardDataType Rte_DashboardData;
static Rte_EnvironmentDataType Rte_EnvironmentData;
static Rte_TpmsDataType Rte_TpmsData;
static Rte_ConfigDataType Rte_ConfigData;
static Rte_ControlDomainStatusType Rte_ControlDomainStatus;
static RtcIf_TimeType Rte_RtcTime;
static uint8_t Rte_RtcValid;
static Rte_KeyEventType Rte_KeyEvent;
static Rte_UserInputEventType Rte_UserInputEvent;
static uint8_t Rte_UserInputPending;
static uint8_t Rte_BacklightLevel;
static uint32_t Rte_LastPowertrainRxTick;
static uint32_t Rte_LastBodyRxTick;
static uint32_t Rte_LastTpmsRxTick;
static uint32_t Rte_LastConfigRxTick;
static uint32_t Rte_LastControlStatusRxTick;
static uint32_t Rte_LastControlNmRxTick;

void Rte_Signal_Init(void)
{
    Os_EnterCritical();

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
    Rte_DashboardData.vehicle_speed_valid = 0u;
    Rte_DashboardData.engine_rpm_valid = 0u;
    Rte_DashboardData.fuel_percent_valid = 0u;
    Rte_DashboardData.coolant_temp_valid = 0u;
    Rte_DashboardData.outdoor_temp_valid = 0u;
    Rte_DashboardData.battery_voltage_valid = 0u;
    Rte_DashboardData.ignition_status = 0u;
    Rte_DashboardData.gear_position = 0u;
    Rte_DashboardData.door_open_mask = 0u;
    Rte_DashboardData.driver_seatbelt = 0u;
    Rte_DashboardData.low_beam = 0u;
    Rte_DashboardData.high_beam = 0u;
    Rte_DashboardData.turn_signal = 0u;
    Rte_DashboardData.parking_brake = 0u;
    Rte_DashboardData.washer_fluid_low = 0u;
    Rte_DashboardData.ambient_light = 0u;
    Rte_DashboardData.ambient_light_valid = 0u;
    Rte_DashboardData.warning_flags = 0u;
    Rte_DashboardData.can_ems_valid = 0u;
    Rte_DashboardData.can_body_valid = 0u;
    Rte_DashboardData.alarm_active = 0u;
    Rte_DashboardData.buzzer_muted = 0u;
    Rte_DashboardData.simulation_mode = 0u;
    Rte_DashboardData.shutdown_request = 0u;

    Rte_EnvironmentData.temperature_c_x100 = 0;
    Rte_EnvironmentData.humidity_rh_x100 = 0;
    Rte_EnvironmentData.valid = 0u;

    Rte_TpmsData.pressure_bar_x100[0] = 0u;
    Rte_TpmsData.pressure_bar_x100[1] = 0u;
    Rte_TpmsData.pressure_bar_x100[2] = 0u;
    Rte_TpmsData.pressure_bar_x100[3] = 0u;
    Rte_TpmsData.temperature_c[0] = 0;
    Rte_TpmsData.temperature_c[1] = 0;
    Rte_TpmsData.temperature_c[2] = 0;
    Rte_TpmsData.temperature_c[3] = 0;
    Rte_TpmsData.pressure_valid[0] = 0u;
    Rte_TpmsData.pressure_valid[1] = 0u;
    Rte_TpmsData.pressure_valid[2] = 0u;
    Rte_TpmsData.pressure_valid[3] = 0u;
    Rte_TpmsData.temperature_valid[0] = 0u;
    Rte_TpmsData.temperature_valid[1] = 0u;
    Rte_TpmsData.temperature_valid[2] = 0u;
    Rte_TpmsData.temperature_valid[3] = 0u;
    Rte_TpmsData.warning_mask = 0u;
    Rte_TpmsData.valid = 0u;

    Rte_ConfigData.theme_mode = 0u;
    Rte_ConfigData.language = 0u;
    Rte_ConfigData.unit_mode = 0u;
    Rte_ConfigData.warning_volume = 0u;
    Rte_ConfigData.driving_range_km = 0u;
    Rte_ConfigData.datetime_valid = 0u;
    Rte_ConfigData.time_hour = 0u;
    Rte_ConfigData.time_minute = 0u;
    Rte_ConfigData.remote_valid = 0u;

    Rte_ControlDomainStatus.interface_version = 0u;
    Rte_ControlDomainStatus.version_compatible = 0u;
    Rte_ControlDomainStatus.input_mode = 3u;
    Rte_ControlDomainStatus.power_mode = 7u;
    Rte_ControlDomainStatus.health_state = 3u;
    Rte_ControlDomainStatus.remote_fault_present = 0u;
    Rte_ControlDomainStatus.remote_dtc_count = 0xFFu;
    Rte_ControlDomainStatus.domain_status_flags = 0u;
    Rte_ControlDomainStatus.last_remote_fault_id = 0xFFFFu;
    Rte_ControlDomainStatus.alive_counter = 0u;
    Rte_ControlDomainStatus.alive_stalled = 0u;
    Rte_ControlDomainStatus.status_valid = 0u;
    Rte_ControlDomainStatus.application_stale = 1u;
    Rte_ControlDomainStatus.nm_online = 0u;
    Rte_ControlDomainStatus.nm_node_address = 0xFFu;
    Rte_ControlDomainStatus.nm_repeat_status = 0u;
    Rte_ControlDomainStatus.nm_power_on_request = 0u;
    Rte_ControlDomainStatus.nm_diag_request = 0u;

    Rte_RtcTime.year = 2026u;
    Rte_RtcTime.month = 6u;
    Rte_RtcTime.date = 1u;
    Rte_RtcTime.weekday = 1u;
    Rte_RtcTime.hour = 0u;
    Rte_RtcTime.minute = 0u;
    Rte_RtcTime.second = 0u;
    Rte_RtcValid = 0u;

    Rte_KeyEvent = RTE_KEY_EVENT_NONE;
    Rte_UserInputEvent.key_code = 0u;
    Rte_UserInputEvent.key_action = 0u;
    Rte_UserInputEvent.event_counter = 0u;
    Rte_UserInputEvent.power_key_long_press = 0u;
    Rte_UserInputEvent.shutdown_confirm = 0u;
    Rte_UserInputPending = 0u;
    Rte_BacklightLevel = RTE_CFG_DEFAULT_BACKLIGHT_LEVEL;
    Rte_LastPowertrainRxTick = 0u;
    Rte_LastBodyRxTick = 0u;
    Rte_LastTpmsRxTick = 0u;
    Rte_LastConfigRxTick = 0u;
    Rte_LastControlStatusRxTick = 0u;
    Rte_LastControlNmRxTick = 0u;

    Os_ExitCritical();
}

Std_ReturnType Rte_Write_Powertrain(uint16_t speed_kph_x10,
                                    uint16_t rpm,
                                    uint8_t fuel_percent,
                                    int16_t coolant_temp_c,
                                    int16_t outdoor_temp_c,
                                    uint16_t battery_mv,
                                    uint8_t validity_mask,
                                    uint32_t tick_ms)
{
    Os_EnterCritical();

    if ((validity_mask & RTE_POWERTRAIN_VALID_SPEED) != 0u)
    {
        Rte_DashboardData.vehicle_speed_kph_x10 = speed_kph_x10;
    }
    if ((validity_mask & RTE_POWERTRAIN_VALID_RPM) != 0u)
    {
        Rte_DashboardData.engine_rpm = rpm;
    }
    if ((validity_mask & RTE_POWERTRAIN_VALID_FUEL) != 0u)
    {
        Rte_DashboardData.fuel_percent = fuel_percent;
    }
    if ((validity_mask & RTE_POWERTRAIN_VALID_COOLANT) != 0u)
    {
        Rte_DashboardData.coolant_temp_c = coolant_temp_c;
    }
    if ((validity_mask & RTE_POWERTRAIN_VALID_OUTDOOR) != 0u)
    {
        Rte_DashboardData.outdoor_temp_c = outdoor_temp_c;
    }
    if ((validity_mask & RTE_POWERTRAIN_VALID_BATTERY) != 0u)
    {
        Rte_DashboardData.battery_mv = battery_mv;
    }
    Rte_DashboardData.vehicle_speed_valid = ((validity_mask & RTE_POWERTRAIN_VALID_SPEED) != 0u) ? 1u : 0u;
    Rte_DashboardData.engine_rpm_valid = ((validity_mask & RTE_POWERTRAIN_VALID_RPM) != 0u) ? 1u : 0u;
    Rte_DashboardData.fuel_percent_valid = ((validity_mask & RTE_POWERTRAIN_VALID_FUEL) != 0u) ? 1u : 0u;
    Rte_DashboardData.coolant_temp_valid = ((validity_mask & RTE_POWERTRAIN_VALID_COOLANT) != 0u) ? 1u : 0u;
    Rte_DashboardData.outdoor_temp_valid = ((validity_mask & RTE_POWERTRAIN_VALID_OUTDOOR) != 0u) ? 1u : 0u;
    Rte_DashboardData.battery_voltage_valid = ((validity_mask & RTE_POWERTRAIN_VALID_BATTERY) != 0u) ? 1u : 0u;
    Rte_DashboardData.can_ems_valid = 1u;
    /*
     * tick_ms 是这帧动力域数据进入 RTE 的时间，后续超时判断只看这个时间戳，
     * 不依赖 Com 的内部调度状态。
     */
    Rte_LastPowertrainRxTick = tick_ms;

    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_BodyStatus(const Rte_BodyStatusType *body,
                                    uint32_t tick_ms)
{
    if (body == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();

    Rte_DashboardData.ignition_status = body->ignition_status;
    Rte_DashboardData.gear_position = body->gear_position;
    Rte_DashboardData.door_open_mask = body->door_open_mask;
    Rte_DashboardData.driver_seatbelt = body->driver_seatbelt;
    Rte_DashboardData.low_beam = body->low_beam;
    Rte_DashboardData.high_beam = body->high_beam;
    Rte_DashboardData.turn_signal = body->turn_signal;
    Rte_DashboardData.parking_brake = body->parking_brake;
    Rte_DashboardData.warning_flags = body->warning_flags;
    Rte_DashboardData.washer_fluid_low = body->washer_fluid_low;
    Rte_DashboardData.ambient_light = body->ambient_light;
    Rte_DashboardData.ambient_light_valid = body->ambient_light_valid;
    Rte_DashboardData.can_body_valid = 1u;
    /* 车身报文单独记录接收时间，避免动力域和车身域互相影响有效位判断。 */
    Rte_LastBodyRxTick = tick_ms;

    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_Environment(const Rte_EnvironmentDataType *environment)
{
    if (environment == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    Rte_EnvironmentData = *environment;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_TpmsStatus(const Rte_TpmsDataType *tpms, uint32_t tick_ms)
{
    if (tpms == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    Rte_TpmsData = *tpms;
    Rte_TpmsData.valid = 1u;
    Rte_LastTpmsRxTick = tick_ms;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_ConfigData(const Rte_ConfigDataType *config, uint32_t tick_ms)
{
    if (config == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    Rte_ConfigData = *config;
    Rte_LastConfigRxTick = tick_ms;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_ControlDomainStatus(const Rte_ControlDomainStatusType *status, uint32_t tick_ms)
{
    if (status == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    /*
     * 0x441 的 NM 字段由独立 API 更新，写 0x329 时不能覆盖它们。
     */
    Rte_ControlDomainStatus.interface_version = status->interface_version;
    Rte_ControlDomainStatus.version_compatible = status->version_compatible;
    Rte_ControlDomainStatus.input_mode = status->input_mode;
    Rte_ControlDomainStatus.power_mode = status->power_mode;
    Rte_ControlDomainStatus.health_state = status->health_state;
    Rte_ControlDomainStatus.remote_fault_present = status->remote_fault_present;
    Rte_ControlDomainStatus.remote_dtc_count = status->remote_dtc_count;
    Rte_ControlDomainStatus.domain_status_flags = status->domain_status_flags;
    Rte_ControlDomainStatus.last_remote_fault_id = status->last_remote_fault_id;
    Rte_ControlDomainStatus.alive_counter = status->alive_counter;
    Rte_ControlDomainStatus.alive_stalled = status->alive_stalled;
    Rte_ControlDomainStatus.status_valid = status->status_valid;
    Rte_ControlDomainStatus.application_stale = 0u;
    Rte_LastControlStatusRxTick = tick_ms;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_ControlDomainNm(uint8_t node_address,
                                        uint8_t repeat_status,
                                        uint8_t power_on_request,
                                        uint8_t diag_request,
                                        uint32_t tick_ms)
{
    Os_EnterCritical();
    Rte_ControlDomainStatus.nm_node_address = node_address;
    Rte_ControlDomainStatus.nm_repeat_status = (repeat_status != 0u) ? 1u : 0u;
    Rte_ControlDomainStatus.nm_power_on_request = (power_on_request != 0u) ? 1u : 0u;
    Rte_ControlDomainStatus.nm_diag_request = (diag_request != 0u) ? 1u : 0u;
    Rte_ControlDomainStatus.nm_online = 1u;
    Rte_LastControlNmRxTick = tick_ms;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_RtcTime(const RtcIf_TimeType *time, uint8_t valid)
{
    if (time == 0)
    {
        Os_EnterCritical();
        Rte_RtcValid = 0u;
        Os_ExitCritical();
        return E_NOT_OK;
    }

    Os_EnterCritical();
    Rte_RtcTime = *time;
    Rte_RtcValid = valid;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_KeyEvent(Rte_KeyEventType event)
{
    Os_EnterCritical();
    Rte_KeyEvent = event;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_UserInputEvent(const Rte_UserInputEventType *event)
{
    if (event == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    Rte_UserInputEvent = *event;
    Rte_UserInputPending = 1u;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_BacklightLevel(uint8_t level)
{
    if (level > 100u)
    {
        /* RTE 层先做一次钳位，避免异常配置继续向 BacklightIf 传播。 */
        level = 100u;
    }

    Os_EnterCritical();
    Rte_BacklightLevel = level;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_BuzzerMuted(uint8_t muted)
{
    Os_EnterCritical();
    Rte_DashboardData.buzzer_muted = (muted != 0u) ? 1u : 0u;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_SimulationMode(uint8_t enabled)
{
    Os_EnterCritical();
    Rte_DashboardData.simulation_mode = (enabled != 0u) ? 1u : 0u;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_AlarmActive(uint8_t active)
{
    Os_EnterCritical();
    Rte_DashboardData.alarm_active = (active != 0u) ? 1u : 0u;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Write_ShutdownRequest(uint8_t requested)
{
    Os_EnterCritical();
    Rte_DashboardData.shutdown_request = (requested != 0u) ? 1u : 0u;
    Os_ExitCritical();
    return E_OK;
}

void Rte_Clear_DashboardValues(void)
{
    Os_EnterCritical();
    Rte_DashboardData.vehicle_speed_kph_x10 = 0u;
    Rte_DashboardData.engine_rpm = 0u;
    Rte_DashboardData.vehicle_speed_valid = 0u;
    Rte_DashboardData.engine_rpm_valid = 0u;
    Rte_DashboardData.can_ems_valid = 0u;
    Os_ExitCritical();
}

void Rte_Update_CanValidity(uint32_t tick_ms)
{
    Os_EnterCritical();

    if ((Rte_DashboardData.can_ems_valid != 0u) &&
        ((tick_ms - Rte_LastPowertrainRxTick) > CAN_CFG_EMS_POWERTRAIN_TIMEOUT_MS))
    {
        /*
         * 使用无符号减法计算超时，tick_ms 回绕时仍能得到正确的时间差语义。
         * 超时只清 valid 位，不清最后一次数值，显示层可继续显示旧值并提示 CAN LOST。
         */
        Rte_DashboardData.can_ems_valid = 0u;
        Rte_DashboardData.vehicle_speed_valid = 0u;
        Rte_DashboardData.engine_rpm_valid = 0u;
        Rte_DashboardData.fuel_percent_valid = 0u;
        Rte_DashboardData.coolant_temp_valid = 0u;
        Rte_DashboardData.outdoor_temp_valid = 0u;
        Rte_DashboardData.battery_voltage_valid = 0u;
    }

    if ((Rte_DashboardData.can_body_valid != 0u) &&
        ((tick_ms - Rte_LastBodyRxTick) > CAN_CFG_BCM_BODYSTATUS_TIMEOUT_MS))
    {
        Rte_DashboardData.can_body_valid = 0u;
    }

    if ((Rte_TpmsData.valid != 0u) &&
        ((tick_ms - Rte_LastTpmsRxTick) > CAN_CFG_TPMS_STATUS_TIMEOUT_MS))
    {
        Rte_TpmsData.valid = 0u;
        Rte_TpmsData.pressure_valid[0] = 0u;
        Rte_TpmsData.pressure_valid[1] = 0u;
        Rte_TpmsData.pressure_valid[2] = 0u;
        Rte_TpmsData.pressure_valid[3] = 0u;
        Rte_TpmsData.temperature_valid[0] = 0u;
        Rte_TpmsData.temperature_valid[1] = 0u;
        Rte_TpmsData.temperature_valid[2] = 0u;
        Rte_TpmsData.temperature_valid[3] = 0u;
        Rte_TpmsData.warning_mask = 0u;
    }

    if ((Rte_ConfigData.remote_valid != 0u) &&
        ((tick_ms - Rte_LastConfigRxTick) > CAN_CFG_CONFIG_TIMEOUT_MS))
    {
        /* 只撤销远端 freshness；运行期最后合法值保留，重启后重新使用 NvM 基线。 */
        Rte_ConfigData.remote_valid = 0u;
    }

    if ((Rte_ControlDomainStatus.application_stale == 0u) &&
        ((tick_ms - Rte_LastControlStatusRxTick) > CAN_CFG_CDM_STATUS_TIMEOUT_MS))
    {
        Rte_ControlDomainStatus.status_valid = 0u;
        Rte_ControlDomainStatus.application_stale = 1u;
    }

    if ((Rte_ControlDomainStatus.nm_online != 0u) &&
        ((tick_ms - Rte_LastControlNmRxTick) > CAN_CFG_CDM_NM_TIMEOUT_MS))
    {
        Rte_ControlDomainStatus.nm_online = 0u;
    }

    Os_ExitCritical();
}

Std_ReturnType Rte_Read_DashboardData(Rte_DashboardDataType *data)
{
    if (data == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *data = Rte_DashboardData;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Read_Environment(Rte_EnvironmentDataType *environment)
{
    if (environment == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *environment = Rte_EnvironmentData;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Read_TpmsStatus(Rte_TpmsDataType *tpms)
{
    if (tpms == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *tpms = Rte_TpmsData;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Read_ConfigData(Rte_ConfigDataType *config)
{
    if (config == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *config = Rte_ConfigData;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Read_ControlDomainStatus(Rte_ControlDomainStatusType *status)
{
    if (status == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *status = Rte_ControlDomainStatus;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Read_RtcTime(RtcIf_TimeType *time, uint8_t *valid)
{
    if ((time == 0) || (valid == 0))
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *time = Rte_RtcTime;
    *valid = Rte_RtcValid;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Read_KeyEvent(Rte_KeyEventType *event)
{
    if (event == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *event = Rte_KeyEvent;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Take_KeyEvent(Rte_KeyEventType *event)
{
    if (event == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *event = Rte_KeyEvent;
    /*
     * Take 语义用于边沿事件：读出后立即清空。
     * 如果业务只想观察当前槽位而不消费，应调用 Rte_Read_KeyEvent()。
     */
    Rte_KeyEvent = RTE_KEY_EVENT_NONE;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Take_UserInputEvent(Rte_UserInputEventType *event)
{
    if (event == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    if (Rte_UserInputPending == 0u)
    {
        Os_ExitCritical();
        return E_NOT_OK;
    }

    *event = Rte_UserInputEvent;
    Rte_UserInputPending = 0u;
    Os_ExitCritical();
    return E_OK;
}

Std_ReturnType Rte_Read_BacklightLevel(uint8_t *level)
{
    if (level == 0)
    {
        return E_NOT_OK;
    }

    Os_EnterCritical();
    *level = Rte_BacklightLevel;
    Os_ExitCritical();
    return E_OK;
}

uint32_t Rte_GetLastPowertrainRxTick(void)
{
    uint32_t tick_ms;

    Os_EnterCritical();
    tick_ms = Rte_LastPowertrainRxTick;
    Os_ExitCritical();
    return tick_ms;
}

uint8_t Rte_IsPowertrainValid(void)
{
    uint8_t valid;

    Os_EnterCritical();
    valid = Rte_DashboardData.can_ems_valid;
    Os_ExitCritical();
    return valid;
}

uint8_t Rte_IsTpmsValid(void)
{
    uint8_t valid;

    Os_EnterCritical();
    valid = Rte_TpmsData.valid;
    Os_ExitCritical();
    return valid;
}
