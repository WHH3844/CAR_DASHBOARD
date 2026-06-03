#include "CanIf.h"

#include "Can.h"
#include "CanTrcv.h"
#include "Can_Cfg.h"
#include "Dem.h"
#include "LogM.h"
#include "PduR.h"

static CanIf_StatusType CanIf_Status;

Std_ReturnType CanIf_Init(void)
{
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
            continue;
        }

        pdu.id = (uint16_t)(message.id & 0x7FFu);
        pdu.dlc = (message.dlc > CAN_CFG_DLC) ? CAN_CFG_DLC : message.dlc;

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
#include "CanIf.h"

#include "Can.h"
#include "CanTrcv.h"
#include "Dem.h"
#include "LogM.h"
#include "PduR.h"
#include "Can_Cfg.h"

static uint8_t CanIf_Online;
static uint32_t CanIf_RxCounter;
static uint32_t CanIf_TxCounter;

Std_ReturnType CanIf_Init(void)
{
    CanTrcv_Init();
    (void)CanTrcv_SetMode(CANTRCV_MODE_NORMAL);

    if (Can1_Init500K() == 0u)
    {
        CanIf_Online = 0u;
        (void)Dem_SetEventStatus(DEM_EVENT_CAN_BUS_OFF, DEM_EVENT_STATUS_FAILED);
        LogM_Error("CanIf init failed");
        return E_NOT_OK;
    }

    CanIf_Online = 1u;
    (void)Dem_SetEventStatus(DEM_EVENT_CAN_BUS_OFF, DEM_EVENT_STATUS_PASSED);
    LogM_Info("CanIf CAN1 500K online");
    return E_OK;
}

Std_ReturnType CanIf_Transmit(const CanIf_PduType *pdu)
{
    Can1_TxResultType result;

    if ((pdu == 0) || (pdu->dlc > CAN_CFG_DLC) || (CanIf_Online == 0u))
    {
        return E_NOT_OK;
    }

    result = Can1_SendStd(pdu->id, pdu->data, pdu->dlc, CAN_CFG_TX_TIMEOUT_LOOP);
    if (result == CAN1_TX_OK)
    {
        CanIf_TxCounter++;
        return E_OK;
    }

    return E_NOT_OK;
}

void CanIf_MainFunctionRx(void)
{
    Can_MessageType message;
    CanIf_PduType pdu;
    uint8_t index;

    if (CanIf_Online == 0u)
    {
        return;
    }

    /*
     * 这里是轮询式接收。后续如果改成 CAN RX 中断，
     * 也建议中断只入队，真正的 PDU 分发仍放在 CanIf_MainFunctionRx()。
     */
    while (Can1_Read(&message) != 0u)
    {
        if ((message.is_extended != 0u) || (message.is_remote != 0u))
        {
            continue;
        }

        pdu.id = (uint16_t)(message.id & 0x7FFu);
        pdu.dlc = (message.dlc > CAN_CFG_DLC) ? CAN_CFG_DLC : message.dlc;
        for (index = 0u; index < pdu.dlc; index++)
        {
            pdu.data[index] = message.data[index];
        }
        for (; index < CAN_CFG_DLC; index++)
        {
            pdu.data[index] = 0u;
        }

        CanIf_RxCounter++;
        PduR_CanIfRxIndication(&pdu);
    }
}

void CanIf_MainFunctionBusOff(void)
{
    if (CanIf_Online == 0u)
    {
        return;
    }

    if (CanTrcv_IsErrorAsserted() != 0u)
    {
        (void)Dem_SetEventStatus(DEM_EVENT_CAN_TRCV_ERROR, DEM_EVENT_STATUS_FAILED);
    }
    else
    {
        (void)Dem_SetEventStatus(DEM_EVENT_CAN_TRCV_ERROR, DEM_EVENT_STATUS_PASSED);
    }
}

uint8_t CanIf_IsOnline(void)
{
    return CanIf_Online;
}

uint32_t CanIf_GetRxCounter(void)
{
    return CanIf_RxCounter;
}

uint32_t CanIf_GetTxCounter(void)
{
    return CanIf_TxCounter;
}
