#include "Com.h"

#include "Can_Cfg.h"
#include "Com_Cfg.h"
#include "Dcm.h"
#include "Dem.h"
#include "Rte_Signal.h"

static uint8_t Com_StatusCounter;
static uint8_t Com_DiagStatusCounter;
static uint8_t Com_NmCounter;
static uint32_t Com_NextStatusTxMs;
static uint32_t Com_NextDiagStatusTxMs;
static uint32_t Com_NextNmTxMs;
static uint8_t Com_PowertrainSeen;

static uint16_t Com_ReadLeU16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static void Com_WriteLeU16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)((value >> 8u) & 0xFFu);
}

void Com_Init(void)
{
    Com_StatusCounter = 0u;
    Com_DiagStatusCounter = 0u;
    Com_NmCounter = 0u;
    Com_NextStatusTxMs = 100u;
    Com_NextDiagStatusTxMs = 1000u;
    Com_NextNmTxMs = 1000u;
    Com_PowertrainSeen = 0u;
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

    if (pdu->dlc < 8u)
    {
        return;
    }

    /*
     * 0x321 EMS_Powertrain_20ms：
     * Byte0-1 VehicleSpeed，Byte2-3 EngineSpeed，其余是油量/水温/外温/电压。
     * 物理值换算集中在 Com_Cfg.h，后续换 DBC 时不用改 APP。
     */
    speed_raw = Com_ReadLeU16(&pdu->data[0]);
    rpm_raw = Com_ReadLeU16(&pdu->data[2]);
    speed_x10 = (uint16_t)(((uint32_t)speed_raw * COM_CFG_SPEED_RAW_TO_X10_NUM) /
                           COM_CFG_SPEED_RAW_TO_X10_DEN);
    rpm = (uint16_t)(((uint32_t)rpm_raw * COM_CFG_RPM_RAW_TO_RPM_NUM) /
                     COM_CFG_RPM_RAW_TO_RPM_DEN);
    fuel_percent = (uint8_t)(((uint16_t)pdu->data[4] * COM_CFG_FUEL_RAW_TO_PERCENT_NUM) /
                             COM_CFG_FUEL_RAW_TO_PERCENT_DEN);
    coolant_c = (int16_t)((int16_t)pdu->data[5] - COM_CFG_TEMP_OFFSET_C);
    outdoor_c = (int16_t)((int16_t)pdu->data[6] - COM_CFG_TEMP_OFFSET_C);
    battery_mv = (uint16_t)((uint16_t)pdu->data[7] * COM_CFG_BATTERY_RAW_TO_MV);

    (void)Rte_Write_Powertrain(speed_x10,
                               rpm,
                               fuel_percent,
                               coolant_c,
                               outdoor_c,
                               battery_mv,
                               tick_ms);
    Com_PowertrainSeen = 1u;
    Dem_SetEventStatus(DEM_EVENT_CAN_RX_TIMEOUT, DEM_EVENT_STATUS_PASSED);
}

static void Com_DecodeBodyStatus(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    uint8_t ignition;
    uint8_t gear;
    uint8_t warning_flags;

    if (pdu->dlc < 3u)
    {
        return;
    }

    /*
     * 0x322 当前先按第一版教学矩阵做简化：
     * Byte0 低 4 bit = 点火状态，高 4 bit = 档位；Byte2 = 报警灯/车身告警位。
     */
    ignition = (uint8_t)(pdu->data[0] & 0x0Fu);
    gear = (uint8_t)((pdu->data[0] >> 4u) & 0x0Fu);
    warning_flags = pdu->data[2];
    (void)Rte_Write_BodyStatus(ignition, gear, warning_flags, tick_ms);
}

