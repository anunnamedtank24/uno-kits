#ifndef TCRT5000_REFLECTOR_H
#define TCRT5000_REFLECTOR_H

#include <stdint.h>

namespace TCRT5000 {
    // a TCRT5000 reflective sensor with a comparator (LM393) on a board: one digital OUT (see module/HW_006.h)
    //
    // the TCRT5000 has an infrared LED and a phototransistor side by side, looking the same way:
    // a light surface near it (white paper, ~1 ~ 25 mm) reflects the infrared back -> "reflecting";
    // black (ink, electrical tape), a hole or nothing in front reflects little -> "dark"
    // the comparator turns that into OUT; the blue potentiometer sets where reflecting starts
    //
    // Pin: a struct providing these static functions (see lib/Uno/Uno_pin.h)
    //   void init()
    //   bool read()          true = HIGH
    //   uint32_t nowMs()
    //
    // ACTIVE_LOW: true if OUT is LOW while reflecting (and the signal LED is on), the usual board
    // FILTER_MS: a new level counts only after it stayed this long (near the potentiometer's point the
    //            comparator can flicker); 0 = no filter, every change at once
    //
    // update() must be called often (every loop), it never waits
    template <class Pin, bool ACTIVE_LOW = true, uint8_t FILTER_MS = 2>
    class Reflector {
    public:
        // reads the pin once: starts in the state it is in
        inline void init();

        // call as often as possible; true if the (filtered) state changed just now
        inline bool update();

        // the filtered state: something light / near in front
        inline bool isReflecting() const;

        // the opposite: black, far or nothing (on a black line, for line following)
        inline bool isDark() const;

        // the pin right now, not filtered: true = HIGH (to find ACTIVE_LOW)
        inline bool readLevel() const;

        // true once after each change to reflecting / dark (cleared by the call)
        inline bool becameReflecting();
        inline bool becameDark();

        // changes since init() (stops at 65535)
        inline uint16_t changeCount() const;

        // how long it has been in the current state, ms
        inline uint32_t stateMs() const;

    private:
        bool _state = false;
        bool _lastRaw = false;
        uint32_t _rawSinceMs = 0;
        uint32_t _stateSinceMs = 0;
        uint16_t _changeCount = 0;
        bool _becameReflecting = false;
        bool _becameDark = false;

        inline bool readReflecting() const;
    };

    // ================= definitions =================

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline void Reflector<Pin, ACTIVE_LOW, FILTER_MS>::init() {
        Pin::init();
        _state = readReflecting();
        _lastRaw = _state;
        _rawSinceMs = Pin::nowMs();
        _stateSinceMs = _rawSinceMs;
        _changeCount = 0;
        _becameReflecting = false;
        _becameDark = false;
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline bool Reflector<Pin, ACTIVE_LOW, FILTER_MS>::update() {
        uint32_t now = Pin::nowMs();
        bool raw = readReflecting();
        if (raw != _lastRaw) {
            _lastRaw = raw;
            _rawSinceMs = now;
            if (FILTER_MS > 0) {
                return false;
            }
        }
        if (raw == _state || now - _rawSinceMs < FILTER_MS) {
            return false;
        }
        _state = raw;
        _stateSinceMs = now;
        if (_changeCount < 65535) {
            _changeCount++;
        }
        if (_state) {
            _becameReflecting = true;
        } else {
            _becameDark = true;
        }
        return true;
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline bool Reflector<Pin, ACTIVE_LOW, FILTER_MS>::isReflecting() const {
        return _state;
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline bool Reflector<Pin, ACTIVE_LOW, FILTER_MS>::isDark() const {
        return !_state;
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline bool Reflector<Pin, ACTIVE_LOW, FILTER_MS>::readLevel() const {
        return Pin::read();
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline bool Reflector<Pin, ACTIVE_LOW, FILTER_MS>::becameReflecting() {
        bool became = _becameReflecting;
        _becameReflecting = false;
        return became;
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline bool Reflector<Pin, ACTIVE_LOW, FILTER_MS>::becameDark() {
        bool became = _becameDark;
        _becameDark = false;
        return became;
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline uint16_t Reflector<Pin, ACTIVE_LOW, FILTER_MS>::changeCount() const {
        return _changeCount;
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline uint32_t Reflector<Pin, ACTIVE_LOW, FILTER_MS>::stateMs() const {
        return Pin::nowMs() - _stateSinceMs;
    }

    template <class Pin, bool ACTIVE_LOW, uint8_t FILTER_MS>
    inline bool Reflector<Pin, ACTIVE_LOW, FILTER_MS>::readReflecting() const {
        return Pin::read() != ACTIVE_LOW;
    }
}

#endif
