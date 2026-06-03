#include "PduR.h"

#include "CanTp.h"
#include "Can_Cfg.h"
#include "Com.h"

void PduR_RxIndication(const CanIf_PduType *pdu, uint32_t tick_ms)
{
    if (pdu == 0)
    {
        return;
    }

    /*
     * PduR 只做“路由”，不解析业务含义。
     * 诊断 ID 送 CanTp/Dcm，普通仪表报文送 Com 解信号。
     */
    if ((pdu->id == CAN_ID_DIAG_FUNCTIONAL_REQ) ||
        (pdu->id == CAN_ID_DIAG_PHYSICAL_REQ))
    {
        CanTp_RxIndication(pdu, tick_ms);
    }
    else
    {
        Com_RxIndication(pdu, tick_ms);
    }
}
#include "PduR.h"

#include "CanTp.h"
#include "Com.h"
#include "Can_Cfg.h"

void PduR_Init(void)
{
    /* 当前路由表全是静态 CAN ID，初始化时不需要动态分配资源。 */
}

void PduR_CanIfRxIndication(const CanIf_PduType *pdu)
{
    if (pdu == 0)
    {
        return;
    }

    /*
     * PduR 只看 PDU ID 并决定交给谁：
     * - 诊断请求交给 CanTp/Dcm
     * - 周期业务报文交给 Com 解信号
     */
    if ((pdu->id == CAN_ID_DIAG_FUNCTIONAL_REQ) ||
        (pdu->id == CAN_ID_DIAG_PHYSICAL_REQ))
    {
        CanTp_RxIndication(pdu);
        return;
    }

    Com_RxIndication(pdu);
}

Std_ReturnType PduR_ComTransmit(const CanIf_PduType *pdu)
{
    return CanIf_Transmit(pdu);
}

Std_ReturnType PduR_CanTpTransmit(const CanIf_PduType *pdu)
{
    return CanIf_Transmit(pdu);
}
