#ifndef DS18B20_KY_001_H
#define DS18B20_KY_001_H

#include "../thermometer.h"

namespace DS18B20 {
    // KY-001 (and similar boards): a DS18B20 on a small board with the 4.7 kΩ pull-up (and often an LED)
    // 3 pins: S = DQ, middle = VCC, - = GND (follow the letters printed on the board); no resistor to add
    template <class Wire>
    using KY001 = Thermometer<Wire>;
}

#endif
