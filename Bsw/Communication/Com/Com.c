#include "Com.h"

#include "Can_Cfg.h"
#include "CanSM.h"
#include "Com_Cfg.h"
#include "Crc.h"
#include "Dcm.h"
#include "Dcm_Cfg.h"
#include "Dem.h"
#include "EcuM.h"
#include "FatFsIf.h"
#include "LcdIf.h"
#include "NvM.h"
#include "Rte_Signal.h"
#include "SdramIf.h"

static uint32_t Com_NextStatusTxMs;
static uint32_t Com_NextDiagStatusTxMs;
static uint32_t Com_NextNmTxMs;
static uint32_t Com_NextLogStatusTxMs;
static uint8_t Com_PowertrainSeen;
static uint8_t Com_ControlAliveSeen;
static uint8_t Com_LastControlAlive;
static uint8_t Com_ControlAliveRepeatCount;
static uint8_t Com_UserInputPending;
static Rte_UserInputEventType Com_PendingUserInput;

static uint16_t Com_ReadLeU16(const uint8_t *data)
{
    /* CAN 矩阵中的多字节业务信号按 little-endian 打包。 */
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static void Com_WriteLeU16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)((value >> 8u) & 0xFFu);
}

static void Com_WriteLeU32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)((value >> 8u) & 0xFFu);
    data[2] = (uint8_t)((value >> 16u) & 0xFFu);
    data[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

void Com_Init(void)
{
    Com_NextStatusTxMs = 100u;
    Com_NextDiagStatusTxMs = 1000u;
    Com_NextNmTxMs = 1000u;
    Com_NextLogStatusTxMs = 1000u;
    Com_PowertrainSeen = 0u;
    Com_ControlAliveSeen = 0u;
    Com_LastControlAlive = 0u;
    Com_ControlAliveRepeatCount = 0u;
    Com_UserInputPending = 0u;
}

static void Com_DecodePowertrain(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    uint16_t speed_raw;
    uint16_t rpm_raw;
    uint16_t speed_x10;
    uint16_t rpm;
    uint8_t fuel_percent;
    int16_t coolant_c;
    int16_t outdoor_c;
    uint16_t battery_mv;
    uint8_t validity_mask;

    if (pdu->dlc < 8u)
    {
        return;
    }

    /*
     * 0x321 EMS_Powertrain_20ms：
     * Byte0-1 VehicleSpeed，Byte2-3 EngineSpeed，其余是油量/水温/外温/电压。
     * 物理值换算集中在 Com_Cfg.h，后续换 DBC 时不用改 APP。
     */
    speed_raw = (uint16_t)(Com_ReadLeU16(&pdu->data[0]) & COM_CFG_SPEED_RAW_MASK);
    rpm_raw = Com_ReadLeU16(&pdu->data[2]);
    speed_x10 = 0u;
    rpm = 0u;
    fuel_percent = 0u;
    coolant_c = 0;
    outdoor_c = 0;
    battery_mv = 0u;
    validity_mask = 0u;

    if (speed_raw != COM_CFG_SPEED_RAW_INVALID)
    {
        speed_x10 = (uint16_t)(((uint32_t)speed_raw * COM_CFG_SPEED_RAW_TO_X10_NUM) /
                               COM_CFG_SPEED_RAW_TO_X10_DEN);
        validity_mask |= RTE_POWERTRAIN_VALID_SPEED;
    }
    if (rpm_raw != COM_CFG_U16_RAW_INVALID)
    {
        rpm = (uint16_t)(((uint32_t)rpm_raw * COM_CFG_RPM_RAW_TO_RPM_NUM) /
                         COM_CFG_RPM_RAW_TO_RPM_DEN);
        validity_mask |= RTE_POWERTRAIN_VALID_RPM;
    }
    if (pdu->data[4] != COM_CFG_U8_RAW_INVALID)
    {
        fuel_percent = (uint8_t)(((uint16_t)pdu->data[4] * COM_CFG_FUEL_RAW_TO_PERCENT_NUM) /
                                 COM_CFG_FUEL_RAW_TO_PERCENT_DEN);
        if (fuel_percent > 100u)
        {
            fuel_percent = 100u;
        }
        validity_mask |= RTE_POWERTRAIN_VALID_FUEL;
    }
    if (pdu->data[5] != COM_CFG_U8_RAW_INVALID)
    {
        coolant_c = (int16_t)((int16_t)pdu->data[5] - COM_CFG_TEMP_OFFSET_C);
        validity_mask |= RTE_POWERTRAIN_VALID_COOLANT;
    }
    if (pdu->data[6] != COM_CFG_U8_RAW_INVALID)
    {
        outdoor_c = (int16_t)((int16_t)pdu->data[6] - COM_CFG_TEMP_OFFSET_C);
        validity_mask |= RTE_POWERTRAIN_VALID_OUTDOOR;
    }
    if (pdu->data[7] != COM_CFG_U8_RAW_INVALID)
    {
        battery_mv = (uint16_t)((uint16_t)pdu->data[7] * COM_CFG_BATTERY_RAW_TO_MV);
        validity_mask |= RTE_POWERTRAIN_VALID_BATTERY;
    }

    /*
     * Com 只负责把 CAN 原始值转换成统一物理单位。
     * RTE 再负责保存快照和最近接收时间，App_Display/App_Dashboard 不直接碰 CAN 字节。
     */
    (void)Rte_Write_Powertrain(speed_x10,
                               rpm,
                               fuel_percent,
                               coolant_c,
                               outdoor_c,
                               battery_mv,
                               validity_mask,
                               tick_ms);
    Com_PowertrainSeen = 1u;
    Dem_SetEventStatus(DEM_EVENT_CAN_RX_TIMEOUT, DEM_EVENT_STATUS_PASSED);
}

static void Com_DecodeBodyStatus(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    Rte_BodyStatusType body;

    if (pdu->dlc < 6u)
    {
        return;
    }

    /*
     * 0x322 BCM_BodyStatus_100ms 的 17 个信号全部解码。
     * 离散状态保留 raw 枚举，显示/业务层不再接触 CAN 字节。
     */
    body.ignition_status = (uint8_t)(pdu->data[0] & 0x03u);
    body.gear_position = (uint8_t)((pdu->data[0] >> 2u) & 0x0Fu);
    body.door_open_mask = (uint8_t)(pdu->data[1] & 0x0Fu);
    body.driver_seatbelt = (uint8_t)((pdu->data[1] >> 4u) & 0x01u);
    body.low_beam = (uint8_t)(pdu->data[2] & 0x01u);
    body.high_beam = (uint8_t)((pdu->data[2] >> 1u) & 0x01u);
    body.turn_signal = (uint8_t)((pdu->data[2] >> 2u) & 0x03u);
    body.parking_brake = (uint8_t)((pdu->data[2] >> 4u) & 0x01u);
    body.warning_flags = (uint8_t)(pdu->data[3] & 0x0Fu);
    body.washer_fluid_low = (uint8_t)(pdu->data[4] & 0x01u);
    body.ambient_light = pdu->data[5];
    body.ambient_light_valid = (pdu->data[5] != COM_CFG_U8_RAW_INVALID) ? 1u : 0u;
    (void)Rte_Write_BodyStatus(&body, tick_ms);
}

static void Com_DecodeTpms(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    Rte_TpmsDataType tpms;
    uint8_t index;

    if (pdu->dlc < 8u)
    {
        return;
    }

    /*
     * 0x323 TPMS_Status_1000ms：
     * Byte0-3 为四轮胎压 raw，分辨率 0.0275bar/bit；
     * Byte4-7 为四轮胎温 raw，物理值 = raw - 40。
     */
    for (index = 0u; index < 4u; index++)
    {
        tpms.pressure_valid[index] = (pdu->data[index] != COM_CFG_U8_RAW_INVALID) ? 1u : 0u;
        tpms.temperature_valid[index] = (pdu->data[index + 4u] != COM_CFG_U8_RAW_INVALID) ? 1u : 0u;
        tpms.pressure_bar_x100[index] = 0u;
        tpms.temperature_c[index] = 0;

        if (tpms.pressure_valid[index] != 0u)
        {
            tpms.pressure_bar_x100[index] =
                (uint16_t)(((uint16_t)pdu->data[index] * 275u) / 100u);
        }
        if (tpms.temperature_valid[index] != 0u)
        {
            tpms.temperature_c[index] =
                (int16_t)((int16_t)pdu->data[index + 4u] - COM_CFG_TEMP_OFFSET_C);
        }

    }
    tpms.warning_mask = 0u;
    for (index = 0u; index < 4u; index++)
    {
        if (((tpms.pressure_valid[index] != 0u) &&
             ((tpms.pressure_bar_x100[index] < COM_CFG_TPMS_LOW_PRESSURE_X100) ||
              (tpms.pressure_bar_x100[index] > COM_CFG_TPMS_HIGH_PRESSURE_X100))) ||
            ((tpms.temperature_valid[index] != 0u) &&
             (tpms.temperature_c[index] > COM_CFG_TPMS_HIGH_TEMPERATURE_C)))
        {
            tpms.warning_mask |= (uint8_t)(1u << index);
        }
    }
    tpms.valid = 1u;

    (void)Rte_Write_TpmsStatus(&tpms, tick_ms);
}

static void Com_DecodeConfig(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    Rte_ConfigDataType config;
    uint8_t backlight;
    uint8_t theme;
    uint8_t language;
    uint8_t unit_mode;

    if (pdu->dlc >= 8u)
    {
        /*
         * 0x324 IHU_CGW_Config_500ms：
         * Byte0 打包主题/语言/单位制，Byte1 是背光目标亮度，
         * Byte2 是报警音量，Byte3-4 是续航里程，Byte5-7 是外部时间同步。
         */
        (void)Rte_Read_ConfigData(&config);
        theme = (uint8_t)(pdu->data[0] & 0x07u);
        language = (uint8_t)((pdu->data[0] >> 3u) & 0x07u);
        unit_mode = (uint8_t)((pdu->data[0] >> 6u) & 0x03u);
        if (theme <= 2u)
        {
            config.theme_mode = theme;
        }
        if (language <= 1u)
        {
            config.language = language;
        }
        if (unit_mode <= 1u)
        {
            config.unit_mode = unit_mode;
        }
        if (pdu->data[2] <= 100u)
        {
            config.warning_volume = pdu->data[2];
        }
        if (Com_ReadLeU16(&pdu->data[3]) != COM_CFG_U16_RAW_INVALID)
        {
            config.driving_range_km = Com_ReadLeU16(&pdu->data[3]);
        }
        config.datetime_valid = 0u;
        if (((pdu->data[5] & 0x01u) != 0u) &&
            (pdu->data[6] <= 23u) &&
            (pdu->data[7] <= 59u))
        {
            config.datetime_valid = 1u;
            config.time_hour = pdu->data[6];
            config.time_minute = pdu->data[7];
        }
        config.remote_valid = 1u;

        backlight = pdu->data[1];
        if (backlight <= 100u)
        {
            (void)Rte_Write_BacklightLevel(backlight);
        }

        (void)Rte_Write_ConfigData(&config, tick_ms);
    }
}

static void Com_DecodeControlDomainNm(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    if (pdu->dlc < 3u)
    {
        return;
    }

    (void)Rte_Write_ControlDomainNm(pdu->data[0],
                                    (uint8_t)(pdu->data[2] & 0x01u),
                                    (uint8_t)((pdu->data[2] >> 1u) & 0x01u),
                                    (uint8_t)((pdu->data[2] >> 2u) & 0x01u),
                                    tick_ms);
}

static void Com_DecodeControlDomainStatus(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    uint8_t crc_data[9];
    uint8_t index;
    Rte_ControlDomainStatusType status;

    if (pdu->dlc < 8u)
    {
        return;
    }

    crc_data[0] = (uint8_t)(CAN_ID_CDM_STATUS_500MS & 0xFFu);
    crc_data[1] = (uint8_t)((CAN_ID_CDM_STATUS_500MS >> 8u) & 0xFFu);
    for (index = 0u; index < 7u; index++)
    {
        crc_data[index + 2u] = pdu->data[index];
    }
    if (Crc_CalculateCrc8J1850(crc_data, sizeof(crc_data)) != pdu->data[7])
    {
        /* CRC 错误帧不刷新 freshness，也不覆盖最后一次可信状态。 */
        return;
    }

    status.interface_version = pdu->data[0];
    status.version_compatible =
        (pdu->data[0] == CAN_CFG_INTERFACE_VERSION_V1_0) ? 1u : 0u;
    status.input_mode = (uint8_t)(pdu->data[1] & 0x03u);
    status.power_mode = (uint8_t)((pdu->data[1] >> 2u) & 0x07u);
    status.health_state = (uint8_t)((pdu->data[1] >> 5u) & 0x03u);
    status.remote_fault_present = (uint8_t)((pdu->data[1] >> 7u) & 0x01u);
    status.remote_dtc_count = pdu->data[2];
    status.domain_status_flags = pdu->data[3];
    status.last_remote_fault_id = Com_ReadLeU16(&pdu->data[4]);
    status.alive_counter = pdu->data[6];

    if ((Com_ControlAliveSeen != 0u) && (status.alive_counter == Com_LastControlAlive))
    {
        if (Com_ControlAliveRepeatCount < 0xFFu)
        {
            Com_ControlAliveRepeatCount++;
        }
    }
    else
    {
        Com_ControlAliveRepeatCount = 0u;
    }
    Com_ControlAliveSeen = 1u;
    Com_LastControlAlive = status.alive_counter;
    status.alive_stalled =
        (Com_ControlAliveRepeatCount >= CAN_CFG_CDM_ALIVE_STALL_FRAMES) ? 1u : 0u;
    status.status_valid = ((status.version_compatible != 0u) &&
                           (status.input_mode != 3u) &&
                           (status.power_mode != 7u) &&
                           (status.health_state != 3u)) ? 1u : 0u;
    status.application_stale = 0u;
    status.nm_online = 0u;
    status.nm_node_address = 0u;
    status.nm_repeat_status = 0u;
    status.nm_power_on_request = 0u;
    status.nm_diag_request = 0u;
    (void)Rte_Write_ControlDomainStatus(&status, tick_ms);
}

void Com_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    if (pdu == 0)
    {
        return;
    }

    switch (pdu->id)
    {
    case CAN_ID_EMS_POWERTRAIN_20MS:
        Com_DecodePowertrain(pdu, tick_ms);
        break;

    case CAN_ID_BCM_BODYSTATUS_100MS:
        Com_DecodeBodyStatus(pdu, tick_ms);
        break;

    case CAN_ID_TPMS_STATUS_1000MS:
        Com_DecodeTpms(pdu, tick_ms);
        break;

    case CAN_ID_IHU_CGW_CONFIG_500MS:
        Com_DecodeConfig(pdu, tick_ms);
        break;

    case CAN_ID_CDM_STATUS_500MS:
        Com_DecodeControlDomainStatus(pdu, tick_ms);
        break;

    case CAN_ID_NM_AUTOSAR_SIM:
        Com_DecodeControlDomainNm(pdu, tick_ms);
        break;

    default:
        /* 未配置的 CAN ID 静默丢弃，避免未知帧影响仪表状态。 */
        break;
    }
}

