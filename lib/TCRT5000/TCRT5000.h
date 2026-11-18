#ifndef TCRT5000_H
#define TCRT5000_H

// TCRT5000 reflective infrared sensor (on a board with a comparator), entry point: include this file only
//   reflector.h  TCRT5000::Reflector<Pin, ACTIVE_LOW, FILTER_MS>: reflecting / dark, filtered, changes, times
// module/  the parts you buy
//   HW_006.h     HW006<Pin>  line tracking board: VCC, GND, OUT
//   TCRT5000_4_pins.h  AnalogReflector<AnalogPin>  4-pin boards: A0, how much comes back (D0: Reflector)
#include "reflector.h"
#include "module/HW_006.h"
#include "module/TCRT5000_4_pins.h"

#endif
