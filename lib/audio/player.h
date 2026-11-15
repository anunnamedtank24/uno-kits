#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>
#include "note.h"

namespace audio {
    // plays sounds without waiting: start one, then call update() every loop
    // (one thing at a time: starting a new one replaces the old)
    //
    // Sound: a struct providing these static functions (e.g. buzzer::ActiveBuzzer,
    // buzzer::PassiveBuzzer of lib/buzzer)
    //   void init()
    //   void start(uint16_t hz)     sound at hz (> 0); called again to change it (a sound that has
    //                               one pitch only, like an active buzzer, just turns on)
    //   void stop()
    //   uint32_t nowMs()
    //   uint16_t readFlash16(const uint16_t *address)
    //
    // GAP_MS: the end of every melody note is silent for this long, so two equal notes
    // in a row are heard as two (notes shorter than 2 * GAP_MS are not cut)
    template <class Sound, uint8_t GAP_MS = 20>
    class Player {
    public:
        // silent
        inline void init();

        // call as often as possible: moves on to the next note / beep, never waits
        inline void update();

        // sounds until stop() or the next start
        inline void tone(uint16_t hz);

        // one sound for ms
        inline void beep(uint16_t hz, uint16_t ms);

        // count sounds of onMs, offMs apart
        inline void beeps(uint16_t hz, uint8_t count, uint16_t onMs, uint16_t offMs);

        // notes: a flash array of count pairs { hz, ms } (see note.h); repeat: from the start again
        inline void play(const uint16_t *notes, uint16_t count, bool repeat = false);

        // a melody of output/selected.h (source/convert.py): play<melody::ode_to_joy_semitone>()
        template <class Melody>
        inline void play(bool repeat = false);

        // from fromHz to toHz in ms, straight in Hz (a siren, a laser ...)
        inline void slide(uint16_t fromHz, uint16_t toHz, uint16_t ms);

        // silent now
        inline void stop();

        // true while anything is going on (also during tone() and the rests of a melody)
        inline bool isPlaying() const;

        // the hz sounding right now, 0 = silent
        inline uint16_t currentHz() const;

        // the note of the melody (or the beep) playing now: 0 = first
        inline uint16_t noteIndex() const;

    private:
        enum Mode : uint8_t { IDLE, HOLD, STEPS, SLIDE };

        Mode _mode = IDLE;
        uint16_t _hz = 0;
        uint32_t _stepStartMs = 0;
        uint16_t _stepMs = 0;
        uint16_t _soundMs = 0;

        // STEPS: a melody (_notes) or beeps (_notes = nullptr: even steps on, odd steps off)
        const uint16_t *_notes = nullptr;
        uint16_t _count = 0;
        uint16_t _index = 0;
        bool _repeat = false;
        uint16_t _beepHz = 0;
        uint16_t _onMs = 0;
        uint16_t _offMs = 0;

        // SLIDE
        uint16_t _fromHz = 0;
        uint16_t _toHz = 0;

        inline void startStep();
        inline void sound(uint16_t hz);
        inline void silence();
    };

    // ================= definitions =================

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::init() {
        Sound::init();
        _mode = IDLE;
        _hz = 0;
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::update() {
        if (_mode == IDLE || _mode == HOLD) {
            return;
        }
        uint32_t elapsed = Sound::nowMs() - _stepStartMs;

        if (_mode == SLIDE) {
            if (elapsed >= _stepMs) {
                stop();
                return;
            }
            int32_t change = ((int32_t)_toHz - (int32_t)_fromHz) * (int32_t)elapsed / (int32_t)_stepMs;
            sound((uint16_t)((int32_t)_fromHz + change));
            return;
        }

        // STEPS
        if (_hz != 0 && elapsed >= _soundMs) {
            silence();
        }
        if (elapsed < _stepMs) {
            return;
        }
        // the next step starts where this one should have ended: late updates do not slow the tempo
        _stepStartMs += _stepMs;
        _index++;
        // beeps: on, off, on, ..., on (no silence after the last one)
        uint16_t steps = _notes != nullptr ? _count : (uint16_t)(_count * 2 - 1);
        if (_index >= steps) {
            if (!_repeat) {
                stop();
                return;
            }
            _index = 0;
        }
        startStep();
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::tone(uint16_t hz) {
        if (hz == 0) {
            stop();
            return;
        }
        _mode = HOLD;
        sound(hz);
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::beep(uint16_t hz, uint16_t ms) {
        beeps(hz, 1, ms, 0);
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::beeps(uint16_t hz, uint8_t count, uint16_t onMs, uint16_t offMs) {
        if (hz == 0 || count == 0) {
            stop();
            return;
        }
        _mode = STEPS;
        _notes = nullptr;
        _count = count;
        _index = 0;
        _repeat = false;
        _beepHz = hz;
        _onMs = onMs;
        _offMs = offMs;
        _stepStartMs = Sound::nowMs();
        startStep();
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::play(const uint16_t *notes, uint16_t count, bool repeat) {
        if (notes == nullptr || count == 0) {
            stop();
            return;
        }
        _mode = STEPS;
        _notes = notes;
        _count = count;
        _index = 0;
        _repeat = repeat;
        _stepStartMs = Sound::nowMs();
        startStep();
    }

    template <class Sound, uint8_t GAP_MS>
    template <class Melody>
    inline void Player<Sound, GAP_MS>::play(bool repeat) {
        play(Melody::getNotes(), Melody::count, repeat);
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::slide(uint16_t fromHz, uint16_t toHz, uint16_t ms) {
        if (ms == 0 || fromHz == 0 || toHz == 0) {
            stop();
            return;
        }
        _mode = SLIDE;
        _fromHz = fromHz;
        _toHz = toHz;
        _stepMs = ms;
        _stepStartMs = Sound::nowMs();
        sound(fromHz);
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::stop() {
        _mode = IDLE;
        silence();
    }

    template <class Sound, uint8_t GAP_MS>
    inline bool Player<Sound, GAP_MS>::isPlaying() const {
        return _mode != IDLE;
    }

    template <class Sound, uint8_t GAP_MS>
    inline uint16_t Player<Sound, GAP_MS>::currentHz() const {
        return _hz;
    }

    template <class Sound, uint8_t GAP_MS>
    inline uint16_t Player<Sound, GAP_MS>::noteIndex() const {
        return _notes != nullptr ? _index : (uint16_t)(_index / 2);
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::startStep() {
        uint16_t hz;
        if (_notes != nullptr) {
            hz = Sound::readFlash16(&_notes[(uint32_t)_index * 2]);
            _stepMs = Sound::readFlash16(&_notes[(uint32_t)_index * 2 + 1]);
            _soundMs = _stepMs >= 2 * GAP_MS ? (uint16_t)(_stepMs - GAP_MS) : _stepMs;
        } else {
            bool on = (_index % 2) == 0;
            hz = on ? _beepHz : REST;
            _stepMs = on ? _onMs : _offMs;
            _soundMs = _stepMs;
        }
        if (hz != REST && _soundMs > 0) {
            sound(hz);
        } else {
            silence();
        }
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::sound(uint16_t hz) {
        if (hz != _hz) {
            Sound::start(hz);
            _hz = hz;
        }
    }

    template <class Sound, uint8_t GAP_MS>
    inline void Player<Sound, GAP_MS>::silence() {
        Sound::stop();
        _hz = 0;
    }
}

#endif
