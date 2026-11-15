#ifndef BUZZER_H
#define BUZZER_H

// buzzers, entry point: include this file only
//   active_buzzer.h   buzzer::ActiveBuzzer<Pin, ACTIVE_HIGH>  own oscillator: on / off, one pitch
//   passive_buzzer.h  buzzer::PassiveBuzzer<Tone>             no oscillator: the board makes the pitch
// module/  the parts you buy (both are a Sound for audio::Player of lib/audio)
//   active_buzzer_2_pins.h   ActiveBuzzer2Pins<Pin>                bare, + and - (KY-012 too)
//   active_buzzer_3_pins.h   ActiveBuzzer3Pins<Pin, LOW_TRIGGER>   VCC, I/O, GND with a transistor
//   passive_buzzer_2_pins.h  PassiveBuzzer2Pins<Tone>              bare, + and - (KY-006 too)
//   passive_buzzer_3_pins.h  PassiveBuzzer3Pins<Tone>              VCC, I/O, GND with a transistor
#include "active_buzzer.h"
#include "passive_buzzer.h"
#include "module/active_buzzer_2_pins.h"
#include "module/active_buzzer_3_pins.h"
#include "module/passive_buzzer_2_pins.h"
#include "module/passive_buzzer_3_pins.h"

#endif
