#ifndef LED_LED_2_PINS_H
#define LED_LED_2_PINS_H

#include "../single_LED.h"

namespace LED {
    // one bare LED, 2 legs: the long one (+) through 220 ~ 330 Ω to the PWM pin, the short one (-) to GND
    // (lit while the pin is HIGH)
    template <class Pwm>
    using LED2Pins = SingleLED<Pwm, true>;
}

#endif
