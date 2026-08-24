#include "IOExpanderRadioHal.h"

volatile bool IOExpanderRadioHal::s_expanderEventPending = false;

// Real GPIO ISR for the TCA9535's shared INT line. Deliberately does no
// I2C work -- just flags that service() should go and check the expander.
void IRAM_ATTR IOExpanderRadioHal::sharedIsr() {
  s_expanderEventPending = true;
}
