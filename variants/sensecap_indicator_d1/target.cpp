#include <Arduino.h>
#include "target.h"
#include <helpers/ArduinoHelpers.h>

SenseCapD1Board board;

TCA9535 io_expander(IOEXP_I2C_ADDR);

static SPIClass lora_spi(FSPI);
IOExpanderRadioHal radio_hal(io_expander, PIN_IO_EXP_INT, lora_spi, RADIOLIB_DEFAULT_SPI_SETTINGS);

RADIO_CLASS radio = new Module(&radio_hal, IOEXP_PIN(IOEXP_LORA_NSS), IOEXP_PIN(IOEXP_LORA_DIO1), IOEXP_PIN(IOEXP_LORA_RST), IOEXP_PIN(IOEXP_LORA_BUSY));

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock rtc_clock;

EnvironmentSensorManager sensors = EnvironmentSensorManager();

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true, true);
#endif

bool radio_init() {
  rtc_clock.begin();

  // ESP32Board::begin() already brought up Wire on PIN_BOARD_SDA/SCL
  if (!io_expander.begin()) {
    MESH_DEBUG_PRINTLN("ERROR: TCA9535 IO expander not responding on I2C addr 0x%02X", IOEXP_I2C_ADDR);
    return false;
  }

  lora_spi.begin(PIN_LORA_SCLK, PIN_LORA_MISO, PIN_LORA_MOSI);

  return radio.std_init(&lora_spi);
}

void radio_service() {
  radio_hal.service();
}

// used only to generate a fresh node identity on first boot, before the
// LoRa radio (a much better entropy source, see RadioNoiseListener) is
// available -- ESP32-S3 has a true hardware RNG regardless of radio state:
//    https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/random.html
class ESP_RNG : public mesh::RNG {
public:
  void random(uint8_t* dest, size_t sz) override {
    esp_fill_random(dest, sz);
  }
};

mesh::LocalIdentity radio_new_identity() {
  ESP_RNG rng;
  return mesh::LocalIdentity(&rng);  // create new random identity
}
