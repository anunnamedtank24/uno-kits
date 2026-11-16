#ifndef DS18B20_WATERPROOF_PROBE_H
#define DS18B20_WATERPROOF_PROBE_H

#include "../thermometer.h"

namespace DS18B20 {
    // the waterproof probe (a DS18B20 in a steel tube, 3 wires, often 1 m): red VDD, black GND,
    // yellow (or white / blue) DQ; check the seller's sheet, colors vary
    // no pull-up inside: put 4.7 kΩ between DQ and VDD (a long cable needs it, the internal one is too weak)
    template <class Wire>
    using WaterproofProbe = Thermometer<Wire>;
}

#endif
