#ifndef ANALOG_SOUND_LEVEL_H
#define ANALOG_SOUND_LEVEL_H

#include <stdint.h>

namespace analog {
    // how loud it is, from a microphone module's analog output (KY-037, KY-038 ...)
    //
    // the AO pin is the sound wave around a middle voltage (set by the module's potentiometer); its swing
    // (highest - lowest reading over WINDOW_MS) is the loudness: ~ 0 ~ 5 quiet, tens for talking near it,
    // hundreds for a clap right at it (these modules are not very sensitive)
    // (the module's DO pin is "louder than the potentiometer's point", on / off: read it with lib/switch)
    //
    // AnalogPin: as for Input (UnoAnalogPin); update() reads once (~ 112 µs): call it as often as possible,
    // the more readings a window gets, the better it catches the peaks
    template <class AnalogPin, uint16_t WINDOW_MS = 50>
    class SoundLevel {
    public:
        inline void init();

        // one reading; true when a window has ended (level() is new)
        inline bool update();

        // the swing of the last window, 0 ~ MAX
        inline uint16_t level() const;

        // the loudest window since the last call (cleared by the call)
        inline uint16_t peak();

        // the middle voltage (the mean of the last window): to set the module's potentiometer near MAX / 2
        inline uint16_t middle() const;

        // a clap / knock: level() rose to at least threshold (once per sound, until it falls under half)
        inline bool heard(uint16_t threshold);

    private:
        uint16_t _low = 0xFFFF;
        uint16_t _high = 0;
        uint32_t _sum = 0;
        uint16_t _readings = 0;
        uint16_t _level = 0;
        uint16_t _peak = 0;
        uint16_t _middle = 0;
        uint32_t _windowStartMs = 0;
        bool _heard = false;
    };

    // ================= definitions =================

    template <class AnalogPin, uint16_t WINDOW_MS>
    inline void SoundLevel<AnalogPin, WINDOW_MS>::init() {
        AnalogPin::init();
        _windowStartMs = AnalogPin::nowMs();
    }

    template <class AnalogPin, uint16_t WINDOW_MS>
    inline bool SoundLevel<AnalogPin, WINDOW_MS>::update() {
        uint16_t reading = AnalogPin::readAnalog();
        if (reading < _low) {
            _low = reading;
        }
        if (reading > _high) {
            _high = reading;
        }
        _sum += reading;
        _readings++;
        uint32_t now = AnalogPin::nowMs();
        if (now - _windowStartMs < WINDOW_MS) {
            return false;
        }
        _windowStartMs = now;
        _level = (uint16_t)(_high - _low);
        _middle = (uint16_t)(_sum / _readings);
        if (_level > _peak) {
            _peak = _level;
        }
        _low = 0xFFFF;
        _high = 0;
        _sum = 0;
        _readings = 0;
        return true;
    }

    template <class AnalogPin, uint16_t WINDOW_MS>
    inline uint16_t SoundLevel<AnalogPin, WINDOW_MS>::level() const {
        return _level;
    }

    template <class AnalogPin, uint16_t WINDOW_MS>
    inline uint16_t SoundLevel<AnalogPin, WINDOW_MS>::peak() {
        uint16_t peak = _peak;
        _peak = 0;
        return peak;
    }

    template <class AnalogPin, uint16_t WINDOW_MS>
    inline uint16_t SoundLevel<AnalogPin, WINDOW_MS>::middle() const {
        return _middle;
    }

    template <class AnalogPin, uint16_t WINDOW_MS>
    inline bool SoundLevel<AnalogPin, WINDOW_MS>::heard(uint16_t threshold) {
        if (!_heard && _level >= threshold) {
            _heard = true;
            return true;
        }
        if (_heard && _level < threshold / 2) {
            _heard = false;
        }
        return false;
    }
}

#endif
