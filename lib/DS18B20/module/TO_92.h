#ifndef DS18B20_TO_92_H
#define DS18B20_TO_92_H

#include "../thermometer.h"

namespace DS18B20 {
    // the bare DS18B20 (TO-92: black, flat face, 3 legs): flat face toward you, legs down: GND, DQ, VDD
    // no pull-up inside: put 4.7 kΩ between DQ and VDD yourself
    // (the internal ~35 kΩ of Uno::UnoOneWire may do for a few tens of cm, not guaranteed)
    template <class Wire>
    using TO92 = Thermometer<Wire>;
}

#endif
