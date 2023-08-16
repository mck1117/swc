/**
 * @file        i2c_bb.h
 * @brief       Bit-banged I2C driver
 *
 * @date February 6, 2020
 * @author Matthew Kennedy, (c) 2020
 */

#pragma once

class BitbangI2c
{
public:
    BitbangI2c(ioline_t scl, ioline_t sda);
    ~BitbangI2c();

    // Write a sequence of bytes to the specified device
    void write(uint8_t addr, const uint8_t* data, size_t size);
    // Read a sequence of bytes from the device
    void read(uint8_t addr, uint8_t* data, size_t size);
    // Write some bytes then read some bytes back after a repeated start bit
    void writeRead(uint8_t addr, const uint8_t* writeData, size_t writeSize, uint8_t* readData, size_t readSize);

    // Read a register at the specified address and register index
    uint8_t readRegister(uint8_t addr, uint8_t reg);
    // Write a register at the specified address and register index
    void writeRegister(uint8_t addr, uint8_t reg, uint8_t val);

private:
    // Returns true if the remote device acknowledged the transmission
    bool writeByte(uint8_t data);
    uint8_t readByte(bool ack);

    void sda_low();
    void sda_high();
    void scl_low();
    void scl_high();

    // Send an I2C start condition
    void start();
    // Send an I2C stop condition
    void stop();

    // Send a single bit
    void sendBit(bool val);
    // Read a single bit
    bool readBit();

    // Wait for 1/4 of a bit time
    void waitQuarterBit();

    const ioline_t m_scl;
    const ioline_t m_sda;
};
