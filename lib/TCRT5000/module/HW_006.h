#ifndef TCRT5000_HW_006_H
#define TCRT5000_HW_006_H

#include "../reflector.h"

namespace TCRT5000 {
    // HW-006: line tracking board, a TCRT5000 + LM393 comparator + potentiometer + 2 LEDs, 3.3 ~ 5 V
    // 3 pins: VCC = 5V, GND = GND, OUT (some boards: D0) = any digital pin (Uno::UnoPin<PIN>, no pull-up)
    // OUT = LOW and the signal LED on while reflecting (white, near); HIGH over black or nothing
    // the other 3-pin TCRT5000 line tracking boards (KY-033 ...) are the same; if yours is the other way
    // round, use Reflector<Pin, false> (src/week_8_TCRT5000/tests/1_line_tracking.cpp tells)
    template <class Pin, uint8_t FILTER_MS = 2>
    using HW006 = Reflector<Pin, true, FILTER_MS>;
}

#endif
