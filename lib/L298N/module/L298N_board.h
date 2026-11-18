#ifndef L298N_L298N_BOARD_H
#define L298N_L298N_BOARD_H

#include "../bridge.h"

namespace L298N {
    // the common red L298N board (often printed HW-095): the chip on a heat sink, a 5 V regulator (78M05),
    // 8 protection diodes, screw terminals and 2 jumpers on ENA / ENB, 1 jumper on the regulator (5V-EN)
    //
    //   terminal   what                                    to the UNO / elsewhere
    //   OUT1 OUT2  motor A (or stepper coil A)             the motor's 2 wires
    //   OUT3 OUT4  motor B (or stepper coil B)             the motor's 2 wires
    //   +12V (VS)  the motors' supply, 5 ~ 35 V            battery / adapter +   (not the UNO's 5V)
    //   GND        common ground                           battery -  AND the UNO's GND (both!)
    //   +5V        5V-EN jumper on (VS <= 12 V): 5 V out   may feed the UNO's 5V pin (~ 0.5 A at most)
    //              jumper off (VS > 12 V): 5 V in          from the UNO's 5V pin (the chip's logic needs it)
    //   ENA        channel A on / speed                    jumper on: always on; jumper off: a PWM pin
    //   IN1 IN2    channel A direction                     digital pins
    //   IN3 IN4    channel B direction                     digital pins
    //   ENB        channel B on / speed                    jumper on: always on; jumper off: a PWM pin
    //
    // In1 ~ In4: Uno::UnoOutputPin<PIN>, or Ground (that IN wired to GND: the motor goes one way only);
// Ena, Enb: Uno::UnoPwmPin<PIN> (jumper off) or EnableJumper (jumper on)
    template <class Ena, class In1, class In2, class In3, class In4, class Enb>
    struct L298NBoard {
        using A = Bridge<In1, In2, Ena>;     // OUT1, OUT2
        using B = Bridge<In3, In4, Enb>;     // OUT3, OUT4
    };

    // the same board with both EN jumpers on, speed by PWM on IN1 ~ IN4 (4 PWM pins, Uno::UnoPwmPin<PIN>;
// an IN on GND: Ground, one PWM pin fewer, that motor one way only);
    // coast is not possible, it brakes (see InputPwmBridge)
    template <class In1, class In2, class In3, class In4>
    struct L298NBoardJumpers {
        using A = InputPwmBridge<In1, In2>;
        using B = InputPwmBridge<In3, In4>;
    };
}

#endif
