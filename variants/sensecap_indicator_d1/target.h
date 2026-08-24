#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/ui/MomentaryButton.h>
#include <helpers/sensors/EnvironmentSensorManager.h>

#include "pins.h"
#include "TCA9535.h"
#include "IOExpanderRadioHal.h"
#include "SenseCapD1Board.h"

#ifdef DISPLAY_CLASS
  #include "IndicatorDisplay.h"
#endif

extern SenseCapD1Board board;
extern WRAPPER_CLASS radio_driver;
extern ESP32RTCClock rtc_clock;
extern TCA9535 io_expander;
extern IOExpanderRadioHal radio_hal;
extern EnvironmentSensorManager sensors;   // no sensors on this board -- exists because
                                            // helpers/ESP32Board.cpp's enterDeepSleep() expects one

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
  extern MomentaryButton user_btn;
#endif

bool radio_init();
mesh::LocalIdentity radio_new_identity();

// must be called from the sketch's loop(), as often as possible -- lets the
// LoRa radio Hal poll the IO-expander for DIO1 (see IOExpanderRadioHal.h)
void radio_service();
