#ifndef EXMC_H
#define EXMC_H

#include <stdint.h>

#define EXMC_SDRAM_BASE_ADDR       ((uint32_t)0xC0000000u)
#define EXMC_SDRAM_SIZE_BYTES      (32u * 1024u * 1024u)

uint8_t Exmc_SdramInit(void);
uint32_t Exmc_SdramBase(void);
uint32_t Exmc_SdramSize(void);

#endif /* EXMC_H */
