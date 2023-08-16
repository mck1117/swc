#include "hal.h"

#include "wing.h"

Wing::Wing(ioline_t scl, ioline_t sda)
    : m_scl(scl)
    , m_sda(sda)
{
}

static constexpr bool getbit(uint8_t val, uint8_t bit)
{
    return (val >> bit) & 0x1;
}

static constexpr uint8_t pack(uint8_t num)
{
    return num;
}

static constexpr uint8_t pack(uint8_t num, bool b)
{
    return (num << 1) | (b ? 1 : 0);
}

template<typename... Ts>
static constexpr uint8_t pack(uint8_t num, bool b, Ts... args)
{
    uint8_t val = pack(num, b);
    return pack(val, args...);
}

void Wing::Init()
{
    BitbangI2c bus(m_scl, m_sda);

    // Invert no pins
    Pca9557::SetInvert(bus, 0, 0);
    Pca9557::SetInvert(bus, 1, 0);
    Pca9557::SetInvert(bus, 2, 0);

    // Chip 1:
    // Bits 4, 7 are LEDs
    // Bits 0-3 are knob
    // Bits 5, 6 are buttons
    uint8_t c1cfg = pack(0, 1, 1, 0, 1, 1, 1, 1);

    // Chip 2:
    // All bits are knob
    uint8_t c2cfg = pack(1, 1, 1, 1, 1, 1, 1, 1);

    // Chip 3:
    // Bits 1, 2, 7 are LEDs
    // Bits 0, 3, 6 are buttons
    uint8_t c3cfg = pack(0, 1, 1, 1, 1, 0, 0, 1);

    Pca9557::Configure(bus, 0, c1cfg);
    Pca9557::Configure(bus, 1, c2cfg);
    Pca9557::Configure(bus, 2, c3cfg);

    // Turn off all the LEDs
    WriteLeds(0);
}

bool Wing::CheckAlive()
{
    BitbangI2c bus(m_scl, m_sda);

    auto readBefore = Pca9557::GetInvert(bus, 2);

    // Toggle a bit for an unused output
    auto expect = readBefore ^ 0x10;
    Pca9557::SetInvert(bus, 2, expect);

    // Check that the bit changed!
    return expect == Pca9557::GetInvert(bus, 2);
}

void Wing::WriteLeds(uint8_t leds)
{
    
}

uint8_t Wing::ReadButtons()
{
    BitbangI2c bus(m_scl, m_sda);

    uint8_t c1 = Pca9557::Read(bus, 0);
    uint8_t c3 = Pca9557::Read(bus, 2);

    bool b1 = getbit(c1, 5);
    bool b2 = getbit(c1, 6);
    bool b3 = getbit(c3, 0);
    bool b4 = getbit(c3, 6);
    bool b5 = getbit(c3, 3);

    return pack(b5, b4, b3, b2, b1);
}

namespace Pca9557
{
// PCA9557 - 8.3.2.1
static constexpr uint8_t baseAddress = 0x18;

// PCA9557 - 8.3.2.2
enum class Opcode : uint8_t
{
    Input = 0x00,
    Output = 0x01,
    PolarityInversion = 0x02,
    Configuration = 0x03,
};

static void DoWrite(BitbangI2c& i2c, uint8_t offset, Opcode op, uint8_t data)
{
    auto addr = baseAddress + offset;

    i2c.writeRegister(addr, (uint8_t)op, data);
}

static uint8_t DoRead(BitbangI2c& i2c, uint8_t offset, Opcode op)
{
    auto addr = baseAddress + offset;

    return i2c.readRegister(addr, (uint8_t)op);
}

uint8_t Read(BitbangI2c& i2c, uint8_t offset)
{
    return DoRead(i2c, offset, Opcode::Input);
}

void Write(BitbangI2c& i2c, uint8_t offset, uint8_t output)
{
    DoWrite(i2c, offset, Opcode::Output, output);
}

void Configure(BitbangI2c& i2c, uint8_t offset, uint8_t config)
{
    DoWrite(i2c, offset, Opcode::Configuration, config);
}

void SetInvert(BitbangI2c& i2c, uint8_t offset, uint8_t invert)
{
    DoWrite(i2c, offset, Opcode::PolarityInversion, invert);
}

uint8_t GetInvert(BitbangI2c& i2c, uint8_t offset)
{
    return DoRead(i2c, offset, Opcode::PolarityInversion);
}
}
