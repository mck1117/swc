#pragma once

#include "i2c_bb.h"

class Wing
{
public:
    Wing(ioline_t scl, ioline_t sda);
    void Init();

    uint8_t ReadButtons();
    void WriteLeds(uint8_t);

private:
    ioline_t m_scl;
    ioline_t m_sda;
};

namespace Pca9557
{
    // Reads the true state of each pin, whether an input or output.
    uint8_t Read(BitbangI2c& i2c, uint8_t offset);

    // If a pin is in output mode, 1 sets a pin to high, 0 sets it to low.
    void Write(BitbangI2c& i2c, uint8_t offset, uint8_t output);

    // Set each bit to 1 to use as an input, 0 to use as an output
    void Configure(BitbangI2c& i2c, uint8_t offset, uint8_t config);

    // Set each bit to 1 for channels that should output the inversion of the output value.
    void SetInvert(BitbangI2c& i2c, uint8_t offset, uint8_t invert);
};
