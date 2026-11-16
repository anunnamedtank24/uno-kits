#ifndef AUDIO_NOTE_H
#define AUDIO_NOTE_H

#include <stdint.h>

namespace audio {
    // a melody is a flash array of pairs { hz, ms, hz, ms, ... }: hz 0 = rest
    // (made from audio files by source/convert.py, or by hand with noteHz)

    // the 12 notes of an octave, S = sharp (CS = C#)
    enum Pitch : uint8_t { C, CS, D, DS, E, F, FS, G, GS, A, AS, B };

    constexpr uint16_t REST = 0;

    // equal temperament, A4 = 440 Hz, rounded to whole Hz; octave 0 ~ 8 (C4 = middle C = 262 Hz)
    // constexpr: noteHz(A, 4) in a melody costs nothing at run time
    inline constexpr uint16_t noteHz(Pitch pitch, uint8_t octave);

    // a MIDI note number -> hz (60 = C4, 69 = A4); 12 ~ 119 (C0 ~ B8), anything else 0 (rest)
    inline constexpr uint16_t midiHz(uint8_t midi);

    // length of a note at a tempo: beatMs(120, 1, 2) = an eighth when a quarter is the beat = 250 ms
    inline constexpr uint16_t beatMs(uint16_t bpm, uint8_t beats = 1, uint8_t divide = 1);

    // ================= definitions =================

    // octave 8 in 0.01 Hz: C8 = 4186.01 Hz ... B8 = 7902.13 Hz; lower octaves halve it
    constexpr uint32_t OCTAVE_8_CENTI_HZ[12] = {
        418601, 443492, 469864, 497803, 527404, 558765,
        591991, 627196, 664488, 704000, 745862, 790213,
    };

    inline constexpr uint16_t noteHz(Pitch pitch, uint8_t octave) {
        return octave > 8 ? 0
             : (uint16_t)((OCTAVE_8_CENTI_HZ[pitch] + (50UL << (8 - octave))) / (100UL << (8 - octave)));
    }

    inline constexpr uint16_t midiHz(uint8_t midi) {
        return midi < 12 || midi > 119 ? 0 : noteHz((Pitch)(midi % 12), (uint8_t)(midi / 12 - 1));
    }

    inline constexpr uint16_t beatMs(uint16_t bpm, uint8_t beats, uint8_t divide) {
        return (uint16_t)(60000UL * beats / ((uint32_t)bpm * divide));
    }
}

#endif
