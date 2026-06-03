#ifndef APP_DIAG_H
#define APP_DIAG_H

/*
 * 应用层诊断扩展点。
 *
 * 当前 UDS 服务主要由 Dcm/Dem/NvM 直接完成，App_Diag 暂时作为应用自检、
 * 诊断联动和后续 DID 动态数据准备的预留模块。保留独立入口可以避免将来把
 * 应用诊断逻辑塞进 EcuM 或 Dcm。
 */
void App_Diag_Init(void);
void App_Diag_MainFunction(void);

#endif /* APP_DIAG_H */
