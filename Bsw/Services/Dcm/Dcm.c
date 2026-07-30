#include "Dcm.h"

#include "CanTp.h"
#include "Can_Cfg.h"
#include "Dcm_Cfg.h"
#include "Dem.h"
#include "NvM.h"
#include "Rte_Signal.h"

#define DCM_SID_DIAGNOSTIC_SESSION_CONTROL     0x10u
#define DCM_SID_ECU_RESET                      0x11u
#define DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION   0x14u
#define DCM_SID_READ_DTC_INFORMATION           0x19u
#define DCM_SID_READ_DATA_BY_IDENTIFIER        0x22u
#define DCM_SID_TESTER_PRESENT                 0x3Eu

#define DCM_NRC_GENERAL_REJECT                 0x10u
#define DCM_NRC_SERVICE_NOT_SUPPORTED          0x11u
#define DCM_NRC_SUBFUNCTION_NOT_SUPPORTED      0x12u
#define DCM_NRC_INCORRECT_LENGTH               0x13u
#define DCM_NRC_RESPONSE_TOO_LONG              0x14u
#define DCM_NRC_CONDITIONS_NOT_CORRECT         0x22u
#define DCM_NRC_REQUEST_OUT_OF_RANGE           0x31u

static uint8_t Dcm_CurrentSession;
static uint32_t Dcm_LastTesterActivityMs;

static void Dcm_SendPositive(const uint8_t *payload, uint8_t length)
{
    /* Dcm 不直接发 CAN，统一交给 CanTp 添加单帧 PCI 和响应 CAN ID。 */
    (void)CanTp_TransmitResponse(payload, length);
}

static void Dcm_SendNegative(uint8_t sid, uint8_t nrc)
{
    uint8_t response[3];

    response[0] = 0x7Fu;
    response[1] = sid;
    response[2] = nrc;
    (void)CanTp_TransmitResponse(response, sizeof(response));
}

static uint16_t Dcm_ReadU16(const uint8_t *data)
{
    /* UDS DID 在请求中按 big-endian 编码，例如 22 F1 80。 */
    return (uint16_t)(((uint16_t)data[0] << 8u) | data[1]);
}

static void Dcm_WriteBeU16(uint8_t *data, uint16_t value)
{
    /* UDS 多字节数值统一使用网络序/Big-Endian。 */
    data[0] = (uint8_t)((value >> 8u) & 0xFFu);
    data[1] = (uint8_t)(value & 0xFFu);
}

static void Dcm_WriteBeU32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)((value >> 24u) & 0xFFu);
    data[1] = (uint8_t)((value >> 16u) & 0xFFu);
    data[2] = (uint8_t)((value >> 8u) & 0xFFu);
    data[3] = (uint8_t)(value & 0xFFu);
}

