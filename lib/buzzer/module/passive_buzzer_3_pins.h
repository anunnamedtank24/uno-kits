#ifndef BUZZER_PASSIVE_BUZZER_3_PINS_H
#define BUZZER_PASSIVE_BUZZER_3_PINS_H

#include "../passive_buzzer.h"

namespace buzzer {
    // a passive buzzer module with a PNP transistor (S8550 / 9012), 3 pins: VCC, I/O, GND
    // the transistor is on while I/O is LOW, so the pin must rest HIGH when silent, or the coil gets DC:
    // Tone: idle HIGH (Uno::UnoTone<PIN, true>)
    template <class Tone>
    struct PassiveBuzzer3Pins : PassiveBuzzer<Tone> {
        static_assert(Tone::IDLE_LEVEL_HIGH, "PassiveBuzzer3Pins: the Tone must idle HIGH (Uno::UnoTone<PIN, true>)");
    };
}

#endif
