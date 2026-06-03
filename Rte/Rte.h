#ifndef RTE_H
#define RTE_H

#include "Rte_Event.h"
#include "Rte_Service.h"
#include "Rte_Signal.h"

void Rte_Init(void);

#endif /* RTE_H */
#ifndef RTE_H
#define RTE_H

#include "Rte_Signal.h"

void Rte_Init(void);
void Rte_MainFunction(uint16_t elapsed_ms);

#endif /* RTE_H */
