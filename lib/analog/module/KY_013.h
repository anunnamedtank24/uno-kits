#ifndef ANALOG_KY_013_H
#define ANALOG_KY_013_H

#include "../thermistor.h"

namespace analog {
    // KY-013 analog temperature module: an NTC 10 kΩ (B 3950) and a 10 kΩ resistor on a board
    // 3 pins: S -> an analog pin (Uno::UnoAnalogPin<A0 ~ A5>), middle -> 5V, - -> GND
    // on most boards the NTC is between S and GND (warmer = lower reading); readings going the wrong way
    // (warming it with a finger lowers the temperature): NTC_ON_GND = false
    template <class AnalogPin, bool NTC_ON_GND = true>
    using KY013 = Thermistor<AnalogPin, NTC_ON_GND, 10000, 3950, 10000>;
}

#endif