static uint8_t Com_MapEcuMode(EcuM_StateType state)
{
    if (state == ECUM_STATE_RUN)
    {
        return 1u;
    }
    if (state == ECUM_STATE_SLEEP_PREPARE)
    {
        return 2u;
    }
    if (state == ECUM_STATE_OFF)
    {
        return 3u;
    }
    if (state == ECUM_STATE_FAULT)
    {
        return 4u;
    }

    return 0u;
}

static uint8_t Com_MapDiagnosticSession(uint8_t session)
{
    if (session == DCM_SESSION_PROGRAMMING)
    {
        return 1u;
    }
    if (session == DCM_SESSION_EXTENDED)
    {
        return 2u;
    }
    return 0u;
}

static void Com_SendIcmStatus(uint32_t tick_ms)
{
    Rte_DashboardDataType data;
    uint8_t frame[8];
    uint8_t backlight;
    uint16_t drive_minutes;

    if (tick_ms < Com_NextStatusTxMs)
    {
        return;
    }
    Com_NextStatusTxMs = tick_ms + 100u;

    (void)Rte_Read_DashboardData(&data);
    frame[0] = (uint8_t)(Com_MapEcuMode(EcuM_GetState()) & 0x07u);
    frame[0] |= (uint8_t)(1u << 3u);  /* DisplayPage=Main。 */
    frame[1] = 0u;
    if (data.buzzer_muted != 0u)
    {
        frame[1] = 2u;
    }
    else if (data.alarm_active != 0u)
    {
        frame[1] = 1u;
    }

    backlight = 0xFFu;
    (void)Rte_Read_BacklightLevel(&backlight);
    frame[2] = backlight;
    Com_WriteLeU16(&frame[3], 0xFFFFu);  /* TripDistance 尚无可靠数据源。 */
    if ((tick_ms / 60000u) > 0xFFFFu)
    {
        drive_minutes = 0xFFFFu;
    }
    else
    {
        drive_minutes = (uint16_t)(tick_ms / 60000u);
    }
    Com_WriteLeU16(&frame[5], drive_minutes);
    frame[7] = (data.shutdown_request != 0u) ? 0x01u : 0u;
    if ((data.warning_flags != 0u) ||
        (data.alarm_active != 0u) ||
        (Dem_GetConfirmedDtcCount() != 0u))
    {
        frame[7] |= 0x02u;
    }

    (void)CanIf_Transmit(CAN_ID_ICM_STATUS_100MS, frame, 8u);
}

