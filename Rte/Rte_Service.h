#ifndef RTE_SERVICE_H
#define RTE_SERVICE_H

#include "Dem.h"
#include "NvM.h"
#include "Std_Types.h"

Std_ReturnType Rte_Call_NvM_SetSystemConfig(const NvM_SystemConfigType *config);
const NvM_SystemConfigType *Rte_Call_NvM_GetSystemConfig(void);
void Rte_Call_Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status);
void Rte_Call_Buzzer_Set(uint8_t on);
void Rte_Call_Backlight_Set(uint8_t level);

#endif /* RTE_SERVICE_H */
#ifndef RTE_SERVICE_H
#define RTE_SERVICE_H

#include "Std_Types.h"

#include <stdint.h>

Std_ReturnType Rte_Call_Dem_SetEventStatus(uint8_t event_id, uint8_t status);
Std_ReturnType Rte_Call_NvM_WriteAll(void);
uint16_t Rte_Call_NvM_GetBootCounter(void);

#endif /* RTE_SERVICE_H */
