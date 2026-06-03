#ifndef NVM_H
#define NVM_H

#include "Std_Types.h"

#include <stdint.h>

typedef enum
{
    NVM_BLOCK_BOOT_INFO = 0u,
    NVM_BLOCK_SYSTEM_CONFIG,
    NVM_BLOCK_DEM_STATUS
} NvM_BlockIdType;

typedef struct
{
    uint32_t boot_counter;
    uint8_t last_reset_reason;
    uint8_t reserved[3];
} NvM_BootInfoType;

typedef struct
{
    uint8_t backlight_level;
    uint8_t buzzer_enable;
    uint8_t theme;
    uint8_t reserved;
} NvM_SystemConfigType;

void NvM_Init(void);
void NvM_MainFunction(uint32_t tick_ms);
Std_ReturnType NvM_ReadBlock(NvM_BlockIdType block_id, uint8_t *data, uint16_t length);
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType block_id, const uint8_t *data, uint16_t length);
const NvM_BootInfoType *NvM_GetBootInfo(void);
const NvM_SystemConfigType *NvM_GetSystemConfig(void);
Std_ReturnType NvM_SetSystemConfig(const NvM_SystemConfigType *config);
void NvM_WriteAll(void);

#endif /* NVM_H */
#ifndef NVM_H
#define NVM_H

#include "Std_Types.h"

#include <stdint.h>

typedef struct
{
    uint8_t theme;
    uint8_t backlight_level;
    uint8_t buzzer_enable;
    uint8_t can_baudrate_index;
} NvM_SystemConfigType;

void NvM_Init(void);
void NvM_MainFunction(uint16_t elapsed_ms);
Std_ReturnType NvM_WriteAll(void);
void NvM_MarkDemDirty(void);
void NvM_MarkSystemConfigDirty(void);
uint16_t NvM_GetBootCounter(void);
const NvM_SystemConfigType *NvM_GetSystemConfig(void);
void NvM_SetSystemConfig(const NvM_SystemConfigType *config);

#endif /* NVM_H */
