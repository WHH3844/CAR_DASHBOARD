#include "CanSM.h"

#include "Can.h"
#include "CanIf.h"
#include "Dem.h"

static CanSM_StateType CanSM_State;

void CanSM_Init(void)
{
    CanSM_State = CanIf_IsInitialized() ? CANSM_STATE_FULL_COMM : CANSM_STATE_NO_COMM;
}

void CanSM_MainFunction(uint32_t tick_ms)
{
    (void)tick_ms;

    if (CanIf_IsInitialized() == 0u)
    {
        CanSM_State = CANSM_STATE_NO_COMM;
        return;
    }

    /*
     * GD32 标准库当前只暴露错误计数，第一版用错误计数过高近似判断总线异常。
     * 如果后续补充 ESR/BusOff 状态读取，可以在这里替换成更准确的判断。
     */
    if ((Can1_TxErrorCount() > 200u) || (Can1_RxErrorCount() > 200u))
    {
        CanSM_State = CANSM_STATE_BUS_OFF;
        Dem_SetEventStatus(DEM_EVENT_CAN_BUS_OFF, DEM_EVENT_STATUS_FAILED);
    }
    else
    {
        CanSM_State = CANSM_STATE_FULL_COMM;
        Dem_SetEventStatus(DEM_EVENT_CAN_BUS_OFF, DEM_EVENT_STATUS_PASSED);
    }
}

CanSM_StateType CanSM_GetState(void)
{
    return CanSM_State;
}
#include "CanSM.h"

#include "CanIf.h"

static CanSM_StateType CanSM_State;

void CanSM_Init(void)
{
    CanSM_State = CANSM_NO_COMM;
}

void CanSM_MainFunction(void)
{
    CanSM_State = (CanIf_IsOnline() != 0u) ? CANSM_FULL_COMM : CANSM_NO_COMM;
}

CanSM_StateType CanSM_GetState(void)
{
    return CanSM_State;
}
