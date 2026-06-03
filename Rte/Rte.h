#ifndef RTE_H
#define RTE_H

#include "Rte_Event.h"
#include "Rte_Service.h"
#include "Rte_Signal.h"

/*
 * Runtime Environment 聚合入口。
 *
 * RTE 是 APP 与 BSW 之间的隔离层：
 * - Rte_Signal 保存运行期信号快照；
 * - Rte_Event 负责轻量事件发布；
 * - Rte_Service 提供对 NvM/Dem/硬件抽象的服务调用封装。
 */
void Rte_Init(void);

#endif /* RTE_H */
