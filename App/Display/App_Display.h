#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include <stdint.h>

/*
 * 仪表 LCD 显示应用层。
 *
 * App_Display 只组织页面内容和刷新策略，不关心具体 LCD 控制器寄存器。
 * 静态布局只绘制一次，运行数据按固定小区域擦除/重绘，用来降低 SDRAM framebuffer
 * 写入量，也避免整屏清除导致肉眼可见闪烁。
 */
void App_Display_Init(void);

/*
 * 周期刷新当前仪表画面。
 *
 * tick_ms 当前未参与显示计算，但保留在接口中，便于后续添加闪烁图标、
 * 动画报警或显示任务耗时监控时不用修改 EcuM 调用链。
 */
void App_Display_MainFunction(uint32_t tick_ms);

#endif /* APP_DISPLAY_H */