static void Com_SendDiagStatus(uint32_t tick_ms)
{
    uint8_t frame[8];
    uint16_t last_fault_id;
    uint16_t power_on_count;
    uint16_t bus_off_count;
    uint8_t sdram_ok;
    uint8_t lcd_ok;
    uint8_t rtc_ok;
    uint8_t eeprom_ok;
    uint8_t tf_ok;
    uint8_t can_ok;
    uint8_t self_test_ok;
    const NvM_BootInfoType *boot_info;

    if (tick_ms < Com_NextDiagStatusTxMs)
    {
        return;
    }
    Com_NextDiagStatusTxMs = tick_ms + 1000u;

    boot_info = NvM_GetBootInfo();
    last_fault_id = Dem_GetLastFaultId();
    power_on_count = (boot_info->boot_counter > 0xFFFFu) ?
                     0xFFFFu : (uint16_t)boot_info->boot_counter;

    frame[0] = Dem_GetConfirmedDtcCount();
    Com_WriteLeU16(&frame[1], last_fault_id);
    Com_WriteLeU16(&frame[3], power_on_count);

    sdram_ok = SdramIf_IsReady();
    lcd_ok = LcdIf_IsReady();
    rtc_ok = ((Dem_IsEventFailed(DEM_EVENT_RTC_COMM_FAILED) == 0u) &&
              (Dem_IsEventFailed(DEM_EVENT_RTC_TIME_INVALID) == 0u)) ? 1u : 0u;
    eeprom_ok = NvM_IsInitialized();
    tf_ok = FatFsIf_IsMounted();
    can_ok = (CanSM_GetState() == CANSM_STATE_FULL_COMM) ? 1u : 0u;
    frame[5] = 0u;
    frame[5] |= (sdram_ok != 0u) ? 0x01u : 0u;
    frame[5] |= (lcd_ok != 0u) ? 0x02u : 0u;
    frame[5] |= (rtc_ok != 0u) ? 0x04u : 0u;
    frame[5] |= (eeprom_ok != 0u) ? 0x08u : 0u;
    frame[5] |= (tf_ok != 0u) ? 0x10u : 0u;
    frame[5] |= (can_ok != 0u) ? 0x20u : 0u;

    bus_off_count = Dem_GetOccurrenceCounter(DEM_EVENT_CAN_BUS_OFF);
    frame[6] = (bus_off_count > 255u) ? 0xFFu : (uint8_t)bus_off_count;

    frame[7] = (uint8_t)(boot_info->last_reset_reason & 0x0Fu);
    frame[7] |= (uint8_t)(Com_MapDiagnosticSession(Dcm_GetCurrentSession()) << 4u);
    frame[7] |= (Dem_IsDirty() != 0u) ? 0x40u : 0u;
    /*
     * TF/FatFs 是可选日志能力，不阻断关键自检汇总；其独立状态仍在 B5.4。
     */
    self_test_ok = ((sdram_ok != 0u) && (lcd_ok != 0u) &&
                    (rtc_ok != 0u) && (eeprom_ok != 0u) &&
                    (can_ok != 0u)) ? 1u : 0u;
    frame[7] |= (self_test_ok != 0u) ? 0x80u : 0u;

    (void)CanIf_Transmit(CAN_ID_ICM_DIAGSTATUS_1000MS, frame, 8u);
}

