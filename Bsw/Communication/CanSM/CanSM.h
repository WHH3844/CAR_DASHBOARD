#ifndef CANSM_H
#define CANSM_H

#include <stdint.h>

typedef enum
{
    CANSM_STATE_NO_COMM = 0u,
    CANSM_STATE_FULL_COMM = 1u,
    CANSM_STATE_BUS_OFF = 2u
} CanSM_StateType;

void CanSM_Init(void);
void CanSM_MainFunction(uint32_t tick_ms);
CanSM_StateType CanSM_GetState(void);

#endif /* CANSM_H */
#ifndef CANSM_H
#define CANSM_H

#include "Std_Types.h"

typedef enum
{
    CANSM_NO_COMM = 0u,
    CANSM_FULL_COMM = 1u
} CanSM_StateType;

void CanSM_Init(void);
void CanSM_MainFunction(void);
CanSM_StateType CanSM_GetState(void);

#endif /* CANSM_H */
