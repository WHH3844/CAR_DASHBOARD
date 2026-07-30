#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/*
 * CRC 服务层：目前只提供 NvM 需要的 CRC16-CCITT。
 * 后续如果 CAN 报文加 rolling counter/checksum，也可以继续扩展在这里。
 */
uint16_t Crc_CalculateCrc16(const uint8_t *data, uint16_t length, uint16_t start_value);

/*
 * SAE J1850 CRC8：poly=0x1D、init=0xFF、xorout=0xFF、非反射。
 * v1.0 的 0x329 用它覆盖 CAN ID 低/高字节和 Byte0..Byte6。
 */
uint8_t Crc_CalculateCrc8J1850(const uint8_t *data, uint16_t length);

#endif /* CRC_H */
