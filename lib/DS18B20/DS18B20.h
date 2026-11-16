#ifndef DS18B20_H
#define DS18B20_H

// DS18B20 digital thermometer (1-Wire), entry point: include this file only
//   thermometer.h  DS18B20::Thermometer<Wire>: resolution, conversion, reading, CRC
// module/  the parts you buy (the same chip and code; they differ in the pull-up)
//   TO_92.h             TO92<Wire>             the bare part: add 4.7 kΩ
//   waterproof_probe.h  WaterproofProbe<Wire>  in a steel tube, 3 wires: add 4.7 kΩ
//   KY_001.h            KY001<Wire>            on a board with the 4.7 kΩ
#include "thermometer.h"
#include "module/TO_92.h"
#include "module/waterproof_probe.h"
#include "module/KY_001.h"

#endif
