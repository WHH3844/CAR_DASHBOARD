#include "CanIf.h"

#include "Can.h"
#include "CanTrcv.h"
#include "Can_Cfg.h"
#include "Dem.h"
#include "LogM.h"
#include "PduR.h"

/*
 * CanIf_Status 是 CAN 链路的运行期健康快照。
 * 统计量不参与控制逻辑，但对串口日志/诊断 DID/现场 bring-up 很有价值。
 */
static CanIf_StatusType CanIf_Status;

Std_ReturnType CanIf_Init(void)
{
    /*
     * 初始化顺序先收发器、后控制器：
     * 收发器进入 NORMAL 后，CAN 控制器发出的显性/隐性电平才能真正到总线上。
     */
    CanTrcv_Init();
    (void)CanTrcv_SetMode(CANTRCV_MODE_NORMAL);

    if (Can1_Init500K() == 0u)
    {
        CanIf_Status.initialized = 0u;
        Dem_SetEventStatus(DEM_EVENT_CAN_BUS_OFF, DEM_EVENT_STATUS_FAILED);
        LogM_Warn("CAN1 500K init failed");
        return E_NOT_OK;
    }

    CanIf_Status.initialized = 1u;
    CanIf_Status.bus_error = 0u;
    CanIf_Status.trcv_error = 0u;
    Dem_SetEventStatus(DEM_EVENT_CAN_BUS_OFF, DEM_EVENT_STATUS_PASSED);
    Dem_SetEventStatus(DEM_EVENT_CAN_TRCV_ERROR, DEM_EVENT_STATUS_PASSED);
    LogM_Info("CAN1 500K init ok");
    return E_OK;
}

Std_ReturnType CanIf_Transmit(uint16_t can_id, const uint8_t *data, uint8_t dlc)
{
    Can1_TxResultType result;

    if ((CanIf_Status.initialized == 0u) || (data == 0))
    {
        return E_NOT_OK;
    }

    if (dlc > CAN_CFG_DLC)
    {
        /* 上层即使误传超长 dlc，也裁剪为 Classic CAN 最大 8 字节，避免底层越界。 */
        dlc = CAN_CFG_DLC;
    }

    result = Can1_SendStd(can_id, data, dlc, CAN_CFG_TX_TIMEOUT_LOOP);
    if (result == CAN1_TX_OK)
    {
        CanIf_Status.tx_total++;
        return E_OK;
    }

    CanIf_Status.tx_error++;
    return E_NOT_OK;
}

void CanIf_MainFunction(uint32_t tick_ms)
{
    Can_MessageType message;
    CanIf_PduType pdu;
    uint8_t index;

    (void)tick_ms;

    if (CanIf_Status.initialized == 0u)
    {
        return;
    }

    /*
     * 当前第一版使用轮询接收，避免一开始引入中断队列。
     * 后续如果 CAN 负载变高，可以把 Can1_Read 放进 RX 中断，再由 CanIf 消费队列。
     */
    while (Can1_Read(&message) != 0u)
    {
        if ((message.is_extended != 0u) || (message.is_remote != 0u))
        {
            /*
             * 当前 CAN 矩阵只定义 11-bit 数据帧。
             * 扩展帧/远程帧直接丢弃，防止不符合矩阵的帧进入 Com/Dcm。
             */
            continue;
        }

        pdu.id = (uint16_t)(message.id & 0x7FFu);
        pdu.dlc = (message.dlc > CAN_CFG_DLC) ? CAN_CFG_DLC : message.dlc;

        /*
         * PDU 数据统一整理成 8 字节数组。
         * 对短帧补 0 可以让上层按固定下标访问未使用字节时得到确定值。
         */
        for (index = 0u; index < pdu.dlc; index++)
        {
            pdu.data[index] = message.data[index];
        }
        for (; index < CAN_CFG_DLC; index++)
        {
            pdu.data[index] = 0u;
        }

        CanIf_Status.rx_total++;
        PduR_RxIndication(&pdu, tick_ms);
    }

    /*
     * 收发器错误引脚是硬件层面的状态提示，和 CAN 控制器错误计数互补。
     * 每个 CanIf 周期刷新一次 Dem，便于诊断读取到最新链路状态。
     */
    CanIf_Status.trcv_error = CanTrcv_IsErrorAsserted();
    Dem_SetEventStatus(DEM_EVENT_CAN_TRCV_ERROR,
                       (CanIf_Status.trcv_error != 0u) ? DEM_EVENT_STATUS_FAILED : DEM_EVENT_STATUS_PASSED);
}

void CanIf_GetStatus(CanIf_StatusType *status)
{
    if (status != 0)
    {
        *status = CanIf_Status;
    }
}

uint8_t CanIf_IsInitialized(void)
{
    return CanIf_Status.initialized;
}
