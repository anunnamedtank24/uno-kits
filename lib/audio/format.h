#ifndef AUDIO_FORMAT_H
#define AUDIO_FORMAT_H

#include <stdint.h>
#include "note.h"

namespace audio {
    // the exchange formats made by source/convert.py (output/*.h), one struct per sound:
    //
    //   kind          for                    data                                      played by
    //   KIND_NOTES    a passive buzzer       steps of { pitch, ms }                    Player, StreamPlayer
    //   KIND_RHYTHM   an active buzzer       steps of { on / off, ms }                 Player, StreamPlayer
    //   KIND_PCM      a passive buzzer or    samples at `rate` per second              SamplePlayer,
    //                 a speaker (PWM)                                                  StreamSamplePlayer
    //
    //   encoding                 bytes        one step / sample
    //   NOTES_HZ_MS        (0)   4 / step     uint16 hz (0 = rest), uint16 ms            (the hand-written format)
    //   NOTES_COMPACT      (1)   2 / step     uint16: high byte MIDI note (0 = rest, 12 ~ 119), low byte 10 ms ticks
    //   RHYTHM_MS          (2)   2 / step     uint16: bit 15 = on, bits 0 ~ 14 = ms (1 ~ 32767)
    //   PCM_U8             (3)   1 / sample   0 ~ 255, 128 = silence
    //   PCM_U4             (4)   1 / 2        4 bits, low nibble first, 0 ~ 15 -> 0 ~ 255 (x 17)
    //   PCM_ADPCM          (5)   1 / 2        IMA ADPCM, 4 bits, low nibble first, starts at 0 / index 0
    //
    // a struct (e.g. melody::ode_to_joy_flash_passive_medium) has
    //   target, kind, encoding, count (steps or samples), durationMs, rate (PCM), rhythmHz (RHYTHM),
    //   dataSize, getPath() (for stream.py); and the data if made for the UNO:
    //   getNotes() (const uint16_t *, NOTES / RHYTHM) or getSamples() (const uint8_t *, PCM);
    //   PCM_ADPCM also getSteps() (const uint16_t *, the 89 steps of IMA ADPCM), also for --target stream:
    //   the decoder's table, in flash the way the board says (board.json flash.attribute, platformio.ini custom_board)

    constexpr uint8_t TARGET_FLASH = 1;     // the data is in the struct (flash)
    constexpr uint8_t TARGET_STREAM = 2;    // the data stays on the computer: stream.py sends it

    constexpr uint8_t KIND_NOTES = 0;
    constexpr uint8_t KIND_RHYTHM = 1;
    constexpr uint8_t KIND_PCM = 2;

    constexpr uint8_t NOTES_HZ_MS = 0;
    constexpr uint8_t NOTES_COMPACT = 1;
    constexpr uint8_t RHYTHM_MS = 2;
    constexpr uint8_t PCM_U8 = 3;
    constexpr uint8_t PCM_U4 = 4;
    constexpr uint8_t PCM_ADPCM = 5;

    constexpr uint8_t COMPACT_TICK_MS = 10;
    constexpr uint8_t PCM_SILENCE = 128;

    // bytes of one step (NOTES / RHYTHM)
    inline constexpr uint8_t stepBytes(uint8_t encoding);

    // one step of a NOTES / RHYTHM encoding from its words: hz (0 = silent) and ms;
    // rhythmHz: the pitch of an "on" step (an active buzzer does not care, a passive one plays it)
    inline void decodeStep(uint8_t encoding, uint16_t first, uint16_t second, uint16_t rhythmHz,
                           uint16_t &hz, uint16_t &ms);

    // IMA ADPCM: 4-bit codes -> 16-bit samples (the decoder of source/convert.py does the same)
    class Adpcm {
    public:
        // from the start; steps: the sound's getSteps() (89 values in flash); nullptr: decode() gives silence
        inline void reset(const uint16_t *steps);

        // from the start again, the same table (a sound played again)
        inline void restart();

        // the table given to reset()
        inline const uint16_t *steps() const;

        // one code (0 ~ 15) -> an 8-bit sample (0 ~ 255); readFlash16 reads the step table
        template <uint16_t (*readFlash16)(const uint16_t *)>
        inline uint8_t decode(uint8_t code);

    private:
        const uint16_t *_steps = nullptr;
        int16_t _predictor = 0;
        int8_t _index = 0;
    };

    // the step table of a sound: Clip::getSteps() for PCM_ADPCM, else nullptr
    template <class Clip>
    inline const uint16_t *stepsOf();

    // compile-time warnings: static_assert(audio::warn::Xxx<condition>::ok, "")
    namespace warn {
        template <bool> struct NotForStream { static constexpr bool ok = true; };
        template <> struct NotForStream<true> {
            __attribute__((deprecated("made with --target flash only: streams fine, but was not checked for the stream")))
            static constexpr bool ok = true;
        };

        template <bool> struct PitchLost { static constexpr bool ok = true; };
        template <> struct PitchLost<true> {
            __attribute__((deprecated("notes on a Sound without pitch (active buzzer): only the rhythm is heard")))
            static constexpr bool ok = true;
        };
    }

    // ================= definitions =================

    inline constexpr uint8_t stepBytes(uint8_t encoding) {
        return encoding == NOTES_HZ_MS ? 4 : 2;
    }

    inline void decodeStep(uint8_t encoding, uint16_t first, uint16_t second, uint16_t rhythmHz,
                           uint16_t &hz, uint16_t &ms) {
        if (encoding == NOTES_HZ_MS) {
            hz = first;
            ms = second;
        } else if (encoding == NOTES_COMPACT) {
            hz = midiHz((uint8_t)(first >> 8));
            ms = (uint16_t)((first & 0xFF) * COMPACT_TICK_MS);
        } else {
            // RHYTHM_MS
            hz = (first & 0x8000) ? rhythmHz : REST;
            ms = first & 0x7FFF;
        }
    }

    namespace adpcm {
        constexpr int8_t INDEX_CHANGES[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
    }

    inline void Adpcm::reset(const uint16_t *steps) {
        _steps = steps;
        restart();
    }

    inline void Adpcm::restart() {
        _predictor = 0;
        _index = 0;
    }

    inline const uint16_t *Adpcm::steps() const {
        return _steps;
    }

    template <class Clip>
    inline const uint16_t *stepsOf() {
        if constexpr (Clip::encoding == PCM_ADPCM) {
            return Clip::getSteps();
        } else {
            return nullptr;
        }
    }

    template <uint16_t (*readFlash16)(const uint16_t *)>
    inline uint8_t Adpcm::decode(uint8_t code) {
        if (_steps == nullptr) {
            return PCM_SILENCE;
        }
        uint16_t step = readFlash16(&_steps[_index]);
        int32_t difference = step >> 3;
        if (code & 4) {
            difference += step;
        }
        if (code & 2) {
            difference += step >> 1;
        }
        if (code & 1) {
            difference += step >> 2;
        }
        int32_t predictor = (code & 8) ? (int32_t)_predictor - difference : (int32_t)_predictor + difference;
        if (predictor > 32767) {
            predictor = 32767;
        } else if (predictor < -32768) {
            predictor = -32768;
        }
        _predictor = (int16_t)predictor;
        int8_t index = (int8_t)(_index + adpcm::INDEX_CHANGES[code & 7]);
        _index = index < 0 ? 0 : index > 88 ? 88 : index;
        return (uint8_t)((_predictor >> 8) + 128);
    }
}

#endif
