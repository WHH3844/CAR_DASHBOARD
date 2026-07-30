#ifndef DCM_H
#define DCM_H

#include "Std_Types.h"

#include <stdint.h>

/*
 * Diagnostic Communication Manager 简化实现。
 *
 * 支持的 UDS 子集包括会话控制、读 DID、读/清 DTC 和 TesterPresent。
 * 诊断传输只走 CanTp 单帧，因此本层响应长度也保持在单帧范围内。
 */
void Dcm_Init(void);

/* 周期维护诊断会话超时，超过 S3Server 后自动回默认会话。 */
void Dcm_MainFunction(uint32_t tick_ms);

/*
 * 接收一条已经去掉 ISO-TP PCI 的 UDS 请求。
 *
 * payload[0] 是 SID，length 是 UDS payload 长度；rx_can_id 用于执行
 * 物理/功能寻址权限白名单。
 */
void Dcm_RxRequest(const uint8_t *payload, uint8_t length, uint16_t rx_can_id, uint32_t tick_ms);

/* 返回当前诊断会话，供 Com 诊断状态报文上报。 */
uint8_t Dcm_GetCurrentSession(void);

#endif /* DCM_H */
