#pragma once

// SenseCAP Indicator D1 pin map.
//
// The LCD/touch/button/I2C pins below are shared with MeshCore's existing
// `sensecap_indicator-espnow` variant (variants/sensecap_indicator-espnow),
// which is the same physical board without the LoRa radio populated.
//
// The LoRa (SX1262) wiring is NOT in any official Seeed schematic we could
// find; it's cross-checked between two independent community
// reverse-engineering efforts that both arrived at the same pinout:
//   - fdlamotte/MCSenseCAP_Indicator
//   - ril3y/sensecap-indicator-d1l (SENSECAP_INDICATOR_WIRING.txt)
// See this variant's README.md for the full attribution and for how to
// self-test this wiring before trusting it on your own board.

// ---- I2C bus (touch panel, TCA9535 IO expander) ----
// NOTE: PIN_BOARD_SDA/PIN_BOARD_SCL are set via platformio.ini build_flags,
// not here -- helpers/ESP32Board.h's begin() (which brings up Wire.begin())
// is a header included ahead of this file, so it needs them defined via
// the compiler command line to see them at all.

// ---- TCA9535 IO expander ----
#define IOEXP_I2C_ADDR        0x20
#define PIN_IO_EXP_INT        42   // shared active-low interrupt line

// TCA9535 logical pin numbers (0..15)
#define IOEXP_LORA_NSS         0   // output: SX1262 chip select
#define IOEXP_LORA_RST         1   // output: SX1262 reset
#define IOEXP_LORA_BUSY        2   // input:  SX1262 busy
#define IOEXP_LORA_DIO1        3   // input:  SX1262 IRQ (RX/TX done)
#define IOEXP_LCD_CS           4   // output: ST7701S chip select
#define IOEXP_LCD_RST          5   // output: ST7701S reset
#define IOEXP_TOUCH_RST        7   // output: touch controller reset

// ---- LoRa radio SPI (native GPIO; control lines are on the IO expander) ----
#define PIN_LORA_SCLK         41
#define PIN_LORA_MISO         47
#define PIN_LORA_MOSI         48

// ---- misc ----
#define PIN_USER_BTN          38   // active LOW

// NOTE: P_LORA_NSS/P_LORA_DIO_1 are set via platformio.ini build_flags, not
// here -- helpers/ESP32Board.cpp #includes "ESP32Board.h" (where the inline
// getIRQGpio() uses P_LORA_DIO_1) BEFORE it #includes <target.h> (where
// this file would otherwise define it), so a plain #define here arrives
// too late for the preprocessor. See platformio.ini for the full story.
