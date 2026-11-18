#ifndef NEC_KY_005_H
#define NEC_KY_005_H

#include "../transmitter.h"

namespace NEC {
    // KY-005: an infrared LED on a small board, 3 pins: S -> D3 (or D11), middle -> 5V (not connected on some
    // boards), - -> GND. Many of these boards have NO resistor: put 100 ~ 220 Ω between S and the pin
    // (see IR_LED.h for the range)
    // Carrier: Uno::UnoIrCarrier<3> (D3) or <11> (D11)
    template <class Carrier>
    using KY005 = Transmitter<Carrier>;
}

#endif
