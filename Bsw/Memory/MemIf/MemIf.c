#include "MemIf.h"

#include "Eep.h"

void MemIf_Init(void)
{
    Eep_Init();
}

Std_ReturnType MemIf_ReadEep(uint16_t address, uint8_t *data)
{
    return (Eep_ReadByte(address, data) != 0u) ? E_OK : E_NOT_OK;
}

Std_ReturnType MemIf_WriteEep(uint16_t address, uint8_t data)
{
    return (Eep_WriteByte(address, data) != 0u) ? E_OK : E_NOT_OK;
}
