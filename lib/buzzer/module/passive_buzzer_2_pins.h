#ifndef BUZZER_PASSIVE_BUZZER_2_PINS_H
#define BUZZER_PASSIVE_BUZZER_2_PINS_H

#include "../passive_buzzer.h"

namespace buzzer {
    // a bare passive buzzer, 2 legs: + on the pin (through ~100 Ω for a magnetic one), - on GND
    // also the 3-pin boards whose middle pin is not connected (KY-006: S = +, - = GND)
    // Tone: idle LOW (Uno::UnoTone<PIN>, the default), so no DC flows through it while silent
    template <class Tone>
    struct PassiveBuzzer2Pins : PassiveBuzzer<Tone> {
        static_assert(!Tone::IDLE_LEVEL_HIGH, "PassiveBuzzer2Pins: the Tone must idle LOW (Uno::UnoTone<PIN>)");
    };
}

#endif
