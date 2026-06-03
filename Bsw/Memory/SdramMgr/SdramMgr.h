#ifndef SDRAMMGR_H
#define SDRAMMGR_H

#include "Std_Types.h"

#include <stdint.h>

/*
 * SDRAM 管理器。
 *
 * 负责 EXMC SDRAM 初始化结果维护，以及板级内存区域分配策略。
 * 当前只暴露 LCD framebuffer；保留 base/size 接口是为了后续做内存自检和
 * 显存/日志缓冲区划分。
 */
Std_ReturnType SdramMgr_Init(void);
uint32_t SdramMgr_GetBase(void);
uint32_t SdramMgr_GetSize(void);

/* 第一版 framebuffer 固定放在 SDRAM base，大小由 LCD 分辨率和 RGB565 决定。 */
uint32_t SdramMgr_GetFrameBuffer(void);

/* 返回 SDRAM 是否可安全访问。 */
uint8_t SdramMgr_IsReady(void);

#endif /* SDRAMMGR_H */
