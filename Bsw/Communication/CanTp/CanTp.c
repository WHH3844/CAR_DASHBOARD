#include "CanTp.h"

#include "Can_Cfg.h"
#include "Dcm.h"

void CanTp_Init(void)
{
}

void CanTp_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    uint8_t payload[8];
    uint8_t length;
    uint8_t index;

    if ((pdu == 0) || (pdu->dlc == 0u))
    {
        return;
    }

    /*
     * 标准 ISO-TP 单帧格式：Byte0 低 4 bit 是 UDS payload 长度，
     * Byte1 开始才是真正的 UDS 服务数据，例如 02 10 03。
     *
     * 为了 USB-CAN 手工调试方便，也兼容“裸 UDS”格式，例如直接发 10 03。
     */
    if ((pdu->data[0] <= 7u) && ((uint8_t)(pdu->data[0] + 1u) <= pdu->dlc))
    {
        length = pdu->data[0];
        for (index = 0u; index < length; index++)
        {
            payload[index] = pdu->data[index + 1u];
        }
    }
    else
    {
        length = pdu->dlc;
        for (index = 0u; index < length; index++)
        {
            payload[index] = pdu->data[index];
        }
    }

    Dcm_RxRequest(payload, length, pdu->id, tick_ms);
}

Std_ReturnType CanTp_TransmitResponse(const uint8_t *payload, uint8_t length)
{
    uint8_t frame[8];
    uint8_t index;

    if ((payload == 0) || (length == 0u) || (length > 7u))
    {
        return E_NOT_OK;
    }

    frame[0] = length;
    for (index = 0u; index < length; index++)
    {
        frame[index + 1u] = payload[index];
    }
    for (index = (uint8_t)(length + 1u); index < 8u; index++)
    {
        frame[index] = 0u;
    }

    return CanIf_Transmit(CAN_ID_DIAG_PHYSICAL_RESP, frame, 8u);
}
#include "CanTp.h"

#include "Dcm.h"
#include "PduR.h"
#include "Can_Cfg.h"

void CanTp_Init(void)
{
    /*
     * 第一版只支持 ISO-TP Single Frame。
     * 多帧 FirstFrame/ConsecutiveFrame 后续做长 DID、DTC 快照、刷写时再实现。
     */
}

void CanTp_RxIndication(const CanIf_PduType *pdu)
{
    uint8_t payload[7];
    uint8_t payload_len;
    uint8_t index;
    uint8_t functional;

    if ((pdu == 0) || (pdu->dlc == 0u))
    {
        return;
    }

    functional = (pdu->id == CAN_ID_DIAG_FUNCTIONAL_REQ) ? 1u : 0u;

    /*
     * 正常 ISO-TP 单帧格式：
     * Byte0 高 4bit = 0，低 4bit = 诊断 payload 长度。
     * 为了 USB-CAN 手工测试方便，也兼容直接发送 10 01 / 22 F1 81 这种裸 UDS。
     */
    if (((pdu->data[0] & 0xF0u) == 0u) &&
        ((pdu->data[0] & 0x0Fu) != 0u) &&
        ((pdu->data[0] & 0x0Fu) <= 7u) &&
        (pdu->dlc >= (uint8_t)((pdu->data[0] & 0x0Fu) + 1u)))
    {
        payload_len = (uint8_t)(pdu->data[0] & 0x0Fu);
        for (index = 0u; index < payload_len; index++)
        {
            payload[index] = pdu->data[index + 1u];
        }
    }
    else
    {
        payload_len = (pdu->dlc > 7u) ? 7u : pdu->dlc;
        for (index = 0u; index < payload_len; index++)
        {
            payload[index] = pdu->data[index];
        }
    }

    Dcm_RxIndication(payload, payload_len, functional);
}

Std_ReturnType CanTp_Transmit(uint16_t response_id, const uint8_t *payload, uint8_t length)
{
    CanIf_PduType pdu;
    uint8_t index;

    if ((payload == 0) || (length == 0u) || (length > 7u))
    {
        return E_NOT_OK;
    }

    pdu.id = response_id;
    pdu.dlc = CAN_CFG_DLC;
    pdu.data[0] = length;

    for (index = 0u; index < length; index++)
    {
        pdu.data[index + 1u] = payload[index];
    }
    for (index = (uint8_t)(length + 1u); index < CAN_CFG_DLC; index++)
    {
        pdu.data[index] = 0u;
    }

    return PduR_CanTpTransmit(&pdu);
}
