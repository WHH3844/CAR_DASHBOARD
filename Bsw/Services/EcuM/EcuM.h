#ifndef ECUM_H
#define ECUM_H

#include <stdint.h>

typedef enum
{
    ECUM_STATE_OFF = 0u,
    ECUM_STATE_BOOT,
    ECUM_STATE_SELF_TEST,
    ECUM_STATE_RUN,
    ECUM_STATE_SLEEP_PREPARE,
    ECUM_STATE_FAULT
} EcuM_StateType;

void EcuM_Init(void);
void EcuM_MainFunction(void);
void EcuM_MainLoop(void);
EcuM_StateType EcuM_GetState(void);
uint32_t EcuM_GetTickMs(void);

#endif /* ECUM_H */
#ifndef ECUM_H
#define ECUM_H

#include <stdint.h>

typedef enum
{
    ECUM_STATE_BOOT = 0u,
    ECUM_STATE_SELF_TEST,
    ECUM_STATE_RUN,
    ECUM_STATE_FAULT,
    ECUM_STATE_SLEEP_PREPARE
} EcuM_StateType;

void EcuM_Init(void);
void EcuM_MainFunction(void);
void EcuM_RunSystem10ms(void);
void EcuM_RunCan10ms(void);
void EcuM_RunApp10ms(void);
void EcuM_RunDisplay500ms(void);
void EcuM_RunDiagNvM100ms(void);
EcuM_StateType EcuM_GetState(void);

#endif /* ECUM_H */
