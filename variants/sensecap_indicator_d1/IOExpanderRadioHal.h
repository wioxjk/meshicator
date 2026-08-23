#pragma once

#include <RadioLib.h>
#include "TCA9535.h"
#include "pins.h"

// SX1262 control lines (NSS, RESET, BUSY, DIO1) are not on native ESP32-S3
// GPIOs on this board -- they're wired through a TCA9535 I2C IO expander
// (see README.md for the full pinout and where these numbers came from).
// We give these "pins" numbers far outside the real GPIO range so a single
// Hal can transparently route pinMode/digitalWrite/digitalRead to I2C
// instead of native GPIO, while RadioLib itself stays none the wiser.
#define IOEXP_PIN_BASE   0x8000u
#define IOEXP_PIN(n)     (IOEXP_PIN_BASE + (uint32_t)(n))
#define IOEXP_PIN_IS_EXPANDER(p)   (((p) & IOEXP_PIN_BASE) != 0 && (p) != RADIOLIB_NC)
#define IOEXP_PIN_NUM(p)           ((p) & 0xFF)

/**
 * RadioLibHal that speaks native Arduino SPI/GPIO for everything except
 * the four SX1262 control lines, which live behind a TCA9535 I2C IO
 * expander shared with the LCD chip-select/reset lines.
 *
 * IMPORTANT: RadioLib expects a real edge interrupt on DIO1 to notice
 * RX/TX-done quickly. Because DIO1 is behind the I2C expander, we can't
 * give it one -- the expander's single shared INT pin (GPIO42) covers
 * DIO1, BUSY and the TCXO-detect line all at once, and servicing it
 * requires an I2C transaction, which is not safe to do from inside a
 * GPIO ISR. Instead:
 *   - the real GPIO42 ISR only sets a flag (safe, no I2C)
 *   - service() -- which the sketch's loop() must call frequently --
 *     reads the expander over I2C when that flag is set, and invokes
 *     RadioLib's registered DIO1 callback on a LOW->HIGH transition.
 * This trades a little latency (bounded by how often loop() runs, which
 * for MeshCore is on the order of milliseconds) for I2C-in-ISR safety.
 * See README.md for why, and what to check if RX feels sluggish.
 */
class IOExpanderRadioHal : public ArduinoHal {
  TCA9535* _expander;
  uint8_t _irqPin;              // real ESP32 GPIO for the expander's shared INT line

  void (*_dio1Callback)(void) = nullptr;
  bool _lastDio1Level = false;

  static volatile bool s_expanderEventPending;
  static void IRAM_ATTR sharedIsr();

public:
  IOExpanderRadioHal(TCA9535& expander, uint8_t expanderIrqPin, SPIClass& spi, SPISettings spiSettings)
    : ArduinoHal(spi, spiSettings), _expander(&expander), _irqPin(expanderIrqPin) { }

  void init() override {
    ArduinoHal::init();
    // talk to the real ESP32 GPIO directly here, bypassing our own
    // pinMode()/attachInterrupt() overrides above (those are for RadioLib's
    // virtual expander pins, not this real one)
    ::pinMode(_irqPin, INPUT_PULLUP);
    ArduinoHal::attachInterrupt(digitalPinToInterrupt(_irqPin), sharedIsr, FALLING);
  }

  // must be called often (every main loop iteration) -- polls the expander
  // when the shared INT line fired, and dispatches RadioLib's DIO1 callback
  // on a detected LOW->HIGH edge of the LoRa DIO1 line.
  void service() {
    if (!s_expanderEventPending) return;
    s_expanderEventPending = false;

    uint16_t inputs;
    if (!_expander->readAllInputs(inputs)) return;   // I2C hiccup, try again next time

    bool dio1Now = (inputs & (1U << IOEXP_LORA_DIO1)) != 0;
    if (dio1Now && !_lastDio1Level && _dio1Callback) {
      _dio1Callback();
    }
    _lastDio1Level = dio1Now;
  }

  // NOTE: RadioLibHal's GpioModeInput/Output and GpioLevelLow/High are set
  // by ArduinoHal to Arduino's own INPUT/OUTPUT/LOW/HIGH, so these values
  // pass straight through to TCA9535 unchanged.
  void pinMode(uint32_t pin, uint32_t mode) override {
    if (!IOEXP_PIN_IS_EXPANDER(pin)) { ArduinoHal::pinMode(pin, mode); return; }
    _expander->pinMode(IOEXP_PIN_NUM(pin), (uint8_t)mode);
  }

  void digitalWrite(uint32_t pin, uint32_t value) override {
    if (!IOEXP_PIN_IS_EXPANDER(pin)) { ArduinoHal::digitalWrite(pin, value); return; }
    _expander->digitalWrite(IOEXP_PIN_NUM(pin), (uint8_t)value);
  }

  uint32_t digitalRead(uint32_t pin) override {
    if (!IOEXP_PIN_IS_EXPANDER(pin)) return ArduinoHal::digitalRead(pin);
    return (uint32_t)_expander->digitalRead(IOEXP_PIN_NUM(pin));
  }

  uint32_t pinToInterrupt(uint32_t pin) override {
    if (!IOEXP_PIN_IS_EXPANDER(pin)) return ArduinoHal::pinToInterrupt(pin);
    return pin;   // identity map -- attachInterrupt()/detachInterrupt() below know what this means
  }

  void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
    if (!IOEXP_PIN_IS_EXPANDER(interruptNum)) { ArduinoHal::attachInterrupt(interruptNum, interruptCb, mode); return; }
    // this is DIO1 (the only expander input RadioLib ever attaches an
    // interrupt to) -- remember the callback, service() will call it.
    _dio1Callback = interruptCb;
    _lastDio1Level = false;
  }

  void detachInterrupt(uint32_t interruptNum) override {
    if (!IOEXP_PIN_IS_EXPANDER(interruptNum)) { ArduinoHal::detachInterrupt(interruptNum); return; }
    _dio1Callback = nullptr;
  }
};
