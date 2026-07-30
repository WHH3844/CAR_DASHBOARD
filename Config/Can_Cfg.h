#ifndef CAN_CFG_H
#define CAN_CFG_H

#include <stdint.h>

/*
 * CAN 矩阵基线：
 * - Classic CAN 标准帧 11-bit ID
 * - DLC 固定按 8 字节处理
 * - 波特率 500K，已通过 PCAN-View 板级测试
 */
#define CAN_CFG_BAUDRATE                       500000u
#define CAN_CFG_DLC                            8u
#define CAN_CFG_TX_TIMEOUT_LOOP                200000u

/* 网络管理报文，第一版只做教学级状态上报，不实现完整 AUTOSAR NM。 */
#define CAN_ID_NM_AUTOSAR_ICM                  0x440u
#define CAN_ID_NM_AUTOSAR_SIM                  0x441u

/* 诊断 CAN ID，来自诊断需求矩阵 01.General / 13.CAN_Diag。 */
#define CAN_ID_DIAG_FUNCTIONAL_REQ             0x7DFu
#define CAN_ID_DIAG_PHYSICAL_REQ               0x700u
#define CAN_ID_DIAG_PHYSICAL_RESP              0x708u

/* 仪表业务报文，来自 CAN 报文矩阵 MsgSummary。 */
#define CAN_ID_EMS_POWERTRAIN_20MS             0x321u
#define CAN_ID_BCM_BODYSTATUS_100MS            0x322u
#define CAN_ID_TPMS_STATUS_1000MS              0x323u
#define CAN_ID_IHU_CGW_CONFIG_500MS            0x324u
#define CAN_ID_ICM_STATUS_100MS                0x325u
#define CAN_ID_ICM_DIAGSTATUS_1000MS           0x326u
#define CAN_ID_ICM_USERINPUT_EVENT             0x327u
#define CAN_ID_ICM_LOGSTATUS_1000MS            0x328u
#define CAN_ID_CDM_STATUS_500MS                 0x329u

/* v1.0 接口版本和节点地址。0x329 Byte0 使用 0x10 表示 v1.0。 */
#define CAN_CFG_INTERFACE_VERSION_V1_0         0x10u
#define CAN_CFG_ICM_NODE_ADDRESS               0x40u
#define CAN_CFG_CDM_NODE_ADDRESS               0x41u

/* 接收报文 freshness 阈值。关键动力报文 0x321 漏 5 帧后即判无效。 */
#define CAN_CFG_EMS_POWERTRAIN_TIMEOUT_MS      100u
#define CAN_CFG_BCM_BODYSTATUS_TIMEOUT_MS      500u
#define CAN_CFG_TPMS_STATUS_TIMEOUT_MS         3000u
#define CAN_CFG_CONFIG_TIMEOUT_MS              3000u
#define CAN_CFG_CDM_STATUS_TIMEOUT_MS          1500u
#define CAN_CFG_CDM_NM_TIMEOUT_MS              3000u
#define CAN_CFG_CDM_ALIVE_STALL_FRAMES         3u

#endif /* CAN_CFG_H */
