#ifndef SWITCH_H
#define SWITCH_H

// switches, entry point: include this file only ("switch" is a C++ keyword, so the namespace is switches)
//   debounced_switch.h  switches::DebouncedSwitch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>: debounce, presses, times
// module/  the parts you buy
//   switch_2_pins.h     Switch2Pins<Pin>               bare switch to GND, internal pull-up (on = LOW)
//   switch_3_pins.h     Switch3Pins<Pin, ACTIVE_HIGH>  module with GND, VCC, OUT
#include "debounced_switch.h"
#include "module/switch_2_pins.h"
#include "module/switch_3_pins.h"

#endif