static void Com_DecodeConfig(const CanIf_PduType *pdu)
{
    if (pdu->dlc >= 5u)
    {
        /*
         * 0x324 Byte4 暂作为背光百分比。若后续矩阵细化，只需调整这里。
         */
        (void)Rte_Write_BacklightLevel(pdu->data[4]);
    }
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
        /* 第一版 UI 暂不显示胎压，先保留入口，后续可写入 RTE。 */
        break;

    case CAN_ID_IHU_CGW_CONFIG_500MS:
        Com_DecodeConfig(pdu);
        break;

    default:
        break;
    }
}

static void Com_SendIcmStatus(uint32_t tick_ms)
{
    Rte_DashboardDataType data;
    uint8_t frame[8];

    if (tick_ms < Com_NextStatusTxMs)
    {
        return;
    }
    Com_NextStatusTxMs = tick_ms + 100u;

    (void)Rte_Read_DashboardData(&data);
    Com_WriteLeU16(&frame[0], data.vehicle_speed_kph_x10);
    Com_WriteLeU16(&frame[2], data.engine_rpm);
    frame[4] = data.alarm_active;
    if (data.buzzer_muted != 0u)
    {
        frame[4] |= 0x02u;
    }
    if (data.simulation_mode != 0u)
    {
        frame[4] |= 0x04u;
    }
    frame[5] = data.can_ems_valid;
    frame[6] = data.warning_flags;
    frame[7] = Com_StatusCounter++;

    (void)CanIf_Transmit(CAN_ID_ICM_STATUS_100MS, frame, 8u);
}

static void Com_SendDiagStatus(uint32_t tick_ms)
{
    uint8_t frame[8];

    if (tick_ms < Com_NextDiagStatusTxMs)
    {
        return;
    }
    Com_NextDiagStatusTxMs = tick_ms + 1000u;

    frame[0] = Dem_GetConfirmedDtcCount();
    frame[1] = Dcm_GetCurrentSession();
    frame[2] = Rte_IsPowertrainValid();
    frame[3] = 0u;
    frame[4] = 0u;
    frame[5] = 0u;
    frame[6] = 0u;
    frame[7] = Com_DiagStatusCounter++;

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

    frame[0] = 0x28u;  /* 教学用 ICM 节点地址。 */
    frame[1] = 0x00u;
    frame[2] = 0x01u;  /* RUN 状态。 */
    frame[3] = 0u;
    frame[4] = 0u;
    frame[5] = 0u;
    frame[6] = 0u;
    frame[7] = Com_NmCounter++;

    (void)CanIf_Transmit(CAN_ID_NM_AUTOSAR_ICM, frame, 8u);
}

void Com_MainFunction(uint32_t tick_ms)
{
    Rte_DashboardDataType data;

    Rte_Update_CanValidity(tick_ms);
    (void)Rte_Read_DashboardData(&data);

    if ((Com_PowertrainSeen != 0u) &&
        (data.can_ems_valid == 0u) &&
        (data.simulation_mode == 0u))
    {
        Dem_SetEventStatus(DEM_EVENT_CAN_RX_TIMEOUT, DEM_EVENT_STATUS_FAILED);
    }

    Com_SendIcmStatus(tick_ms);
    Com_SendDiagStatus(tick_ms);
    Com_SendNm(tick_ms);
}
#include "Com.h"

#include "Can_Cfg.h"
#include "Com_Cfg.h"
#include "Dem.h"
#include "PduR.h"
#include "Rte_Signal.h"

static uint16_t Com_IcmStatusTimerMs;
static uint16_t Com_IcmDiagTimerMs;
static uint8_t Com_IcmStatusCounter;
static uint8_t Com_IcmDiagCounter;

static uint16_t Com_ReadLe16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static uint16_t Com_ScaleU16(uint16_t raw, uint16_t numerator, uint16_t denominator)
{
    uint32_t value;

    if (denominator == 0u)
    {
        return 0u;
    }

    value = ((uint32_t)raw * numerator) / denominator;
    if (value > 0xFFFFu)
    {
        value = 0xFFFFu;
    }

    return (uint16_t)value;
}

