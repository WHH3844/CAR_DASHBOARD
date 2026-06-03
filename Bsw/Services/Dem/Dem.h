#ifndef DEM_H
#define DEM_H

#include "Dem_Cfg.h"
#include "Std_Types.h"

#include <stdint.h>

typedef enum
{
    DEM_EVENT_STATUS_PASSED = 0u,
    DEM_EVENT_STATUS_FAILED = 1u
} Dem_EventStatusType;

typedef struct
{
    uint32_t dtc;
    uint8_t status;
    uint16_t occurrence_counter;
} Dem_DtcRecordType;

void Dem_Init(void);
void Dem_MainFunction(uint32_t tick_ms);
void Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status);
uint8_t Dem_GetDtcCountByStatusMask(uint8_t status_mask);
Std_ReturnType Dem_GetFirstDtcByStatusMask(uint8_t status_mask, Dem_DtcRecordType *record);
uint8_t Dem_GetConfirmedDtcCount(void);
void Dem_ClearAllDtc(void);
void Dem_SaveNow(void);

#endif /* DEM_H */
#ifndef DEM_H
#define DEM_H

#include "Dem_Cfg.h"
#include "Std_Types.h"

#include <stdint.h>

typedef enum
{
    DEM_EVENT_STATUS_PASSED = 0u,
    DEM_EVENT_STATUS_FAILED = 1u
} Dem_EventStatusType;

typedef struct
{
    uint32_t dtc;
    uint8_t status;
    uint16_t occurrence_counter;
} Dem_DtcStatusType;

void Dem_Init(void);
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status);
uint8_t Dem_GetEventStatusByte(Dem_EventIdType event_id);
uint8_t Dem_GetFailedDtcCount(uint8_t status_mask);
Std_ReturnType Dem_GetFirstFailedDtc(uint8_t status_mask, Dem_DtcStatusType *dtc_status);
Std_ReturnType Dem_ClearAllDtc(void);
uint8_t Dem_GetNvMData(uint8_t *buffer, uint16_t length);
void Dem_LoadNvMData(const uint8_t *buffer, uint16_t length);

#endif /* DEM_H */
