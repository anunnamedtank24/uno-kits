#ifndef AUDIO_H
#define AUDIO_H

// melodies and their player: entry point, include this file only
//   note.h             audio::noteHz, Pitch, REST, beatMs   notes and the melody format
//   player.h           audio::Player<Sound>                 tone / beep / beeps / melody / slide, never waits
//   output/selected.h  melody::*                            melodies made from audio files
// a Sound makes the sound: e.g. buzzer::ActiveBuzzer, buzzer::PassiveBuzzer (lib/buzzer)
//
// tools (lib/audio/source):
//   convert.py  .wav / .flac / .mp3 / .m4a / .ogg ... -> output/*.h, added to output/selected.h
//   check.py    checks output/selected.h

#include "note.h"
#include "player.h"
#include "output/selected.h"

#endif
