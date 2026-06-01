#ifndef SDIF_H
#define SDIF_H

#include "Sdio.h"

#include <stdint.h>

Sdio_StatusType SdIf_Init(void);
uint8_t SdIf_ReadBlock0(uint8_t *buffer);
const Sdio_CardInfoType *SdIf_GetCardInfo(void);
Sdio_StatusType SdIf_GetLastStatus(void);
uint32_t SdIf_GetLastCommand(void);
uint32_t SdIf_GetLastStatusRegister(void);

#endif /* SDIF_H */
