#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <helpers/ESP32Board.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/ui/MomentaryButton.h>

#include "pins.h"
#include "TCA9535.h"
#include "IOExpanderRadioHal.h"

#ifdef DISPLAY_CLASS
  #include "IndicatorDisplay.h"
#endif

extern ESP32Board board;
extern WRAPPER_CLASS radio_driver;
extern ESP32RTCClock rtc_clock;
extern TCA9535 io_expander;
extern IOExpanderRadioHal radio_hal;

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
  extern MomentaryButton user_btn;
#endif

bool radio_init();
mesh::LocalIdentity radio_new_identity();

// must be called from the sketch's loop(), as often as possible -- lets the
// LoRa radio Hal poll the IO-expander for DIO1 (see IOExpanderRadioHal.h)
void radio_service();
