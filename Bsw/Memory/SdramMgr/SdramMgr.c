#include "SdramMgr.h"

#include "Dem.h"
#include "Exmc.h"
#include "LcdTli.h"
#include "LogM.h"

static uint8_t SdramMgr_Ready;
static uint32_t SdramMgr_FrameBuffer;

Std_ReturnType SdramMgr_Init(void)
{
    if (Exmc_SdramInit() == 0u)
    {
        SdramMgr_Ready = 0u;
        Dem_SetEventStatus(DEM_EVENT_SDRAM_INIT_FAILED, DEM_EVENT_STATUS_FAILED);
        LogM_Error("SDRAM init failed");
        return E_NOT_OK;
    }

    /*
     * 第一版把 LCD framebuffer 固定放在 SDRAM 起始地址。
     * 800x480 RGB565 只用约 750KB，远小于 32MB SDRAM。
     */
    SdramMgr_FrameBuffer = Exmc_SdramBase();
    SdramMgr_Ready = 1u;
    Dem_SetEventStatus(DEM_EVENT_SDRAM_INIT_FAILED, DEM_EVENT_STATUS_PASSED);
    LogM_Info("SDRAM init ok");
    return E_OK;
}

uint32_t SdramMgr_GetBase(void)
{
    return Exmc_SdramBase();
}

uint32_t SdramMgr_GetSize(void)
{
    return Exmc_SdramSize();
}

uint32_t SdramMgr_GetFrameBuffer(void)
{
    return SdramMgr_FrameBuffer;
}

uint8_t SdramMgr_IsReady(void)
{
    return SdramMgr_Ready;
}
#include "SdramMgr.h"

#include "Dem.h"
#include "Exmc.h"

static uint8_t SdramMgr_Ready;

Std_ReturnType SdramMgr_Init(void)
{
    if (Exmc_SdramInit() == 0u)
    {
        SdramMgr_Ready = 0u;
        (void)Dem_SetEventStatus(DEM_EVENT_SDRAM_INIT_FAILED, DEM_EVENT_STATUS_FAILED);
        return E_NOT_OK;
    }

    SdramMgr_Ready = 1u;
    (void)Dem_SetEventStatus(DEM_EVENT_SDRAM_INIT_FAILED, DEM_EVENT_STATUS_PASSED);
    return E_OK;
}

uint32_t SdramMgr_GetFrameBuffer(void)
{
    return Exmc_SdramBase();
}

uint32_t SdramMgr_GetSizeBytes(void)
{
    return Exmc_SdramSize();
}

uint8_t SdramMgr_IsReady(void)
{
    return SdramMgr_Ready;
}
