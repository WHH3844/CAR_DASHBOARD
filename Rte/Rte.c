#include "Rte.h"

void Rte_Init(void)
{
    /*
     * 先初始化信号快照，再初始化事件层。
     * 事件层当前依赖 Rte_Write_KeyEvent()，因此需要保证默认 key event 已经清零。
     */
    Rte_Signal_Init();
    Rte_Event_Init();
}