static void Dcm_HandleSessionControl(const uint8_t *payload, uint8_t length)
{
    uint8_t session;
    uint8_t suppress_positive;
    uint8_t response[6];

    if (length != 2u)
    {
        Dcm_SendNegative(DCM_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    suppress_positive = (payload[1] & 0x80u) ? 1u : 0u;
    session = (uint8_t)(payload[1] & 0x7Fu);

    /*
     * bit7 是 suppressPosRspMsgIndicationBit，低 7bit 才是真正的 session。
     * 第一版只支持默认会话和扩展会话，编程会话暂不开放。
     */
    if ((session != DCM_SESSION_DEFAULT) && (session != DCM_SESSION_EXTENDED))
    {
        Dcm_SendNegative(DCM_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_NRC_SUBFUNCTION_NOT_SUPPORTED);
        return;
    }

    Dcm_CurrentSession = session;
    if (suppress_positive != 0u)
    {
        return;
    }

    response[0] = 0x50u;
    response[1] = session;
    /* 正响应附带 P2/P2*，方便诊断工具显示服务器时间参数。 */
    response[2] = 0x00u;
    response[3] = DCM_CFG_P2_SERVER_MAX_MS;
    response[4] = (uint8_t)((DCM_CFG_P2STAR_SERVER_MAX_MS >> 8u) & 0xFFu);
    response[5] = (uint8_t)(DCM_CFG_P2STAR_SERVER_MAX_MS & 0xFFu);
    Dcm_SendPositive(response, sizeof(response));
}

static void Dcm_PositiveDidHeader(uint8_t *response, uint16_t did)
{
    response[0] = 0x62u;
    response[1] = (uint8_t)((did >> 8u) & 0xFFu);
    response[2] = (uint8_t)(did & 0xFFu);
}

static void Dcm_HandleReadDid(const uint8_t *payload, uint8_t length)
{
    uint8_t response[7];
    uint16_t did;
    Rte_DashboardDataType dashboard;
    RtcIf_TimeType rtc_time;
    uint8_t rtc_valid;
    const NvM_BootInfoType *boot_info;

    if (length != 3u)
    {
        Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    did = Dcm_ReadU16(&payload[1]);
    Dcm_PositiveDidHeader(response, did);
    (void)Rte_Read_DashboardData(&dashboard);

    /*
     * 所有 DID 响应都控制在 7 字节以内，确保 CanTp 单帧足够。
     * 如果后续 DID 需要更长数据，应先扩展 CanTp 多帧。
     */
    switch (did)
    {
    case DCM_DID_BOOT_COUNTER:
        boot_info = NvM_GetBootInfo();
        Dcm_WriteBeU32(&response[3], boot_info->boot_counter);
        Dcm_SendPositive(response, 7u);
        break;

    case DCM_DID_VEHICLE_SPEED:
        if (dashboard.vehicle_speed_valid == 0u)
        {
            Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_CONDITIONS_NOT_CORRECT);
            break;
        }
        Dcm_WriteBeU16(&response[3], dashboard.vehicle_speed_kph_x10);
        Dcm_SendPositive(response, 5u);
        break;

    case DCM_DID_ENGINE_RPM:
        if (dashboard.engine_rpm_valid == 0u)
        {
            Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_CONDITIONS_NOT_CORRECT);
            break;
        }
        Dcm_WriteBeU16(&response[3], dashboard.engine_rpm);
        Dcm_SendPositive(response, 5u);
        break;

    case DCM_DID_BATTERY_VOLTAGE:
        if (dashboard.battery_voltage_valid == 0u)
        {
            Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_CONDITIONS_NOT_CORRECT);
            break;
        }
        Dcm_WriteBeU16(&response[3], dashboard.battery_mv);
        Dcm_SendPositive(response, 5u);
        break;

    case DCM_DID_RTC_TIME:
        (void)Rte_Read_RtcTime(&rtc_time, &rtc_valid);
        if (rtc_valid == 0u)
        {
            Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_CONDITIONS_NOT_CORRECT);
            break;
        }
        /*
         * 单帧响应可用 4 字节数据：year(BE)、month、date。
         * 完整年月日时分秒需先实现 ISO-TP 多帧。
         */
        Dcm_WriteBeU16(&response[3], rtc_time.year);
        response[5] = rtc_time.month;
        response[6] = rtc_time.date;
        Dcm_SendPositive(response, 7u);
        break;

    case DCM_DID_SDRAM_TEST_RESULT:
        response[3] = 1u;
        Dcm_SendPositive(response, 4u);
        break;

    case DCM_DID_HW_VERSION:
        response[3] = 'A';
        response[4] = '0';
        response[5] = '0';
        response[6] = '1';
        Dcm_SendPositive(response, 7u);
        break;

    case DCM_DID_SW_VERSION:
        response[3] = '0';
        response[4] = '1';
        response[5] = '0';
        response[6] = '0';
        Dcm_SendPositive(response, 7u);
        break;

    default:
        Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_REQUEST_OUT_OF_RANGE);
        break;
    }
}

