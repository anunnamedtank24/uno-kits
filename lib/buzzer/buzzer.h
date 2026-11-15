#ifndef BUZZER_H
#define BUZZER_H

// buzzers with two legs (+ and -) on one pin: entry point, or include one module only
//   module/active_buzzer.h    buzzer::ActiveBuzzer<Pin>    own oscillator: on / off, one pitch
//   module/passive_buzzer.h   buzzer::PassiveBuzzer<Tone>  no oscillator: the board makes the pitch
// both are a Sound for audio::Player (lib/audio), which plays beeps and melodies on them

#include "module/active_buzzer.h"
#include "module/passive_buzzer.h"

#endif
