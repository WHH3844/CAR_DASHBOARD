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
    /* CAN 矩阵中的多字节业务信号按 little-endian 打包。 */
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
        /* 未配置的 CAN ID 静默丢弃，避免未知帧影响仪表状态。 */
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
    /*
     * 0x325 ICM_Status_100ms 是仪表对外状态镜像。
     * Byte0-3 直接放 RTE 物理值，Byte4 用 bit 表示报警/静音/模拟模式。
     */
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
    /*
     * 0x326 提供一个轻量诊断摘要：
     * DTC 数量、当前会话、动力 CAN 是否有效，其余字节预留。
     */
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

    Com_SendIcmStatus(tick_ms);
    Com_SendDiagStatus(tick_ms);
    Com_SendNm(tick_ms);
}
