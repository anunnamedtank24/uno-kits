#ifndef BUZZER_ACTIVE_BUZZER_3_PINS_H
#define BUZZER_ACTIVE_BUZZER_3_PINS_H

#include "../active_buzzer.h"

namespace buzzer {
    // an active buzzer module with a transistor, 3 pins: VCC, I/O, GND (the buzzer gets its current
    // from VCC, the pin only switches the transistor: louder, nothing hungry on the pin)
    //
    // LOW_TRIGGER: true for the usual PNP module (S8550 / 9012): sounds while I/O is LOW, the default;
    //              false for an NPN module (S8050 / 9013): sounds while I/O is HIGH
    //              (it sounds at power on, before init()? it is LOW-trigger)
    template <class Pin, bool LOW_TRIGGER = true>
    using ActiveBuzzer3Pins = ActiveBuzzer<Pin, !LOW_TRIGGER>;
}

#endif
