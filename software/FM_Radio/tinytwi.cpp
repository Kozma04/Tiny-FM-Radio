#include "tinytwi.h"
#include <Arduino.h>

const uint32_t f_CPU = 16000000; // do not touch this

volatile bool TinyTWI_busy;

void TimmyTWI::setSpeed(TWI_Speed speed) {
    uint16_t freq_khz;
    uint16_t t_rise;

    switch(speed) {
        case STANDARD:       freq_khz = 100,  t_rise = 1000; break;
        case FAST_MODE:      freq_khz = 400,  t_rise = 300; break;
        case FAST_MODE_PLUS: freq_khz = 1000, t_rise = 120; break;
    }

	uint32_t baud = ((f_CPU / 1000 / freq_khz) - (((f_CPU * t_rise) / 1000) / 1000) / 1000 - 10) / 2;
    TWI0.MBAUD = uint8_t(baud);
}

void TimmyTWI::begin() {
    PORTB.DIRSET &= ~(1 << 0);
    PORTB.DIRSET &= ~(1 << 1);

    //pinMode(PIN_WIRE_SDA, INPUT_PULLUP);
    //pinMode(PIN_WIRE_SCL, INPUT_PULLUP);

    setSpeed(STANDARD);
    TWI0.MCTRLA = TWI_ENABLE_bm; // Enable TWI peripheral as master w/o interrupts
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc; // Set bus state to idle
}

bool TimmyTWI::beginTransmission(uint8_t slaveAddr, int32_t readCount) {
    TinyTWI_busy = true;
    bool read;
    if (readCount == 0) read = 0; // Write
    else bytesLeft = readCount, read = 1; // Read
    TWI0.MADDR = slaveAddr << 1 | read;  // Send START condition. LSB = 0 for write

    // Wait for write or read interrupt flag
    while(!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm)));
    // Return false if arbitration lost or bus error
    if((TWI0.MSTATUS & TWI_ARBLOST_bm) || (TWI0.MSTATUS & TWI_BUSERR_bm)) return false;
    // Return true if slave gave an ACK
    return !(TWI0.MSTATUS & TWI_RXACK_bm);
}

// black magic register shit
bool TimmyTWI::write(uint8_t data) {
    TinyTWI_busy = true;
    while (!(TWI0.MSTATUS & TWI_WIF_bm)); // Wait for write interrupt flag
    TWI0.MDATA = data;
    TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc; // Do nothing
    return !(TWI0.MSTATUS & TWI_RXACK_bm); // Returns true if slave gave an ACK
}

// more black magic register shit
uint8_t TimmyTWI::read() {
    TinyTWI_busy = true;
    if (bytesLeft != 0) bytesLeft--;
    while (!(TWI0.MSTATUS & TWI_RIF_bm));                               // Wait for read interrupt flag
    uint8_t data = TWI0.MDATA;

    if (bytesLeft != 0) TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc; // Send ACK = more bytes to read
    else {
        TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_RECVTRANS_gc; // Send NACK = stop reading bytes
        if(hasCallback) transEndCallback();
        TinyTWI_busy = false;
    }
    return data;
}

// function to "skip" reading to the last byte
uint8_t TimmyTWI::readLast() {
    bytesLeft = 0;
    return read();
}

void TimmyTWI::endTransmission(bool sendStopBit) {
    if(sendStopBit) TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;
    else TWI0.MCTRLB = TWI_ACKACT_bm;
    if(hasCallback) transEndCallback();
    TinyTWI_busy = false;
}

bool TimmyTWI::requestFrom(uint8_t slaveAddr, int32_t size) {
    return beginTransmission(slaveAddr, size);
}

bool TimmyTWI::restartTransmission(uint8_t slaveAddr, int32_t readCount = 0) {
    return TimmyTWI::beginTransmission(slaveAddr, readCount);
}

void TimmyTWI::read(uint8_t *values, int16_t size) {
    while(size) {
        *values = read();
        size--, values++;
    }
}

void TimmyTWI::write(uint8_t *values, int16_t size) {
    while(size) {
        write(*values);
        size--, values++;
    }
}


TimmyTWI TinyWire;
