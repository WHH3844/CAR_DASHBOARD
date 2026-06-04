#include "Rte_Event.h"

static uint8_t Rte_Event_UserInputCounter;

void Rte_Event_Init(void)
{
    /*
     * KeyEvent 槽位由 Rte_Signal_Init() 清零。
     * 这里额外维护 0x327 事件计数器，用于接收端判断是否漏事件。
     */
    Rte_Event_UserInputCounter = 0u;
}

void Rte_Event_PublishKey(Rte_KeyEventType event)
{
    Rte_UserInputEventType user_input;

    /*
     * 单槽位事件模型：新事件会覆盖旧事件。
     * 目前按键扫描和 Dashboard 消费都在同一个 10ms 主循环内，覆盖风险可接受。
     */
    (void)Rte_Write_KeyEvent(event);

    user_input.key_action = 1u;
    user_input.power_key_long_press = 0u;
    user_input.shutdown_confirm = 0u;

    if (event == RTE_KEY_EVENT_KEY1_SHORT)
    {
        user_input.key_code = 1u;
    }
    else if (event == RTE_KEY_EVENT_KEY2_SHORT)
    {
        user_input.key_code = 2u;
    }
    else if (event == RTE_KEY_EVENT_KEY3_SHORT)
    {
        user_input.key_code = 3u;
    }
    else if (event == RTE_KEY_EVENT_POWER_LONG)
    {
        user_input.key_code = 4u;
        user_input.key_action = 2u;
        user_input.power_key_long_press = 1u;
        user_input.shutdown_confirm = 1u;
    }
    else
    {
        user_input.key_code = 0u;
        user_input.key_action = 0u;
    }

    if (user_input.key_code != 0u)
    {
        Rte_Event_UserInputCounter++;
        user_input.event_counter = Rte_Event_UserInputCounter;
        (void)Rte_Write_UserInputEvent(&user_input);
    }
}