static void Com_RxEmsPowertrain(const CanIf_PduType *pdu)
{
    uint16_t raw_speed;
    uint16_t raw_rpm;
    uint16_t speed_x10;
    uint16_t rpm;
    uint8_t fuel;
    int16_t coolant;
    int16_t outdoor;
    uint16_t battery_mv;

    if (pdu->dlc < 8u)
    {
        return;
    }

    raw_speed = Com_ReadLe16(&pdu->data[0]);
    raw_rpm = Com_ReadLe16(&pdu->data[2]);

    speed_x10 = Com_ScaleU16(raw_speed,
                             COM_CFG_SPEED_RAW_TO_X10_NUM,
                             COM_CFG_SPEED_RAW_TO_X10_DEN);
    rpm = Com_ScaleU16(raw_rpm,
                       COM_CFG_RPM_RAW_TO_RPM_NUM,
                       COM_CFG_RPM_RAW_TO_RPM_DEN);
    fuel = (uint8_t)Com_ScaleU16(pdu->data[4],
                                 COM_CFG_FUEL_RAW_TO_PERCENT_NUM,
                                 COM_CFG_FUEL_RAW_TO_PERCENT_DEN);
    coolant = (int16_t)((int16_t)pdu->data[5] - COM_CFG_TEMP_OFFSET_C);
    outdoor = (int16_t)((int16_t)pdu->data[6] - COM_CFG_TEMP_OFFSET_C);
    battery_mv = (uint16_t)((uint16_t)pdu->data[7] * COM_CFG_BATTERY_RAW_TO_MV);

    (void)Rte_Write_VehicleSpeed(speed_x10);
    (void)Rte_Write_EngineRpm(rpm);
    (void)Rte_Write_FuelPercent(fuel);
    (void)Rte_Write_CoolantTemp(coolant);
    (void)Rte_Write_OutdoorTemp(outdoor);
    (void)Rte_Write_BatteryVoltage(battery_mv);
    Rte_MarkCanEmsReceived();
    (void)Dem_SetEventStatus(DEM_EVENT_CAN_RX_TIMEOUT, DEM_EVENT_STATUS_PASSED);
}

static void Com_RxBcmBodyStatus(const CanIf_PduType *pdu)
{
    uint8_t door_flags;
    uint8_t warning_flags;

    if (pdu->dlc < 8u)
    {
        return;
    }

    /*
     * 0x322 在矩阵中包含大量 bit 信号。
     * 第一版先把常用字段压成 flags，App_Display 再决定怎么展示。
     */
    (void)Rte_Write_IgnitionStatus((uint8_t)(pdu->data[0] & 0x03u));
    (void)Rte_Write_GearPosition((uint8_t)((pdu->data[0] >> 2u) & 0x0Fu));

    door_flags = pdu->data[1];
    warning_flags = (uint8_t)(pdu->data[2] | pdu->data[3]);
    (void)Rte_Write_DoorFlags(door_flags);
    (void)Rte_Write_WarningFlags(warning_flags);
}

static void Com_RxTpmsStatus(const CanIf_PduType *pdu)
{
    (void)pdu;
    /* TPMS 第一版先接收占位，后续 UI 增加四轮胎压再展开解析。 */
}

static void Com_RxConfig(const CanIf_PduType *pdu)
{
    if (pdu->dlc < 2u)
    {
        return;
    }

    (void)Rte_Write_ConfigTheme(pdu->data[0]);
    (void)Rte_Write_BacklightLevel(pdu->data[1]);
}

void Com_Init(void)
{
    Com_IcmStatusTimerMs = 0u;
    Com_IcmDiagTimerMs = 0u;
    Com_IcmStatusCounter = 0u;
    Com_IcmDiagCounter = 0u;
}

