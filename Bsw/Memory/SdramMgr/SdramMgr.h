#ifndef SDRAMMGR_H
#define SDRAMMGR_H

#include "Std_Types.h"

#include <stdint.h>

Std_ReturnType SdramMgr_Init(void);
uint32_t SdramMgr_GetBase(void);
uint32_t SdramMgr_GetSize(void);
uint32_t SdramMgr_GetFrameBuffer(void);
uint8_t SdramMgr_IsReady(void);

#endif /* SDRAMMGR_H */
#ifndef SDRAMMGR_H
#define SDRAMMGR_H

#include "Std_Types.h"

#include <stdint.h>

Std_ReturnType SdramMgr_Init(void);
uint32_t SdramMgr_GetFrameBuffer(void);
uint32_t SdramMgr_GetSizeBytes(void);
uint8_t SdramMgr_IsReady(void);

#endif /* SDRAMMGR_H */
