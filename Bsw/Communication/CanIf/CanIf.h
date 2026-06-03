#ifndef CANIF_H
#define CANIF_H

#include "Std_Types.h"

#include <stdint.h>

typedef struct
{
    /* 11-bit 标准帧 ID，进入上层前已经屏蔽到 0x7FF 范围。 */
    uint16_t id;
    /* Classic CAN 数据长度，CanIf 会裁剪到 CAN_CFG_DLC。 */
    uint8_t dlc;
    /* 上层统一按 8 字节数组读取，短帧未使用字节由 CanIf 补 0。 */
    uint8_t data[8];
} CanIf_PduType;

typedef struct
{
    /* 接收/发送计数用于 bring-up 时确认总线是否真的在跑。 */
    uint32_t rx_total;
    uint32_t tx_total;
    /* 发送失败计数，常见原因是邮箱忙、总线异常或硬件未初始化。 */
    uint32_t tx_error;
    /* initialized 为 0 时，上层发送会直接返回 E_NOT_OK。 */
    uint8_t initialized;
    /* bus_error 由 CanSM/底层错误计数推导，trcv_error 来自收发器 ERR 引脚。 */
    uint8_t bus_error;
    uint8_t trcv_error;
} CanIf_StatusType;

/*
 * 初始化 CAN 收发器和 CAN1 控制器。
 *
 * 当前项目固定使用 500K 标准帧；失败时会上报 Dem CAN_BUS_OFF 事件，
 * 并保持 initialized=0，防止后续上层误发送。
 */
Std_ReturnType CanIf_Init(void);

/*
 * 发送标准 CAN 帧。
 *
 * data 不能为空；dlc 超过 CAN_CFG_DLC 时会被裁剪，调用方不需要重复保护。
 */
Std_ReturnType CanIf_Transmit(uint16_t can_id, const uint8_t *data, uint8_t dlc);

/*
 * CAN 周期服务。
 *
 * 轮询底层接收 FIFO，过滤扩展帧/远程帧后转为 CanIf_PduType 交给 PduR。
 * tick_ms 透传给上层，用于 Com/Dcm 做超时和诊断会话计时。
 */
void CanIf_MainFunction(uint32_t tick_ms);

/* 拷贝一份当前状态快照，status 为空时静默忽略，便于调试代码安全调用。 */
void CanIf_GetStatus(CanIf_StatusType *status);

/* 返回 CAN 控制器是否已经成功初始化。 */
uint8_t CanIf_IsInitialized(void);

#endif /* CANIF_H */
