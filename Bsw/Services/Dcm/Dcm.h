#ifndef DCM_H
#define DCM_H

#include "Std_Types.h"

#include <stdint.h>

void Dcm_Init(void);
void Dcm_MainFunction(uint32_t tick_ms);
void Dcm_RxRequest(const uint8_t *payload, uint8_t length, uint16_t rx_can_id, uint32_t tick_ms);
uint8_t Dcm_GetCurrentSession(void);

#endif /* DCM_H */
#ifndef DCM_H
#define DCM_H

#include <stdint.h>

void Dcm_Init(void);
void Dcm_MainFunction(uint16_t elapsed_ms);
void Dcm_RxIndication(const uint8_t *request, uint8_t length, uint8_t functional);
uint8_t Dcm_GetCurrentSession(void);

#endif /* DCM_H */
