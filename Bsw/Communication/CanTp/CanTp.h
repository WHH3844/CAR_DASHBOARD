#ifndef CANTP_H
#define CANTP_H

#include "CanIf.h"
#include "Std_Types.h"

#include <stdint.h>

void CanTp_Init(void);
void CanTp_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms);
Std_ReturnType CanTp_TransmitResponse(const uint8_t *payload, uint8_t length);

#endif /* CANTP_H */
#ifndef CANTP_H
#define CANTP_H

#include "CanIf.h"
#include "Std_Types.h"

#include <stdint.h>

void CanTp_Init(void);
void CanTp_RxIndication(const CanIf_PduType *pdu);
Std_ReturnType CanTp_Transmit(uint16_t response_id, const uint8_t *payload, uint8_t length);

#endif /* CANTP_H */