static void Com_SendNm(uint32_t tick_ms)
{
    uint8_t frame[8];

    if (tick_ms < Com_NextNmTxMs)
    {
        return;
    }
    Com_NextNmTxMs = tick_ms + 1000u;

    frame[0] = CAN_CFG_ICM_NODE_ADDRESS;
    frame[1] = 0x00u;
    frame[2] = 0x04u;  /* RepeatSts=0，NMState=2 NormalOperation。 */
    frame[3] = 0u;
    frame[4] = 0u;
    frame[5] = 0u;
    frame[6] = 0u;
    frame[7] = 0u;

    (void)CanIf_Transmit(CAN_ID_NM_AUTOSAR_ICM, frame, 8u);
}

static void Com_SendUserInputEvent(void)
{
    uint8_t frame[8];

    if (Com_UserInputPending == 0u)
    {
        if (Rte_Take_UserInputEvent(&Com_PendingUserInput) != E_OK)
        {
            return;
        }
        Com_UserInputPending = 1u;
    }

    frame[0] = Com_PendingUserInput.key_code;
    frame[1] = (uint8_t)(Com_PendingUserInput.key_action & 0x0Fu);
    frame[2] = Com_PendingUserInput.event_counter;
    frame[3] = 0u;
    if (Com_PendingUserInput.power_key_long_press != 0u)
    {
        frame[3] |= 0x01u;
    }
    if (Com_PendingUserInput.shutdown_confirm != 0u)
    {
        frame[3] |= 0x02u;
    }
    frame[4] = 0u;
    frame[5] = 0u;
    frame[6] = 0u;
    frame[7] = 0u;

    if (CanIf_Transmit(CAN_ID_ICM_USERINPUT_EVENT, frame, 8u) == E_OK)
    {
        /* 只有成功交给 CanIf 后才消费；邮箱忙/总线异常时下周期重试。 */
        Com_UserInputPending = 0u;
    }
}

