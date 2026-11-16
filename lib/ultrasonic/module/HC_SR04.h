#ifndef ULTRASONIC_HC_SR04_H
#define ULTRASONIC_HC_SR04_H

#include "../ranger.h"

namespace ultrasonic {
    // HC-SR04 ultrasonic distance module: 2 ~ 400 cm, a cone of ~15°, 5 V
    // 4 pins: VCC = 5V, Trig = any digital pin (Trigger), Echo = any digital pin (Echo, an interrupt pin), GND = GND
    // HC-SR04P (blue board, 3 ~ 5.5 V) and RCWL-9610 boards with the same 4 pins: the same
    //
    // Trigger: Uno::UnoOutputPin<PIN>; Echo: Uno::UnoInterruptPin<PIN>
    template <class Trigger, class Echo>
    using HCSR04 = Ranger<Trigger, Echo, 20, 4000>;
}

#endif
