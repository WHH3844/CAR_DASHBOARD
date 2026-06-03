#ifndef COM_H
#define COM_H

#include "CanIf.h"

#include <stdint.h>

void Com_Init(void);
void Com_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms);
void Com_MainFunction(uint32_t tick_ms);

#endif /* COM_H */
#ifndef COM_H
#define COM_H

#include "CanIf.h"
#include "Std_Types.h"

#include <stdint.h>

void Com_Init(void);
void Com_RxIndication(const CanIf_PduType *pdu);
void Com_MainFunction(uint16_t elapsed_ms);
Std_ReturnType Com_SendIcmStatus(void);
Std_ReturnType Com_SendIcmDiagStatus(void);

#endif /* COM_H */
