#ifndef PDUR_H
#define PDUR_H

#include "CanIf.h"

#include <stdint.h>

/*
 * PDU Router 简化层。
 *
 * 这里只按 CAN ID 做路由，不解析信号，不修改 payload：
 * 诊断请求进入 CanTp/Dcm，普通业务帧进入 Com。
 */
void PduR_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms);

#endif /* PDUR_H */
