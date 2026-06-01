#ifndef CAN_H
#define CAN_H

#include <stdint.h>

typedef struct
{
    uint32_t id;
    uint8_t is_extended;
    uint8_t is_remote;
    uint8_t dlc;
    uint8_t data[8];
} Can_MessageType;

typedef enum
{
    CAN1_TX_OK = 0u,
    CAN1_TX_NO_MAILBOX = 1u,
    CAN1_TX_TIMEOUT = 2u,
    CAN1_TX_FAILED = 3u
} Can1_TxResultType;

uint8_t Can1_Init500K(void);
Can1_TxResultType Can1_SendStd(uint16_t id,
                               const uint8_t *data,
                               uint8_t len,
                               uint32_t timeout_loop);
uint8_t Can1_Read(Can_MessageType *message);
uint8_t Can1_ErrIsAsserted(void);
uint8_t Can1_TxErrorCount(void);
uint8_t Can1_RxErrorCount(void);

#endif /* CAN_H */
