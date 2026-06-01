#ifndef EEP_H
#define EEP_H

#include <stdint.h>

#define EEP_FT24C16A_SIZE_BYTES     2048u

void Eep_Init(void);
uint8_t Eep_WriteByte(uint16_t address, uint8_t data);
uint8_t Eep_ReadByte(uint16_t address, uint8_t *data);

#endif /* EEP_H */
