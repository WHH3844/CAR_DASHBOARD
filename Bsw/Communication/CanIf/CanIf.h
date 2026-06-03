#ifndef CANIF_H
#define CANIF_H

#include "Std_Types.h"

#include <stdint.h>

typedef struct
{
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
} CanIf_PduType;

typedef struct
{
    uint32_t rx_total;
    uint32_t tx_total;
    uint32_t tx_error;
    uint8_t initialized;
    uint8_t bus_error;
    uint8_t trcv_error;
} CanIf_StatusType;

Std_ReturnType CanIf_Init(void);
Std_ReturnType CanIf_Transmit(uint16_t can_id, const uint8_t *data, uint8_t dlc);
void CanIf_MainFunction(uint32_t tick_ms);
void CanIf_GetStatus(CanIf_StatusType *status);
uint8_t CanIf_IsInitialized(void);

#endif /* CANIF_H */
#ifndef CANIF_H
#define CANIF_H

#include "Std_Types.h"

#include <stdint.h>

typedef struct
{
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
} CanIf_PduType;

Std_ReturnType CanIf_Init(void);
Std_ReturnType CanIf_Transmit(const CanIf_PduType *pdu);
void CanIf_MainFunctionRx(void);
void CanIf_MainFunctionBusOff(void);
uint8_t CanIf_IsOnline(void);
uint32_t CanIf_GetRxCounter(void);
uint32_t CanIf_GetTxCounter(void);

#endif /* CANIF_H */
