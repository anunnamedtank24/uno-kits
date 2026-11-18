#ifndef SWITCH_SWITCH_3_PINS_H
#define SWITCH_SWITCH_3_PINS_H

#include "../debounced_switch.h"

namespace switches {
    // a switch module, 3 pins: GND, VCC, OUT (button, TTP223 touch, tilt, reed, Hall ... modules)
    // the module drives OUT itself: the Pin needs no pull-up (Uno::UnoPin<PIN>)
    //
    // ACTIVE_HIGH: true if OUT is HIGH while on (touch modules, most button modules), the default;
    //              false if LOW while on (some button, tilt and Hall modules): see src/week_4_SSD1306_switch_buzzer_audio/tests/2_switch.cpp
    template <class Pin, bool ACTIVE_HIGH = true, uint8_t DEBOUNCE_MS = 20>
    using Switch3Pins = DebouncedSwitch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>;
}

#endif
