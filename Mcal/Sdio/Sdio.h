#ifndef SDIO_H
#define SDIO_H

#include <stdint.h>

typedef enum
{
    SDIO_CARD_TYPE_UNKNOWN = 0u,
    SDIO_CARD_TYPE_SDSC = 1u,
    SDIO_CARD_TYPE_SDHC_SDXC = 2u
} Sdio_CardType;

typedef enum
{
    SDIO_STATUS_OK = 0u,
    SDIO_STATUS_TIMEOUT = 1u,
    SDIO_STATUS_CMD_TIMEOUT = 2u,
    SDIO_STATUS_CMD_CRC = 3u,
    SDIO_STATUS_UNSUPPORTED = 4u,
    SDIO_STATUS_NOT_READY = 5u,
    SDIO_STATUS_READ_ERROR = 6u,
    SDIO_STATUS_PARAM = 7u
} Sdio_StatusType;

typedef struct
{
    Sdio_CardType card_type;
    uint16_t rca;
    uint32_t ocr;
    uint32_t csd[4];
    uint32_t cid[4];
    uint32_t block_count;
    uint32_t block_size;
} Sdio_CardInfoType;

void Sdio_InitPinsAndClock(void);
Sdio_StatusType Sdio_CardInit(void);
Sdio_StatusType Sdio_ReadBlock(uint32_t block_number, uint8_t *buffer);
const Sdio_CardInfoType *Sdio_GetCardInfo(void);
Sdio_StatusType Sdio_GetLastStatus(void);
uint32_t Sdio_GetLastCommand(void);
uint32_t Sdio_GetLastStatusRegister(void);

#endif /* SDIO_H */
