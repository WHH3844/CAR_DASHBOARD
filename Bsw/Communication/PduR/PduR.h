#ifndef PDUR_H
#define PDUR_H

#include "CanIf.h"

#include <stdint.h>

void PduR_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms);

#endif /* PDUR_H */
#ifndef PDUR_H
#define PDUR_H

#include "CanIf.h"
#include "Std_Types.h"

void PduR_Init(void);
void PduR_CanIfRxIndication(const CanIf_PduType *pdu);
Std_ReturnType PduR_ComTransmit(const CanIf_PduType *pdu);
Std_ReturnType PduR_CanTpTransmit(const CanIf_PduType *pdu);

#endif /* PDUR_H */
