#ifndef SDRAMIF_H
#define SDRAMIF_H

#include "Std_Types.h"

#include <stdint.h>

/*
 * SDRAM ECU 抽象层。
 *
 * 该层故意很薄，只把上层和具体 SdramMgr 隔开。后续如果增加内存测试、
 * 多 framebuffer 或堆区划分，可以先在 SdramMgr 扩展，App/LcdIf 调用面不变。
 */
Std_ReturnType SdramIf_Init(void);

/* 返回分配给 LCD 的 framebuffer 起始地址。调用前应确保 SdramIf_Init() 成功。 */
uint32_t SdramIf_GetFrameBuffer(void);

/* 返回 SDRAM 管理器是否已经完成初始化。 */
uint8_t SdramIf_IsReady(void);

#endif /* SDRAMIF_H */