void Com_RxIndication(const CanIf_PduType *pdu)
{
    if (pdu == 0)
    {
        return;
    }

    switch (pdu->id)
    {
    case CAN_ID_EMS_POWERTRAIN_20MS:
        Com_RxEmsPowertrain(pdu);
        break;

    case CAN_ID_BCM_BODYSTATUS_100MS:
        Com_RxBcmBodyStatus(pdu);
        break;

    case CAN_ID_TPMS_STATUS_1000MS:
        Com_RxTpmsStatus(pdu);
        break;

    case CAN_ID_IHU_CGW_CONFIG_500MS:
        Com_RxConfig(pdu);
        break;

    default:
        break;
    }
}

void Com_MainFunction(uint16_t elapsed_ms)
{
    uint32_t next_status;
    uint32_t next_diag;

    if (Rte_GetCanEmsAgeMs() > CAN_CFG_EMS_POWERTRAIN_TIMEOUT_MS)
    {
        (void)Dem_SetEventStatus(DEM_EVENT_CAN_RX_TIMEOUT, DEM_EVENT_STATUS_FAILED);
    }

    next_status = (uint32_t)Com_IcmStatusTimerMs + elapsed_ms;
    next_diag = (uint32_t)Com_IcmDiagTimerMs + elapsed_ms;
    Com_IcmStatusTimerMs = (next_status > 1000u) ? 1000u : (uint16_t)next_status;
    Com_IcmDiagTimerMs = (next_diag > 1000u) ? 1000u : (uint16_t)next_diag;

    if (Com_IcmStatusTimerMs >= 100u)
    {
        Com_IcmStatusTimerMs = 0u;
        (void)Com_SendIcmStatus();
    }

    if (Com_IcmDiagTimerMs >= 1000u)
    {
        Com_IcmDiagTimerMs = 0u;
        (void)Com_SendIcmDiagStatus();
    }
}

Std_ReturnType Com_SendIcmStatus(void)
{
    CanIf_PduType pdu;
    Rte_DashboardDataType data;

    if (Rte_Read_DashboardData(&data) != E_OK)
    {
        return E_NOT_OK;
    }

    pdu.id = CAN_ID_ICM_STATUS_100MS;
    pdu.dlc = CAN_CFG_DLC;
    pdu.data[0] = (uint8_t)(data.speed_kph_x10 & 0xFFu);
    pdu.data[1] = (uint8_t)((data.speed_kph_x10 >> 8u) & 0xFFu);
    pdu.data[2] = (uint8_t)(data.engine_rpm & 0xFFu);
    pdu.data[3] = (uint8_t)((data.engine_rpm >> 8u) & 0xFFu);
    pdu.data[4] = data.backlight_level;
    pdu.data[5] = (uint8_t)((data.buzzer_alarm != 0u) ? 0x01u : 0x00u);
    pdu.data[6] = (uint8_t)((data.can_ems_valid != 0u) ? 0x01u : 0x00u);
    pdu.data[7] = Com_IcmStatusCounter;
    Com_IcmStatusCounter++;

    return PduR_ComTransmit(&pdu);
}

Std_ReturnType Com_SendIcmDiagStatus(void)
{
    CanIf_PduType pdu;
    uint8_t failed_count;

    failed_count = Dem_GetFailedDtcCount(0xFFu);

    pdu.id = CAN_ID_ICM_DIAGSTATUS_1000MS;
    pdu.dlc = CAN_CFG_DLC;
    pdu.data[0] = failed_count;
    pdu.data[1] = Dem_GetEventStatusByte(DEM_EVENT_CAN_RX_TIMEOUT);
    pdu.data[2] = Dem_GetEventStatusByte(DEM_EVENT_EEPROM_CRC_FAILED);
    pdu.data[3] = Dem_GetEventStatusByte(DEM_EVENT_RTC_COMM_FAILED);
    pdu.data[4] = Dem_GetEventStatusByte(DEM_EVENT_SHT30_COMM_FAILED);
    pdu.data[5] = Dem_GetEventStatusByte(DEM_EVENT_TF_CARD_MOUNT_FAILED);
    pdu.data[6] = 0u;
    pdu.data[7] = Com_IcmDiagCounter;
    Com_IcmDiagCounter++;

    return PduR_ComTransmit(&pdu);
}
