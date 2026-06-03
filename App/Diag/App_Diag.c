#include "App_Diag.h"

void App_Diag_Init(void)
{
}

void App_Diag_MainFunction(void)
{
}
#include "App_Diag.h"

#include "Dcm.h"

void App_Diag_Init(void)
{
    Dcm_Init();
}

void App_Diag_MainFunction(uint16_t elapsed_ms)
{
    Dcm_MainFunction(elapsed_ms);
}
