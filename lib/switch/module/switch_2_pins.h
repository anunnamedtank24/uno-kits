#ifndef SWITCH_SWITCH_2_PINS_H
#define SWITCH_SWITCH_2_PINS_H

#include "../debounced_switch.h"

namespace switches {
    // a bare switch, 2 legs: one on the pin, one on GND (button, reed switch, tilt switch ...)
    // a 4-leg tactile button is the same: its legs are joined in pairs, use two diagonal ones
    // nothing drives the pin while it is open: the Pin needs its internal pull-up (Uno::UnoPin<PIN, true>),
    // so it reads HIGH open and LOW closed (on = LOW)
    template <class Pin, uint8_t DEBOUNCE_MS = 20>
    using Switch2Pins = DebouncedSwitch<Pin, false, DEBOUNCE_MS>;
}

#endif
