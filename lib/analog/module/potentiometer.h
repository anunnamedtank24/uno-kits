#ifndef ANALOG_POTENTIOMETER_H
#define ANALOG_POTENTIOMETER_H

#include "../input.h"

namespace analog {
    // a potentiometer (knob, slider, trimmer; or a module such as the rotary one with 3 pins)
    // 3 legs: the outer two on 5V and GND (either way round: it only turns the direction around),
    // the middle one (the wiper) on an analog pin: Uno::UnoAnalogPin<A0 ~ A5>
    // 10 kΩ is typical (1 k ~ 100 kΩ all work; above ~ 10 kΩ the reading is a little slower to settle)
    //   percent(): 0 ~ 100 along the travel; map(0, 180): e.g. a servo angle; hasChanged(): it was turned
    template <class AnalogPin, uint8_t SMOOTHING = 3>
    using Potentiometer = Input<AnalogPin, SMOOTHING>;
}

#endif
