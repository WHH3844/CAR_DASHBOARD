#include "Rte_Event.h"

void Rte_Event_Init(void)
{
    /*
     * 当前事件层没有独立 RAM 状态。
     * 按键事件槽位由 Rte_Signal_Init() 清为 RTE_KEY_EVENT_NONE。
     */
}

void Rte_Event_PublishKey(Rte_KeyEventType event)
{
    /*
     * 单槽位事件模型：新事件会覆盖旧事件。
     * 目前按键扫描和 Dashboard 消费都在同一个 10ms 主循环内，覆盖风险可接受。
     */
    (void)Rte_Write_KeyEvent(event);
}
