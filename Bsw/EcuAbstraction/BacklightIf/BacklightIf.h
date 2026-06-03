#ifndef BACKLIGHTIF_H
#define BACKLIGHTIF_H

#include <stdint.h>

/*
 * LCD 背光抽象接口。
 *
 * 上层统一用 0~100 的亮度百分比表达背光需求；当前硬件阶段只实现开关，
 * 未来改为 PWM 调光时，App_Display 和 NvM 配置不用跟着改。
 */
void BacklightIf_Init(void);

/* 设置背光等级，超过 100 的输入会在实现中钳位到 100。 */
void BacklightIf_SetLevel(uint8_t level);

/* 返回最近一次设置的逻辑亮度等级，不直接读取硬件引脚。 */
uint8_t BacklightIf_GetLevel(void);

#endif /* BACKLIGHTIF_H */
