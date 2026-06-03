#ifndef FATFSIF_H
#define FATFSIF_H

#include "Std_Types.h"

/*
 * 第一版只完成 TF 卡底层只读识别；真正 FATFS 文件系统后续再接。
 * 这里先保留接口，App_Logger 可以通过状态知道日志功能未启用。
 */
void FatFsIf_Init(void);
Std_ReturnType FatFsIf_Mount(void);
uint8_t FatFsIf_IsMounted(void);

#endif /* FATFSIF_H */
