#ifndef RTE_EVENT_H
#define RTE_EVENT_H

#include "Rte_Signal.h"

void Rte_Event_Init(void);
void Rte_Event_PublishKey(Rte_KeyEventType event);

#endif /* RTE_EVENT_H */
#ifndef RTE_EVENT_H
#define RTE_EVENT_H

#include <stdint.h>

#define RTE_KEY_EVENT_NONE          0u
#define RTE_KEY_EVENT_KEY1_SHORT    1u
#define RTE_KEY_EVENT_KEY2_SHORT    2u
#define RTE_KEY_EVENT_KEY3_SHORT    3u
#define RTE_KEY_EVENT_POWER_LONG    4u

void Rte_EventInit(void);
uint8_t Rte_EventPopKey(void);
void Rte_EventPushKey(uint8_t event_id);

#endif /* RTE_EVENT_H */
