#include "SdramIf.h"

#include "SdramMgr.h"

Std_ReturnType SdramIf_Init(void)
{
    /* 当前抽象层直接委托 SdramMgr，保留独立接口是为了隔离上层和内存分配策略。 */
    return SdramMgr_Init();
}

uint32_t SdramIf_GetFrameBuffer(void)
{
    return SdramMgr_GetFrameBuffer();
}

uint8_t SdramIf_IsReady(void)
{
    return SdramMgr_IsReady();
}
