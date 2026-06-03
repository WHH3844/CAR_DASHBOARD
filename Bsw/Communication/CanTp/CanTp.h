#ifndef CANTP_H
#define CANTP_H

#include "CanIf.h"
#include "Std_Types.h"

#include <stdint.h>

/*
 * 诊断传输层最小实现。
 *
 * 当前只支持 ISO-TP 单帧诊断报文，满足 7 字节以内 UDS 请求/响应。
 * 多帧下载、流控、分包重组暂不实现，避免第一版诊断链路复杂化。
 */
void CanTp_Init(void);

/*
 * 接收诊断 CAN 帧并提取 UDS payload。
 *
 * 同时兼容标准单帧格式和 USB-CAN 调试时常用的“裸 UDS”格式，
 * 最终统一转给 Dcm_RxRequest()。
 */
void CanTp_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms);

/*
 * 发送诊断响应。
 *
 * payload 最大 7 字节，函数会自动补 ISO-TP 单帧 PCI 字节和尾部 0。
 */
Std_ReturnType CanTp_TransmitResponse(const uint8_t *payload, uint8_t length);

#endif /* CANTP_H */
