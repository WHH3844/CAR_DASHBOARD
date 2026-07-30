#ifndef NVM_CFG_H
#define NVM_CFG_H

#include <stdint.h>

/*
 * FT24C16A 总容量 2048 字节。测试阶段 0x0100 曾用于 EEPROM 写读验证，
 * 正式 NvM 先放在末尾区域，减少和 bring-up 测试地址冲突的概率。
 */
#define NVM_CFG_MAGIC_BOOT_INFO             0x4342u  /* 'B''C' */
#define NVM_CFG_MAGIC_SYSTEM_CONFIG         0x4353u  /* 'S''C' */
#define NVM_CFG_MAGIC_DEM_STATUS            0x444Du  /* 'D''M' */
#define NVM_CFG_VERSION                     0x01u

#define NVM_CFG_ADDR_BOOT_INFO              0x0700u
#define NVM_CFG_ADDR_SYSTEM_CONFIG          0x0720u
#define NVM_CFG_ADDR_DEM_STATUS             0x0740u

#define NVM_CFG_SYSTEM_BACKLIGHT_DEFAULT    100u
#define NVM_CFG_SYSTEM_BUZZER_DEFAULT       1u
#define NVM_CFG_SYSTEM_THEME_DEFAULT        0u
#define NVM_CFG_SYSTEM_LANGUAGE_DEFAULT     0u

#endif /* NVM_CFG_H */
