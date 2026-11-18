#ifndef NEC_IR_LED_H
#define NEC_IR_LED_H

#include "../transmitter.h"

namespace NEC {
    // an infrared LED (940 nm, clear or light blue, 5 mm), what every remote has inside
    // 2 legs: long (anode) -> 100 ~ 220 Ω -> D3 (or D11), short (cathode) -> GND
    //   220 Ω: ~ 15 mA, a metre or two; 100 Ω: ~ 30 mA (the pin's limit is 40 mA: no less than 100 Ω)
    //   farther (5 ~ 10 m like a real remote): an NPN transistor (2N2222, S8050) and ~ 100 mA through 33 Ω
    // the light is invisible: a phone camera shows it flashing
    // Carrier: Uno::UnoIrCarrier<3> (D3) or <11> (D11)
    template <class Carrier>
    using IRLED = Transmitter<Carrier>;
}

#endif
