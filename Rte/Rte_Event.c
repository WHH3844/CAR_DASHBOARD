#include "Rte_Event.h"

void Rte_Event_Init(void)
{
}

void Rte_Event_PublishKey(Rte_KeyEventType event)
{
    (void)Rte_Write_KeyEvent(event);
}
#include "Rte_Event.h"

static uint8_t Rte_KeyEvent;

void Rte_EventInit(void)
{
    Rte_KeyEvent = RTE_KEY_EVENT_NONE;
}

void Rte_EventPushKey(uint8_t event_id)
{
    /*
     * 第一版只保留最近一次按键事件。
     * 如果后续按键菜单复杂，可以换成小环形队列。
     */
    Rte_KeyEvent = event_id;
}

uint8_t Rte_EventPopKey(void)
{
    uint8_t event_id;

    event_id = Rte_KeyEvent;
    Rte_KeyEvent = RTE_KEY_EVENT_NONE;
    return event_id;
}
