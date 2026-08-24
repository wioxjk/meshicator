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

// TEMP DEBUG: paints the screen a distinct solid color at each boot
// checkpoint inside radio_init(), so a freeze/crash in here is visible on
// screen even with no serial connection. Whichever color is frozen on
// screen tells us which line it died on. Safe to delete once the boot
// hang is diagnosed -- grep "TEMP DEBUG" (also in examples/channel_board/main.cpp).
static void debugStage(ColorVal color) {
  display.startFrame(color);
  display.endFrame();
}

bool radio_init() {
  rtc_clock.begin();

  debugStage(TFT_RED);       // about to talk to the TCA9535 over I2C

  // ESP32Board::begin() already brought up Wire on PIN_BOARD_SDA/SCL
  if (!io_expander.begin()) {
    MESH_DEBUG_PRINTLN("ERROR: TCA9535 IO expander not responding on I2C addr 0x%02X", IOEXP_I2C_ADDR);
    debugStage(TFT_WHITE);   // clean, expected failure: expander didn't ACK
    return false;
  }

  debugStage(TFT_ORANGE);    // expander ACK'd; about to bring up the LoRa SPI bus

  lora_spi.begin(PIN_LORA_SCLK, PIN_LORA_MISO, PIN_LORA_MOSI);

  debugStage(TFT_YELLOW);    // SPI bus up; about to run the SX1262 init sequence

  bool ok = radio.std_init(&lora_spi);

  debugStage(ok ? TFT_GREEN : TFT_WHITE);   // radio init result

  return ok;
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
