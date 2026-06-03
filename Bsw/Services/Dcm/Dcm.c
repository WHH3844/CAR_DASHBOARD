#include "Dcm.h"

#include "CanTp.h"
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
    return (uint16_t)(((uint16_t)data[0] << 8u) | data[1]);
}

static void Dcm_WriteLeU16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)((value >> 8u) & 0xFFu);
}

static void Dcm_WriteLeU32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)((value >> 8u) & 0xFFu);
    data[2] = (uint8_t)((value >> 16u) & 0xFFu);
    data[3] = (uint8_t)((value >> 24u) & 0xFFu);
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

    switch (did)
    {
    case DCM_DID_BOOT_COUNTER:
        boot_info = NvM_GetBootInfo();
        Dcm_WriteLeU32(&response[3], boot_info->boot_counter);
        Dcm_SendPositive(response, 7u);
        break;

    case DCM_DID_VEHICLE_SPEED:
        Dcm_WriteLeU16(&response[3], dashboard.vehicle_speed_kph_x10);
        Dcm_SendPositive(response, 5u);
        break;

    case DCM_DID_ENGINE_RPM:
        Dcm_WriteLeU16(&response[3], dashboard.engine_rpm);
        Dcm_SendPositive(response, 5u);
        break;

    case DCM_DID_BATTERY_VOLTAGE:
        Dcm_WriteLeU16(&response[3], dashboard.battery_mv);
        Dcm_SendPositive(response, 5u);
        break;

    case DCM_DID_RTC_TIME:
        (void)Rte_Read_RtcTime(&rtc_time, &rtc_valid);
        response[3] = rtc_time.hour;
        response[4] = rtc_time.minute;
        response[5] = rtc_time.second;
        response[6] = rtc_valid;
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
        response[0] = 0x59u;
        response[1] = 0x01u;
        response[2] = status_mask;
        response[3] = 0x00u;
        response[4] = Dem_GetDtcCountByStatusMask(status_mask);
        Dcm_SendPositive(response, 5u);
    }
    else if (subfunction == 0x02u)
    {
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
        Dcm_SendNegative(DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION, DCM_NRC_CONDITIONS_NOT_CORRECT);
        return;
    }

    if ((payload[1] != 0xFFu) || (payload[2] != 0xFFu) || (payload[3] != 0xFFu))
    {
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
        Dcm_CurrentSession = DCM_SESSION_DEFAULT;
    }
}

