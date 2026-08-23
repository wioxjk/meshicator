#pragma once

#include <helpers/ui/LGFXDisplay.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

#include "pins.h"

// 480x480 ST7701S RGB panel + FT5x06/FT6336U touch, driven directly by the
// ESP32-S3 (no RP2040 involved). Panel/bus/touch timing below is identical
// to MeshCore's variants/sensecap_indicator-espnow/SCIndicatorDisplay.h --
// same physical board, this variant just adds the LoRa radio.
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7701 _panel_instance;
  lgfx::Bus_RGB _bus_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_FT5x06 _touch_instance;

public:
  const uint16_t screenWidth = 480;
  const uint16_t screenHeight = 480;

  bool hasButton(void) { return true; }

  LGFX(void)
  {
    {
        auto cfg = _panel_instance.config();
        cfg.memory_width = 480;
        cfg.memory_height = 480;
        cfg.panel_width = screenWidth;
        cfg.panel_height = screenHeight;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 1;
        _panel_instance.config(cfg);
    }

    {
        auto cfg = _panel_instance.config_detail();
        cfg.pin_cs = 4 | IO_EXPANDER;
        cfg.pin_sclk = 41;
        cfg.pin_mosi = 48;
        cfg.use_psram = 1;
        _panel_instance.config_detail(cfg);
    }

    {
        auto cfg = _bus_instance.config();
        cfg.panel = &_panel_instance;

        cfg.freq_write = 8000000;
        cfg.pin_henable = 18;

        cfg.pin_pclk = 21;
        cfg.pclk_active_neg = 0;
        cfg.pclk_idle_high = 0;
        cfg.de_idle_high = 1;

        cfg.pin_hsync = 16;
        cfg.hsync_polarity = 0;
        cfg.hsync_front_porch = 10;
        cfg.hsync_pulse_width = 8;
        cfg.hsync_back_porch = 50;

        cfg.pin_vsync = 17;
        cfg.vsync_polarity = 0;
        cfg.vsync_front_porch = 10;
        cfg.vsync_pulse_width = 8;
        cfg.vsync_back_porch = 20;

        cfg.pin_d0 = 15;
        cfg.pin_d1 = 14;
        cfg.pin_d2 = 13;
        cfg.pin_d3 = 12;
        cfg.pin_d4 = 11;
        cfg.pin_d5 = 10;
        cfg.pin_d6 = 9;
        cfg.pin_d7 = 8;
        cfg.pin_d8 = 7;
        cfg.pin_d9 = 6;
        cfg.pin_d10 = 5;
        cfg.pin_d11 = 4;
        cfg.pin_d12 = 3;
        cfg.pin_d13 = 2;
        cfg.pin_d14 = 1;
        cfg.pin_d15 = 0;

        _bus_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);

    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = 45;
        _light_instance.config(cfg);
    }
    _panel_instance.light(&_light_instance);

    {
        auto cfg = _touch_instance.config();
        cfg.pin_cs = GPIO_NUM_NC;
        cfg.x_min = 0;
        cfg.x_max = 479;
        cfg.y_min = 0;
        cfg.y_max = 479;
        cfg.pin_int = GPIO_NUM_NC;
        cfg.pin_rst = GPIO_NUM_NC;
        cfg.bus_shared = true;
        cfg.offset_rotation = 0;

        cfg.i2c_port = 0;
        cfg.i2c_addr = 0x48;
        cfg.pin_sda = PIN_BOARD_SDA;
        cfg.pin_scl = PIN_BOARD_SCL;
        cfg.freq = 400000;
        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

/**
 * Adds a couple of higher-level drawing helpers on top of LGFXDisplay's
 * plain DisplayDriver primitives -- centered word-wrap in a real
 * proportional font, a rounded "card", and soft dot pagination -- used by
 * the channel_board example to keep its screen calm and easy to read
 * from a few metres away, rather than the small fixed-width font used by
 * MeshCore's generic status screens.
 */
class IndicatorDisplay : public LGFXDisplay {
  LGFX disp;
public:
  IndicatorDisplay() : LGFXDisplay(480, 480, disp) {}

  LGFX_Sprite& canvas() { return buffer; }

  void fillRoundRect(int x, int y, int w, int h, int r, ColorVal c) {
    buffer.fillRoundRect(x, y, w, h, r, c);
  }

  void fillCircle(int x, int y, int r, ColorVal c) {
    buffer.fillCircle(x, y, r, c);
  }

  // draws `str` centered in a box, word-wrapped, using whichever font/size
  // was set on the sprite beforehand. Returns the height actually used.
  int drawWrappedCentered(const char* str, int box_x, int box_y, int box_w, int box_h, int line_height) {
    static const int MAX_LINES = 8;
    char lines[MAX_LINES][80];
    int num_lines = wrapText(str, box_w, lines, MAX_LINES);

    int total_h = num_lines * line_height;
    int start_y = box_y + (box_h - total_h) / 2;
    if (start_y < box_y) start_y = box_y;

    for (int n = 0; n < num_lines; n++) {
      int w = buffer.textWidth(lines[n]);
      buffer.setCursor(box_x + (box_w - w) / 2, start_y + n * line_height);
      buffer.print(lines[n]);
    }
    return total_h;
  }

private:
  // greedy word-wrap: fills `lines` (each up to 79 chars) with as many
  // words as fit within max_w, stops early if max_lines is reached (the
  // last line will simply be whatever was accumulated so far -- fine for
  // the short chat-style text this UI displays).
  int wrapText(const char* str, int max_w, char lines[][80], int max_lines) {
    int num_lines = 0;
    const char* word = str;
    char line[80] = "";

    while (*word && num_lines < max_lines) {
      const char* word_end = word;
      while (*word_end && *word_end != ' ') word_end++;
      int word_len = word_end - word;
      if (word_len > 78) word_len = 78;

      char candidate[80];
      if (line[0] == 0) {
        snprintf(candidate, sizeof(candidate), "%.*s", word_len, word);
      } else {
        snprintf(candidate, sizeof(candidate), "%s %.*s", line, word_len, word);
      }

      if (line[0] == 0 || buffer.textWidth(candidate) <= max_w) {
        strncpy(line, candidate, sizeof(line) - 1);
        line[sizeof(line) - 1] = 0;
        word = word_end;
        while (*word == ' ') word++;
      } else {
        strncpy(lines[num_lines++], line, 79);
        lines[num_lines - 1][79] = 0;
        line[0] = 0;   // retry this word against a fresh line
      }
    }
    if (line[0] && num_lines < max_lines) {
      strncpy(lines[num_lines++], line, 79);
      lines[num_lines - 1][79] = 0;
    }
    return num_lines;
  }
};
