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
        /*
         * 功能寻址和物理寻址都交给 CanTp。
         * Dcm 目前没有区分两者的服务权限，但保留 rx_can_id 便于后续扩展。
         */
        CanTp_RxIndication(pdu, tick_ms);
    }
    else
    {
        /* 业务报文进入 Com 后再按具体 CAN ID 解信号。 */
        Com_RxIndication(pdu, tick_ms);
    }
}
