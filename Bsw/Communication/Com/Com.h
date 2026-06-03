#ifndef COM_H
#define COM_H

#include "CanIf.h"

#include <stdint.h>

/*
 * AUTOSAR 风格的简化 Com 层。
 *
 * Com 负责 CAN PDU 与 RTE 信号之间的转换：
 * - Rx：解析 0x321/0x322/0x324 等输入报文，写入 RTE。
 * - Tx：周期打包仪表状态、诊断状态和简化 NM 报文，通过 CanIf 发送。
 */
void Com_Init(void);

/* 接收普通业务 CAN 帧。诊断帧已在 PduR 中分流到 CanTp，不会进入这里。 */
void Com_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms);

/*
 * Com 周期任务。
 *
 * 负责输入信号超时判定、CAN_RX_TIMEOUT Dem 事件维护，以及各类状态报文发送节拍。
 */
void Com_MainFunction(uint32_t tick_ms);

#endif /* COM_H */
