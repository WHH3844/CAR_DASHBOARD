#ifndef RTE_EVENT_H
#define RTE_EVENT_H

#include "Rte_Signal.h"

/*
 * RTE 事件层。
 *
 * 当前项目事件量很小，只用单个 key event 槽位承载最近一次按键事件。
 * 同时为 Com 保留一份用户输入事件快照，用于发送 0x327。
 * 后续如果有丢事件风险，可以在这里替换成环形队列，而 APP 调用接口保持不变。
 */
void Rte_Event_Init(void);

/* 发布按键事件，底层实现写入 Rte_Signal 的 key event 槽位。 */
void Rte_Event_PublishKey(Rte_KeyEventType event);

#endif /* RTE_EVENT_H */
