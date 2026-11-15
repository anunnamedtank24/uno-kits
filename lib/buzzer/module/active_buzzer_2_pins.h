#ifndef BUZZER_ACTIVE_BUZZER_2_PINS_H
#define BUZZER_ACTIVE_BUZZER_2_PINS_H

#include "../active_buzzer.h"

namespace buzzer {
    // a bare active buzzer, 2 legs: + (the longer one) on the pin, - on GND; sounds while the pin is HIGH
    // also the 3-pin boards whose middle pin is not connected (KY-012: S = +, - = GND)
    template <class Pin>
    using ActiveBuzzer2Pins = ActiveBuzzer<Pin, true>;
}

#endif
