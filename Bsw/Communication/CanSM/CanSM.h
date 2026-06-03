#ifndef CANSM_H
#define CANSM_H

#include <stdint.h>

typedef enum
{
    /* CANIF 未成功初始化，或者通信被关闭。 */
    CANSM_STATE_NO_COMM = 0u,
    /* 可以正常收发应用和诊断 CAN 帧。 */
    CANSM_STATE_FULL_COMM = 1u,
    /* 通过错误计数近似识别出的总线异常状态。 */
    CANSM_STATE_BUS_OFF = 2u
} CanSM_StateType;

/* 根据 CanIf 初始化结果建立初始通信状态。 */
void CanSM_Init(void);

/*
 * 周期监控 CAN 控制器错误计数。
 *
 * 第一版没有完整 AUTOSAR CanSM 的恢复状态机，只负责把总线异常映射到 Dem，
 * 便于仪表和诊断先看到可用的故障状态。
 */
void CanSM_MainFunction(uint32_t tick_ms);

/* 读取当前通信状态，供诊断、日志或调试页面使用。 */
CanSM_StateType CanSM_GetState(void);

#endif /* CANSM_H */
