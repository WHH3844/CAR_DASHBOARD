#include "Rte_Service.h"

#include "BacklightIf.h"
#include "BuzzerIf.h"

Std_ReturnType Rte_Call_NvM_SetSystemConfig(const NvM_SystemConfigType *config)
{
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
    BuzzerIf_Set(on);
}

void Rte_Call_Backlight_Set(uint8_t level)
{
    BacklightIf_SetLevel(level);
}
#include "Rte_Service.h"

#include "Dem.h"
#include "NvM.h"

Std_ReturnType Rte_Call_Dem_SetEventStatus(uint8_t event_id, uint8_t status)
{
    return Dem_SetEventStatus((Dem_EventIdType)event_id, (Dem_EventStatusType)status);
}

Std_ReturnType Rte_Call_NvM_WriteAll(void)
{
    return NvM_WriteAll();
}

uint16_t Rte_Call_NvM_GetBootCounter(void)
{
    return NvM_GetBootCounter();
}
