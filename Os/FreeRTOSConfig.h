#ifndef OS_FREERTOS_CONFIG_COMPAT_H
#define OS_FREERTOS_CONFIG_COMPAT_H

/*
 * 兼容旧路径的转发头文件。
 *
 * FreeRTOS 真正生效的配置统一放在 Config/FreeRTOSConfig.h；
 * 保留本文件是为了避免以后旧代码包含 "Os/FreeRTOSConfig.h" 时找不到文件。
 */
#include "../Config/FreeRTOSConfig.h"

#endif /* OS_FREERTOS_CONFIG_COMPAT_H */
