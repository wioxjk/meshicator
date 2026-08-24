#pragma once

#include <stdint.h>
#include <Wire.h>

/**
 * Minimal driver for the TCA9535 16-bit I2C IO expander used on the
 * SenseCAP Indicator D1 to carry the SX1262 LoRa radio's control lines
 * (NSS, RESET, BUSY, DIO1) plus the LCD chip-select/reset lines.
 *
 * Register map (TCA9535, 2x 8-bit ports -> 16 logical pins 0..15):
 *   0/1 = Input Port 0/1     (read-only, current pin state)
 *   2/3 = Output Port 0/1    (read/write, driven level for OUTPUT pins)
 *   4/5 = Polarity Inversion (unused, left at reset default)
 *   6/7 = Configuration 0/1  (1 = INPUT, 0 = OUTPUT, matches Arduino INPUT/OUTPUT sense)
 */
class TCA9535 {
  TwoWire* _wire;
  uint8_t _addr;

  // shadow copies so writes don't need a read-modify-write over I2C
  uint16_t _config = 0xFFFF;  // all pins default to INPUT until configured
  uint16_t _output = 0xFFFF;  // outputs default HIGH (matches chip reset state)

  bool writeReg16(uint8_t reg, uint16_t value) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->write((uint8_t)(value & 0xFF));
    _wire->write((uint8_t)(value >> 8));
    return _wire->endTransmission() == 0;
  }

  bool readReg16(uint8_t reg, uint16_t& value) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) return false;
    if (_wire->requestFrom((int)_addr, 2) != 2) return false;
    uint8_t lo = _wire->read();
    uint8_t hi = _wire->read();
    value = ((uint16_t)hi << 8) | lo;
    return true;
  }

public:
  TCA9535(uint8_t i2c_addr, TwoWire& wire = Wire) : _wire(&wire), _addr(i2c_addr) { }

  // returns true if the chip ACKs on the bus
  bool begin() {
    _wire->beginTransmission(_addr);
    return _wire->endTransmission() == 0;
  }

  void pinMode(uint8_t pin, uint8_t mode) {
    if (pin > 15) return;
    if (mode == OUTPUT) {
      _config &= ~(1U << pin);
    } else {
      _config |= (1U << pin);
    }
    writeReg16(0x06, _config);
  }

  void digitalWrite(uint8_t pin, uint8_t level) {
    if (pin > 15) return;
    if (level) {
      _output |= (1U << pin);
    } else {
      _output &= ~(1U << pin);
    }
    writeReg16(0x02, _output);
  }

  // reads the live Input Port registers (works for both INPUT and OUTPUT pins)
  int digitalRead(uint8_t pin) {
    if (pin > 15) return LOW;
    uint16_t in;
    if (!readReg16(0x00, in)) return LOW;
    return (in & (1U << pin)) ? HIGH : LOW;
  }

  // bulk read of both input port bytes in one I2C transaction, for polling
  // several expander pins (e.g. LoRa BUSY/DIO1) without repeated overhead
  bool readAllInputs(uint16_t& value) {
    return readReg16(0x00, value);
  }
};