static void Dcm_HandleReadDtc(const uint8_t *payload, uint8_t length)
{
    uint8_t response[7];
    uint8_t subfunction;
    uint8_t status_mask;
    Dem_DtcRecordType record;

    if (length < 3u)
    {
        Dcm_SendNegative(DCM_SID_READ_DTC_INFORMATION, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    subfunction = payload[1];
    status_mask = payload[2];

    if (subfunction == 0x01u)
    {
        /* reportNumberOfDTCByStatusMask：返回匹配 status_mask 的 DTC 数量。 */
        response[0] = 0x59u;
        response[1] = 0x01u;
        response[2] = status_mask;
        response[3] = 0x00u;
        response[4] = Dem_GetDtcCountByStatusMask(status_mask);
        Dcm_SendPositive(response, 5u);
    }
    else if (subfunction == 0x02u)
    {
        /*
         * reportDTCByStatusMask：单帧空间有限，第一版只返回第一个匹配 DTC。
         * 后续扩展多帧后可以返回完整 DTC 列表。
         */
        response[0] = 0x59u;
        response[1] = 0x02u;
        response[2] = status_mask;

        if (Dem_GetFirstDtcByStatusMask(status_mask, &record) == E_OK)
        {
            response[3] = (uint8_t)((record.dtc >> 16u) & 0xFFu);
            response[4] = (uint8_t)((record.dtc >> 8u) & 0xFFu);
            response[5] = (uint8_t)(record.dtc & 0xFFu);
            response[6] = record.status;
            Dcm_SendPositive(response, 7u);
        }
        else
        {
            Dcm_SendPositive(response, 3u);
        }
    }
    else
    {
        Dcm_SendNegative(DCM_SID_READ_DTC_INFORMATION, DCM_NRC_SUBFUNCTION_NOT_SUPPORTED);
    }
}

static void Dcm_HandleClearDtc(const uint8_t *payload, uint8_t length)
{
    if (length != 4u)
    {
        Dcm_SendNegative(DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    if (Dcm_CurrentSession != DCM_SESSION_EXTENDED)
    {
        /* 清 DTC 要求扩展会话，避免默认会话下误清历史故障。 */
        Dcm_SendNegative(DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION, DCM_NRC_CONDITIONS_NOT_CORRECT);
        return;
    }

    if ((payload[1] != 0xFFu) || (payload[2] != 0xFFu) || (payload[3] != 0xFFu))
    {
        /* 第一版只支持 groupOfDTC=0xFFFFFF，即清除全部 DTC。 */
        Dcm_SendNegative(DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION, DCM_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }

    Dem_ClearAllDtc();
    payload = payload; /* 保持 MISRA 风格：参数已使用，这行不产生实际逻辑。 */
    {
        uint8_t response[1];
        response[0] = 0x54u;
        Dcm_SendPositive(response, sizeof(response));
    }
}

static void Dcm_HandleTesterPresent(const uint8_t *payload, uint8_t length)
{
    uint8_t suppress_positive;
    uint8_t response[2];

    if (length != 2u)
    {
        Dcm_SendNegative(DCM_SID_TESTER_PRESENT, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    suppress_positive = (payload[1] & 0x80u) ? 1u : 0u;
    if (suppress_positive != 0u)
    {
        /* TesterPresent 支持抑制正响应，但仍会刷新 LastTesterActivityMs。 */
        return;
    }

    response[0] = 0x7Eu;
    response[1] = 0x00u;
    Dcm_SendPositive(response, sizeof(response));
}

void Dcm_Init(void)
{
    Dcm_CurrentSession = DCM_SESSION_DEFAULT;
    Dcm_LastTesterActivityMs = 0u;
}

void Dcm_MainFunction(uint32_t tick_ms)
{
    if ((Dcm_CurrentSession != DCM_SESSION_DEFAULT) &&
        ((tick_ms - Dcm_LastTesterActivityMs) > DCM_CFG_S3_SERVER_TIMEOUT_MS))
    {
        /* S3Server 超时后自动退回默认会话，防止扩展会话长期保持。 */
        Dcm_CurrentSession = DCM_SESSION_DEFAULT;
    }
}

void Dcm_RxRequest(const uint8_t *payload, uint8_t length, uint16_t rx_can_id, uint32_t tick_ms)
{
    uint8_t sid;

    if ((payload == 0) || (length == 0u))
    {
        return;
    }

    sid = payload[0];
    Dcm_LastTesterActivityMs = tick_ms;

    /*
     * 功能寻址只允许只读服务和 TesterPresent。会话切换、清 DTC、
     * ECU Reset 以及任何未来写/控制服务都不能通过 0x7DF 改变 ECU 状态。
     */
    if ((rx_can_id == CAN_ID_DIAG_FUNCTIONAL_REQ) &&
        (sid != DCM_SID_READ_DATA_BY_IDENTIFIER) &&
        (sid != DCM_SID_READ_DTC_INFORMATION) &&
        (sid != DCM_SID_TESTER_PRESENT))
    {
        Dcm_SendNegative(sid, DCM_NRC_SERVICE_NOT_SUPPORTED);
        return;
    }

    /*
     * 每条有效请求都刷新 tester activity。
     * 即使服务最终返回 NRC，也说明 tester 仍在线，不能触发 S3 超时。
     */
    switch (sid)
    {
    case DCM_SID_DIAGNOSTIC_SESSION_CONTROL:
        Dcm_HandleSessionControl(payload, length);
        break;

    case DCM_SID_READ_DATA_BY_IDENTIFIER:
        Dcm_HandleReadDid(payload, length);
        break;

    case DCM_SID_READ_DTC_INFORMATION:
        Dcm_HandleReadDtc(payload, length);
        break;

    case DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION:
        Dcm_HandleClearDtc(payload, length);
        break;

    case DCM_SID_TESTER_PRESENT:
        Dcm_HandleTesterPresent(payload, length);
        break;

    case DCM_SID_ECU_RESET:
        Dcm_SendNegative(sid, DCM_NRC_SERVICE_NOT_SUPPORTED);
        break;

    default:
        Dcm_SendNegative(sid, DCM_NRC_SERVICE_NOT_SUPPORTED);
        break;
    }
}

uint8_t Dcm_GetCurrentSession(void)
{
    return Dcm_CurrentSession;
}
