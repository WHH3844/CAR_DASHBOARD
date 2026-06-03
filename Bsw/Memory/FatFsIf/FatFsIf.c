#include "FatFsIf.h"

#include "Dem.h"
#include "SdIf.h"

static uint8_t FatFsIf_Mounted;

void FatFsIf_Init(void)
{
    FatFsIf_Mounted = 0u;
}

Std_ReturnType FatFsIf_Mount(void)
{
    /*
     * 当前还没有移植 FATFS，只验证 SDIO 读卡。
     * 如果 SDIO 初始化成功，说明硬件通路可用，但仍不声明文件系统已挂载。
     */
    if (SdIf_Init() == SDIO_STATUS_OK)
    {
        FatFsIf_Mounted = 0u;
        (void)Dem_SetEventStatus(DEM_EVENT_TF_CARD_MOUNT_FAILED, DEM_EVENT_STATUS_PASSED);
        return E_OK;
    }

    FatFsIf_Mounted = 0u;
    (void)Dem_SetEventStatus(DEM_EVENT_TF_CARD_MOUNT_FAILED, DEM_EVENT_STATUS_FAILED);
    return E_NOT_OK;
}

uint8_t FatFsIf_IsMounted(void)
{
    return FatFsIf_Mounted;
}
