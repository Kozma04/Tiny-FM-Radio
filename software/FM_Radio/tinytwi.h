#ifndef TINYTWI_H
#define TINYTWI_H

#include <avr/io.h>
#include <stdint.h>

// i2c facut acasa - il demoleaza pe Wire la viteza

enum TWI_Speed {
    STANDARD,      // 100Khz
    FAST_MODE,     // 400Khz
    FAST_MODE_PLUS // 1Mhz - fast boi (needs min. 16Mhz main clock otherwise weird stuff will happen)
};

class TimmyTWI {
public:

    TimmyTWI() {
    }

    void begin();
    void setSpeed(TWI_Speed speed);

    bool write(uint8_t val);
    uint8_t read();
    uint8_t readLast();

    bool requestFrom(uint8_t slaveAddr, int32_t size);
    bool beginTransmission(uint8_t slaveAddr, int32_t readCount = 0);
    bool restartTransmission(uint8_t slaveAddr, int32_t readCount = 0);
    void endTransmission(bool sendStopBit = true);

    void write(uint8_t *values, int16_t size);
    void read(uint8_t *values, int16_t size);

    volatile bool hasCallback = false;
    void (*transEndCallback)(void);

private:
    int32_t bytesLeft;
};

extern TimmyTWI TinyWire;
extern volatile bool TinyTWI_busy;

#endif
