#pragma once

#include "i2c_bb.h"

class Wing
{
public:
    Wing(ioline_t scl, ioline_t sda);
    void Init();

    // Returns true if the wing responds
    bool CheckAlive();

    bool CheckAliveAndReinit();

    uint8_t ReadButtons();
    void WriteLeds(uint8_t);

private:
    ioline_t m_scl;
    ioline_t m_sda;

    bool m_wasAlive = false;
};

namespace Pca9557
{
    // Reads the true state of each pin, whether an input or output.
    uint8_t Read(BitbangI2c& i2c, uint8_t offset);

    // If a pin is in output mode, 1 sets a pin to high, 0 sets it to low.
    void Write(BitbangI2c& i2c, uint8_t offset, uint8_t output);

    // Set each bit to 1 to use as an input, 0 to use as an output
    void Configure(BitbangI2c& i2c, uint8_t offset, uint8_t config);

    // Set each bit to 1 for input channels that should be inverted
    void SetInvert(BitbangI2c& i2c, uint8_t offset, uint8_t invert);

    // Get the value of the invert register
    uint8_t GetInvert(BitbangI2c& i2c, uint8_t offset);
};
