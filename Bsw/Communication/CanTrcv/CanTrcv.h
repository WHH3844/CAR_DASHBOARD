#ifndef CANTRCV_H
#define CANTRCV_H

#include "Std_Types.h"

#include <stdint.h>

typedef enum
{
    CANTRCV_MODE_STANDBY = 0u,
    CANTRCV_MODE_NORMAL = 1u
} CanTrcv_ModeType;

void CanTrcv_Init(void);
Std_ReturnType CanTrcv_SetMode(CanTrcv_ModeType mode);
CanTrcv_ModeType CanTrcv_GetMode(void);
uint8_t CanTrcv_IsErrorAsserted(void);

#endif /* CANTRCV_H */
