/**
 * @file        i2c_bb.cpp
 * @brief       Bit-banged I2C driver
 *
 * @date February 6, 2020
 * @author Matthew Kennedy, (c) 2020
 */

#include "hal.h"
#include <cstdint>

#include "i2c_bb.h"

void BitbangI2c::sda_high()
{
	palSetLine(m_sda);
}

void BitbangI2c::sda_low()
{
	palClearLine(m_sda);
}

void BitbangI2c::scl_high()
{
	palSetLine(m_scl);
}

void BitbangI2c::scl_low()
{
	palClearLine(m_scl);
}

BitbangI2c::BitbangI2c(ioline_t scl, ioline_t sda)
	: m_scl(scl)
	, m_sda(sda)
{
	// Both lines idle high
	scl_high();
	sda_high();

	palSetLineMode(scl, PAL_MODE_OUTPUT_OPENDRAIN);
	palSetLineMode(sda, PAL_MODE_OUTPUT_OPENDRAIN);
}

BitbangI2c::~BitbangI2c()
{
	palSetLineMode(m_scl, PAL_MODE_INPUT);
	palSetLineMode(m_sda, PAL_MODE_INPUT);
}

void BitbangI2c::start()
{
	// Start with both lines high (bus idle)
	sda_high();
	waitQuarterBit();
	scl_high();
	waitQuarterBit();

	// SDA goes low while SCL is high
	sda_low();
	waitQuarterBit();
	scl_low();
	waitQuarterBit();
}

void BitbangI2c::stop()
{
	scl_low();
	waitQuarterBit();
	sda_low();
	waitQuarterBit();
	scl_high();
	waitQuarterBit();
	// SDA goes high while SCL is high
	sda_high();
}

void BitbangI2c::sendBit(bool val)
{
	waitQuarterBit();

	// Write the bit (write while SCL is low)
	if (val)
	{
		sda_high();
	} else
	{
		sda_low();
	}

	// Data setup time (~100ns min)
	waitQuarterBit();

	// Strobe the clock
	scl_high();
	waitQuarterBit();
	scl_low();
	waitQuarterBit();
}

bool BitbangI2c::readBit()
{
	waitQuarterBit();

	scl_high();

	waitQuarterBit();
	waitQuarterBit();

	// Read just before we set the clock low (ie, as late as possible)
	bool val = palReadLine(m_sda);

	scl_low();
	waitQuarterBit();

	return val;
}

bool BitbangI2c::writeByte(uint8_t data)
{
	// write out 8 data bits
	for (size_t i = 0; i < 8; i++)
	{
		// Send the MSB
		sendBit((data & 0x80) != 0);

		data = data << 1;
	}
	
	// Force a release of the data line so the slave can ACK
	sda_high();

	// Read the ack bit
	bool ackBit = readBit();

	// 0 -> ack
	// 1 -> nack
	return !ackBit;
}

uint8_t BitbangI2c::readByte(bool ack)
{
	uint8_t result = 0;

	// Read in 8 data bits
	for (size_t i = 0; i < 8; i++)
	{
		result = result << 1;

		result |= readBit() ? 1 : 0;
	}

	// 0 -> ack
	// 1 -> nack
	sendBit(!ack);

	return result;
}

void BitbangI2c::waitQuarterBit()
{
	for (size_t i = 0; i < 6; i++)
	{
		__asm__ volatile ("nop");
	}
}

void BitbangI2c::write(uint8_t addr, const uint8_t* writeData, size_t writeSize)
{
	start();

	// Address + write
	writeByte(addr << 1 | 0);

	// Write outbound bytes
	for (size_t i = 0; i < writeSize; i++)
	{
		writeByte(writeData[i]);
	}

	stop();
}

void BitbangI2c::writeRead(uint8_t addr, const uint8_t* writeData, size_t writeSize, uint8_t* readData, size_t readSize)
{
	start();

	// Address + write
	writeByte(addr << 1 | 0);

	// Write outbound bytes
	for (size_t i = 0; i < writeSize; i++)
	{
		writeByte(writeData[i]);
	}

	read(addr, readData, readSize);
}

void BitbangI2c::read(uint8_t addr, uint8_t* readData, size_t readSize)
{
	start();

	// Address + read
	writeByte(addr << 1 | 1);

	for (size_t i = 0; i < readSize - 1; i++)
	{
		// All but the last byte send ACK to indicate we're still reading
		readData[i] = readByte(true);
	}

	// last byte sends NAK to indicate we're done reading
	readData[readSize - 1] = readByte(false);

	stop();
}

uint8_t BitbangI2c::readRegister(uint8_t addr, uint8_t reg)
{
	uint8_t retval;

	writeRead(addr, &reg, 1, &retval, 1);

	return retval;
}

void BitbangI2c::writeRegister(uint8_t addr, uint8_t reg, uint8_t val)
{
	uint8_t buf[2];
	buf[0] = reg;
	buf[1] = val;

	write(addr, buf, 2);
}