static void Com_SendLogStatus(uint32_t tick_ms)
{
    uint8_t frame[8];

    if (tick_ms < Com_NextLogStatusTxMs)
    {
        return;
    }
    Com_NextLogStatusTxMs = tick_ms + 1000u;

    /*
     * 0x328 ICM_LogStatus_1000ms：
     * 当前工程还没有 TF 卡日志文件系统，先上报“未挂载、串口日志使能、无写错误”
     * 和运行秒数。后续接入 FatFs 日志后只替换这些来源。
     */
    frame[0] = 0x04u;  /* bit0-1 SdCardStatus=0 NotMounted，bit2 LogEnable=1。 */
    Com_WriteLeU16(&frame[1], 0u);
    frame[3] = 0u;
    Com_WriteLeU32(&frame[4], tick_ms / 1000u);

    (void)CanIf_Transmit(CAN_ID_ICM_LOGSTATUS_1000MS, frame, 8u);
}

void Com_MainFunction(uint32_t tick_ms)
{
    Rte_DashboardDataType data;

    Rte_Update_CanValidity(tick_ms);
    (void)Rte_Read_DashboardData(&data);

    /*
     * 只有曾经收到过动力域报文，且当前不在模拟模式时，才上报 CAN_RX_TIMEOUT。
     * 这样刚上电还没接 CAN 或用户主动开模拟时，不会误报动力 CAN 丢失。
     */
    if ((Com_PowertrainSeen != 0u) &&
        (data.can_ems_valid == 0u) &&
        (data.simulation_mode == 0u))
    {
        Dem_SetEventStatus(DEM_EVENT_CAN_RX_TIMEOUT, DEM_EVENT_STATUS_FAILED);
    }

    Com_SendUserInputEvent();
    Com_SendIcmStatus(tick_ms);
    Com_SendDiagStatus(tick_ms);
    Com_SendNm(tick_ms);
    Com_SendLogStatus(tick_ms);
}