void Dcm_RxRequest(const uint8_t *payload, uint8_t length, uint16_t rx_can_id, uint32_t tick_ms)
{
    uint8_t sid;

    (void)rx_can_id;

    if ((payload == 0) || (length == 0u))
    {
        return;
    }

    sid = payload[0];
    Dcm_LastTesterActivityMs = tick_ms;

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
#include "Dcm.h"

#include "CanTp.h"
#include "Can_Cfg.h"
#include "Dcm_Cfg.h"
#include "Dem.h"
#include "NvM.h"
#include "Rte_Signal.h"

#define DCM_SID_DIAGNOSTIC_SESSION_CONTROL      0x10u
#define DCM_SID_ECU_RESET                       0x11u
#define DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION    0x14u
#define DCM_SID_READ_DTC_INFORMATION            0x19u
#define DCM_SID_READ_DATA_BY_IDENTIFIER         0x22u
#define DCM_SID_TESTER_PRESENT                  0x3Eu

#define DCM_NRC_SERVICE_NOT_SUPPORTED           0x11u
#define DCM_NRC_SUBFUNCTION_NOT_SUPPORTED       0x12u
#define DCM_NRC_INCORRECT_LENGTH                0x13u
#define DCM_NRC_RESPONSE_TOO_LONG               0x14u
#define DCM_NRC_CONDITIONS_NOT_CORRECT          0x22u
#define DCM_NRC_REQUEST_OUT_OF_RANGE            0x31u

static uint8_t Dcm_CurrentSession;
static uint16_t Dcm_S3TimerMs;

static void Dcm_SendPositive(const uint8_t *payload, uint8_t length)
{
    (void)CanTp_Transmit(CAN_ID_DIAG_PHYSICAL_RESP, payload, length);
}

static void Dcm_SendNegative(uint8_t sid, uint8_t nrc)
{
    uint8_t response[3];

    response[0] = 0x7Fu;
    response[1] = sid;
    response[2] = nrc;
    Dcm_SendPositive(response, sizeof(response));
}

static uint16_t Dcm_ReadDidFromRequest(const uint8_t *request)
{
    return (uint16_t)(((uint16_t)request[1] << 8u) | request[2]);
}

static void Dcm_HandleSessionControl(const uint8_t *request, uint8_t length)
{
    uint8_t response[6];
    uint8_t sub_function;

    if (length != 2u)
    {
        Dcm_SendNegative(DCM_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    sub_function = (uint8_t)(request[1] & 0x7Fu);
    if ((sub_function != DCM_SESSION_DEFAULT) && (sub_function != DCM_SESSION_EXTENDED))
    {
        Dcm_SendNegative(DCM_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_NRC_SUBFUNCTION_NOT_SUPPORTED);
        return;
    }

    Dcm_CurrentSession = sub_function;
    Dcm_S3TimerMs = 0u;

    /*
     * 正响应：0x50 + session + P2/P2*。
     * 这里按教学项目简化，P2* 直接用毫秒数编码，方便抓包阅读。
     */
    response[0] = 0x50u;
    response[1] = sub_function;
    response[2] = (uint8_t)((DCM_CFG_P2_SERVER_MAX_MS >> 8u) & 0xFFu);
    response[3] = (uint8_t)(DCM_CFG_P2_SERVER_MAX_MS & 0xFFu);
    response[4] = (uint8_t)((DCM_CFG_P2STAR_SERVER_MAX_MS >> 8u) & 0xFFu);
    response[5] = (uint8_t)(DCM_CFG_P2STAR_SERVER_MAX_MS & 0xFFu);
    Dcm_SendPositive(response, sizeof(response));
}

static uint8_t Dcm_BuildDidData(uint16_t did, uint8_t *buffer, uint8_t *length)
{
    uint16_t value_u16;
    RtcIf_TimeType time;

    if ((buffer == 0) || (length == 0))
    {
        return 0u;
    }

    switch (did)
    {
    case DCM_DID_BOOT_COUNTER:
        value_u16 = NvM_GetBootCounter();
        buffer[0] = (uint8_t)((value_u16 >> 8u) & 0xFFu);
        buffer[1] = (uint8_t)(value_u16 & 0xFFu);
        *length = 2u;
        return 1u;

    case DCM_DID_VEHICLE_SPEED:
        (void)Rte_Read_VehicleSpeed(&value_u16);
        buffer[0] = (uint8_t)((value_u16 >> 8u) & 0xFFu);
        buffer[1] = (uint8_t)(value_u16 & 0xFFu);
        *length = 2u;
        return 1u;

    case DCM_DID_ENGINE_RPM:
        (void)Rte_Read_EngineRpm(&value_u16);
        buffer[0] = (uint8_t)((value_u16 >> 8u) & 0xFFu);
        buffer[1] = (uint8_t)(value_u16 & 0xFFu);
        *length = 2u;
        return 1u;

    case DCM_DID_BATTERY_VOLTAGE:
        (void)Rte_Read_BatteryVoltage(&value_u16);
        buffer[0] = (uint8_t)((value_u16 >> 8u) & 0xFFu);
        buffer[1] = (uint8_t)(value_u16 & 0xFFu);
        *length = 2u;
        return 1u;

    case DCM_DID_RTC_TIME:
        (void)Rte_Read_RtcTime(&time);
        buffer[0] = time.hour;
        buffer[1] = time.minute;
        buffer[2] = time.second;
        *length = 3u;
        return 1u;

    case DCM_DID_SDRAM_TEST_RESULT:
        buffer[0] = (Dem_GetEventStatusByte(DEM_EVENT_SDRAM_INIT_FAILED) == 0u) ? 1u : 0u;
        *length = 1u;
        return 1u;

    case DCM_DID_HW_VERSION:
        buffer[0] = 1u;
        buffer[1] = 0u;
        *length = 2u;
        return 1u;

    case DCM_DID_SW_VERSION:
        buffer[0] = 0u;
        buffer[1] = 1u;
        buffer[2] = 0u;
        *length = 3u;
        return 1u;

    default:
        break;
    }

    return 0u;
}

static void Dcm_HandleReadDataByIdentifier(const uint8_t *request, uint8_t length)
{
    uint8_t response[7];
    uint8_t data_len;
    uint8_t index;
    uint16_t did;

    if (length != 3u)
    {
        Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    did = Dcm_ReadDidFromRequest(request);
    response[0] = 0x62u;
    response[1] = request[1];
    response[2] = request[2];

    if (Dcm_BuildDidData(did, &response[3], &data_len) == 0u)
    {
        Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }

    if ((uint8_t)(3u + data_len) > sizeof(response))
    {
        Dcm_SendNegative(DCM_SID_READ_DATA_BY_IDENTIFIER, DCM_NRC_RESPONSE_TOO_LONG);
        return;
    }

    for (index = (uint8_t)(3u + data_len); index < sizeof(response); index++)
    {
        response[index] = 0u;
    }

    Dcm_SendPositive(response, (uint8_t)(3u + data_len));
}

static void Dcm_HandleReadDtcInformation(const uint8_t *request, uint8_t length)
{
    uint8_t response[7];
    uint8_t sub_function;
    uint8_t status_mask;
    Dem_DtcStatusType first_dtc;

    if (length != 3u)
    {
        Dcm_SendNegative(DCM_SID_READ_DTC_INFORMATION, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    sub_function = request[1];
    status_mask = request[2];

    if (sub_function == 0x01u)
    {
        response[0] = 0x59u;
        response[1] = 0x01u;
        response[2] = status_mask;
        response[3] = 0x01u; /* DTC format: ISO 14229 style placeholder */
        response[4] = 0x00u;
        response[5] = Dem_GetFailedDtcCount(status_mask);
        Dcm_SendPositive(response, 6u);
        return;
    }

    if (sub_function == 0x02u)
    {
        response[0] = 0x59u;
        response[1] = 0x02u;
        response[2] = status_mask;

        if (Dem_GetFirstFailedDtc(status_mask, &first_dtc) == E_OK)
        {
            response[3] = (uint8_t)((first_dtc.dtc >> 16u) & 0xFFu);
            response[4] = (uint8_t)((first_dtc.dtc >> 8u) & 0xFFu);
            response[5] = (uint8_t)(first_dtc.dtc & 0xFFu);
            response[6] = first_dtc.status;
            Dcm_SendPositive(response, 7u);
        }
        else
        {
            Dcm_SendPositive(response, 3u);
        }
        return;
    }

    Dcm_SendNegative(DCM_SID_READ_DTC_INFORMATION, DCM_NRC_SUBFUNCTION_NOT_SUPPORTED);
}

static void Dcm_HandleClearDiagnosticInformation(const uint8_t *request, uint8_t length)
{
    uint8_t response[1];

    if (length != 4u)
    {
        Dcm_SendNegative(DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    if (Dcm_CurrentSession != DCM_SESSION_EXTENDED)
    {
        Dcm_SendNegative(DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION, DCM_NRC_CONDITIONS_NOT_CORRECT);
        return;
    }

    if ((request[1] != 0xFFu) || (request[2] != 0xFFu) || (request[3] != 0xFFu))
    {
        Dcm_SendNegative(DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION, DCM_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }

    (void)Dem_ClearAllDtc();
    response[0] = 0x54u;
    Dcm_SendPositive(response, sizeof(response));
}

static void Dcm_HandleTesterPresent(const uint8_t *request, uint8_t length)
{
    uint8_t response[2];

    if (length != 2u)
    {
        Dcm_SendNegative(DCM_SID_TESTER_PRESENT, DCM_NRC_INCORRECT_LENGTH);
        return;
    }

    if ((request[1] & 0x7Fu) != 0x00u)
    {
        Dcm_SendNegative(DCM_SID_TESTER_PRESENT, DCM_NRC_SUBFUNCTION_NOT_SUPPORTED);
        return;
    }

    Dcm_S3TimerMs = 0u;
    response[0] = 0x7Eu;
    response[1] = 0x00u;
    Dcm_SendPositive(response, sizeof(response));
}

void Dcm_Init(void)
{
    Dcm_CurrentSession = DCM_SESSION_DEFAULT;
    Dcm_S3TimerMs = 0u;
}

void Dcm_MainFunction(uint16_t elapsed_ms)
{
    if (Dcm_CurrentSession == DCM_SESSION_DEFAULT)
    {
        return;
    }

    Dcm_S3TimerMs = (uint16_t)(Dcm_S3TimerMs + elapsed_ms);
    if (Dcm_S3TimerMs >= DCM_CFG_S3_SERVER_TIMEOUT_MS)
    {
        Dcm_CurrentSession = DCM_SESSION_DEFAULT;
        Dcm_S3TimerMs = 0u;
    }
}

void Dcm_RxIndication(const uint8_t *request, uint8_t length, uint8_t functional)
{
    uint8_t sid;

    (void)functional;

    if ((request == 0) || (length == 0u))
    {
        return;
    }

    sid = request[0];
    Dcm_S3TimerMs = 0u;

    switch (sid)
    {
    case DCM_SID_DIAGNOSTIC_SESSION_CONTROL:
        Dcm_HandleSessionControl(request, length);
        break;

    case DCM_SID_READ_DATA_BY_IDENTIFIER:
        Dcm_HandleReadDataByIdentifier(request, length);
        break;

    case DCM_SID_READ_DTC_INFORMATION:
        Dcm_HandleReadDtcInformation(request, length);
        break;

    case DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION:
        Dcm_HandleClearDiagnosticInformation(request, length);
        break;

    case DCM_SID_TESTER_PRESENT:
        Dcm_HandleTesterPresent(request, length);
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
