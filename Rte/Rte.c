#include "Rte.h"

void Rte_Init(void)
{
    Rte_Signal_Init();
    Rte_Event_Init();
}
#include "Rte.h"

void Rte_Init(void)
{
    Rte_SignalInit();
}

void Rte_MainFunction(uint16_t elapsed_ms)
{
    Rte_SignalMainFunction(elapsed_ms);
}
