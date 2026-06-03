#include "Rte_Service.h"

#include "BacklightIf.h"
#include "BuzzerIf.h"

Std_ReturnType Rte_Call_NvM_SetSystemConfig(const NvM_SystemConfigType *config)
{
    /* 通过 RTE 转发 NvM 写配置，便于后续在这里增加权限或参数范围检查。 */
    return NvM_SetSystemConfig(config);
}

const NvM_SystemConfigType *Rte_Call_NvM_GetSystemConfig(void)
{
    return NvM_GetSystemConfig();
}

void Rte_Call_Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status)
{
    Dem_SetEventStatus(event_id, status);
}

void Rte_Call_Buzzer_Set(uint8_t on)
{
    /* 蜂鸣器控制集中从这里进入 BuzzerIf，应用层不直接依赖蜂鸣器驱动。 */
    BuzzerIf_Set(on);
}

void Rte_Call_Backlight_Set(uint8_t level)
{
    /* 背光等级语义保持 0~100，具体 GPIO/PWM 策略交给 BacklightIf。 */
    BacklightIf_SetLevel(level);
}
