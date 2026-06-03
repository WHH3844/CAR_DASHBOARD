#include "Det.h"

#include "LogM.h"

void Det_Init(void)
{
}

void Det_ReportError(uint16_t module_id, uint8_t instance_id, uint8_t api_id, uint8_t error_id)
{
    LogM_PutString("[DET] module=");
    LogM_PutDec(module_id);
    LogM_PutString(" inst=");
    LogM_PutDec(instance_id);
    LogM_PutString(" api=");
    LogM_PutDec(api_id);
    LogM_PutString(" err=");
    LogM_PutDec(error_id);
    LogM_NewLine();
}
