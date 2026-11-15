#ifndef NEC_HX1838_H
#define NEC_HX1838_H

#include "../receiver.h"

namespace NEC {
    // HX1838 infrared receiver: a 38 kHz demodulator, OUT = LOW while the remote's light is on
    // VS1838B (another maker's same part) and the KY-022 board (an HX1838 / VS1838B on a small PCB) work the same
    //
    // the bare part, dome toward you, legs down: OUT, GND, VCC (2.7 ~ 5.5 V)
    // on a board the pins are rerouted (KY-022: S, +, -): follow the letters printed on it
    template <class InterruptPin, uint8_t QUEUE = 4>
    using HX1838 = Receiver<InterruptPin, true, QUEUE>;

    template <class InterruptPin, uint8_t QUEUE = 4>
    using VS1838B = Receiver<InterruptPin, true, QUEUE>;
}

#endif
