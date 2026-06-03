#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/*
 * CRC 服务层：目前只提供 NvM 需要的 CRC16-CCITT。
 * 后续如果 CAN 报文加 rolling counter/checksum，也可以继续扩展在这里。
 */
uint16_t Crc_CalculateCrc16(const uint8_t *data, uint16_t length, uint16_t start_value);

#endif /* CRC_H */
