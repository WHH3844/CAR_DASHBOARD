#include "CanTp.h"

#include "Can_Cfg.h"
#include "Dcm.h"

void CanTp_Init(void)
{
    /*
     * 当前 CanTp 无内部队列和多帧状态机，所以初始化为空。
     * 保留入口是为了 EcuM 初始化链完整，后续添加多帧时不用改调用方。
     */
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
        /*
         * 标准单帧：Byte0 高 4bit 为 0，低 4bit 表示 payload 长度。
         * 这里用 <=7 且长度不超过 DLC 判断，能覆盖 02 10 03 这类常见请求。
         */
        length = pdu->data[0];
        for (index = 0u; index < length; index++)
        {
            payload[index] = pdu->data[index + 1u];
        }
    }
    else
    {
        /*
         * 裸 UDS 兼容路径：把整个 CAN 数据区都视为 UDS payload。
         * 便于用简单 USB-CAN 工具直接发送 10 03、22 F1 80 等字节。
         */
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
    /*
     * 响应统一按 ISO-TP 单帧发出，Byte0 是长度，Byte1 开始是 UDS 响应。
     * 尾部补 0 可以让 CAN 报文固定 8 字节，方便 PC 工具观察。
     */
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
