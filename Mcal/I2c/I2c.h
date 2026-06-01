#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void I2c0_Init100K(void);
uint8_t I2c0_ProbeAddress(uint8_t address7, uint32_t timeout_loop);
uint8_t I2c0_WriteBytes(uint8_t address7,
                        const uint8_t *data,
                        uint8_t len,
                        uint32_t timeout_loop);
uint8_t I2c0_ReadByteAfterWriteByte(uint8_t address7,
                                    uint8_t write_byte,
                                    uint8_t *read_byte,
                                    uint32_t timeout_loop);
uint8_t I2c0_ReadBytes(uint8_t address7,
                       uint8_t *data,
                       uint8_t len,
                       uint32_t timeout_loop);
uint8_t I2c0_SclIsHigh(void);
uint8_t I2c0_SdaIsHigh(void);

#endif /* I2C_H */
