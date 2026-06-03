#ifndef LOGM_H
#define LOGM_H

#include <stdint.h>

/*
 * LogM 是统一日志出口。底层驱动尽量不要直接 printf，
 * 以后如果要切到环形缓冲、TF 卡日志或 RTT，只改这个模块即可。
 */
void LogM_Init(void);
void LogM_Info(const char *text);
void LogM_Warn(const char *text);
void LogM_Error(const char *text);
void LogM_PutString(const char *text);
void LogM_PutHex32(uint32_t value);
void LogM_PutDec(uint32_t value);
void LogM_NewLine(void);

#endif /* LOGM_H */
