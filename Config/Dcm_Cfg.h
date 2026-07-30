#ifndef DCM_CFG_H
#define DCM_CFG_H

/*
 * 诊断第一版目标：
 * 0x10 会话控制、0x22 读取 DID、0x19 读取 DTC、
 * 0x14 清 DTC、0x3E TesterPresent。
 * 只做教学级 UDS 单帧子集，不做刷写和安全访问。
 */

#define DCM_CFG_P2_SERVER_MAX_MS            50u
#define DCM_CFG_P2STAR_SERVER_MAX_MS        5000u
#define DCM_CFG_S3_SERVER_TIMEOUT_MS        5000u

#define DCM_SESSION_DEFAULT                 0x01u
#define DCM_SESSION_PROGRAMMING             0x02u
#define DCM_SESSION_EXTENDED                0x03u

/*
 * v1.0 DID 命名空间：
 * F180-F19F ECU 身份；F200-F21F 运行数据；F220-F23F 配置；F240-F25F 测试。
 */
#define DCM_DID_HW_VERSION                  0xF191u
#define DCM_DID_SW_VERSION                  0xF193u
#define DCM_DID_BOOT_COUNTER                0xF200u
#define DCM_DID_VEHICLE_SPEED               0xF201u
#define DCM_DID_ENGINE_RPM                  0xF202u
#define DCM_DID_BATTERY_VOLTAGE             0xF203u
#define DCM_DID_RTC_TIME                    0xF204u
#define DCM_DID_SDRAM_TEST_RESULT           0xF240u

#endif /* DCM_CFG_H */
