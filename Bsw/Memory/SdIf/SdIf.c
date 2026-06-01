#include "SdIf.h"

Sdio_StatusType SdIf_Init(void)
{
    return Sdio_CardInit();
}

uint8_t SdIf_ReadBlock0(uint8_t *buffer)
{
    if (Sdio_ReadBlock(0u, buffer) == SDIO_STATUS_OK)
    {
        return 1u;
    }

    return 0u;
}

const Sdio_CardInfoType *SdIf_GetCardInfo(void)
{
    return Sdio_GetCardInfo();
}

Sdio_StatusType SdIf_GetLastStatus(void)
{
    return Sdio_GetLastStatus();
}

uint32_t SdIf_GetLastCommand(void)
{
    return Sdio_GetLastCommand();
}

uint32_t SdIf_GetLastStatusRegister(void)
{
    return Sdio_GetLastStatusRegister();
}
