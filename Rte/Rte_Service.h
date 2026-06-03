#ifndef RTE_SERVICE_H
#define RTE_SERVICE_H

#include "Dem.h"
#include "NvM.h"
#include "Std_Types.h"

/*
 * RTE 服务调用封装。
 *
 * 应用层通过这些接口访问 NvM/Dem/蜂鸣器/背光，避免直接包含过多 BSW 头文件。
 * 这样后续替换硬件抽象或增加调用保护时，优先改 Rte_Service.c。
 */
Std_ReturnType Rte_Call_NvM_SetSystemConfig(const NvM_SystemConfigType *config);
const NvM_SystemConfigType *Rte_Call_NvM_GetSystemConfig(void);
void Rte_Call_Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status);

/* 蜂鸣器 on 使用 0/1 语义，非 0 由 BuzzerIf 视为打开。 */
void Rte_Call_Buzzer_Set(uint8_t on);

/* 背光 level 使用 0~100 百分比语义，实际 PWM/GPIO 策略由 BacklightIf 决定。 */
void Rte_Call_Backlight_Set(uint8_t level);

#endif /* RTE_SERVICE_H */
