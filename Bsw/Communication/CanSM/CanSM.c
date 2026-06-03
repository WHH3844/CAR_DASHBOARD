#include "CanSM.h"

#include "Can.h"
#include "CanIf.h"
#include "Dem.h"

/* CanSM_State 是通信模式的简化状态机，供诊断/日志读取。 */
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
        /* CanIf 初始化失败时不尝试恢复，总线保持 NO_COMM，故障由 CanIf_Init 上报。 */
        CanSM_State = CANSM_STATE_NO_COMM;
        return;
    }

    /*
     * GD32 标准库当前只暴露错误计数，第一版用错误计数过高近似判断总线异常。
     * 如果后续补充 ESR/BusOff 状态读取，可以在这里替换成更准确的判断。
     */
    if ((Can1_TxErrorCount() > 200u) || (Can1_RxErrorCount() > 200u))
    {
        /*
         * 200 接近 bus-off 前的严重错误区间。
         * 第一版用保守阈值先触发 Dem，方便现场看到异常，而不是等完全 bus-off。
         */
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
