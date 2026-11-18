#ifndef ANALOG_KY_023_H
#define ANALOG_KY_023_H

#include "../joystick.h"

namespace analog {
    // KY-023 (and the other 5-pin thumb joystick boards): 2 potentiometers + a push button
    // 5 pins: GND, +5V, VRx -> an analog pin, VRy -> an analog pin, SW -> a digital pin (closes to GND)
    //   XPin, YPin: Uno::UnoAnalogPin<A0 ~ A5>; ButtonPin: Uno::UnoInputPin<PIN, true> (the pull-up: SW has none)
    // with the pins on the left, up gives a lower VRy on most boards: INVERT_Y = true, the default here
    // (wrong way? flip INVERT_X / INVERT_Y)
    template <class XPin, class YPin, class ButtonPin, bool INVERT_X = false, bool INVERT_Y = true>
    using KY023 = Joystick<XPin, YPin, ButtonPin, INVERT_X, INVERT_Y>;
}

#endif
