#ifndef MEMIF_H
#define MEMIF_H

#include "Std_Types.h"

#include <stdint.h>

void MemIf_Init(void);
Std_ReturnType MemIf_ReadEep(uint16_t address, uint8_t *data);
Std_ReturnType MemIf_WriteEep(uint16_t address, uint8_t data);

#endif /* MEMIF_H */
