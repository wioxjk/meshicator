#pragma once

#include <helpers/ESP32Board.h>

/**
 * ESP32Board assumes the LoRa radio's DIO1 line is a native GPIO, and uses
 * it (via getIRQGpio()) to let the MCU light-sleep between duty cycles and
 * wake instantly on an incoming packet. On this board DIO1 sits behind the
 * TCA9535 IO expander instead (see pins.h / IOExpanderRadioHal.h), so that
 * trick genuinely isn't available -- reading or waking on it requires an
 * I2C transaction, which isn't something the ESP32's GPIO wake hardware
 * can do. Fall back to a plain timer sleep instead of pretending otherwise.
 */
class SenseCapD1Board : public ESP32Board {
public:
  uint32_t getIRQGpio() override { return (uint32_t)-1; } // not supported on this board -- see class comment

  void sleep(uint32_t secs) override {
    if (inhibit_sleep) {
      delay(1); // give any pending OTA a chance to run
      return;
    }
    if (secs > 0) delay(secs * 1000);
  }
};
