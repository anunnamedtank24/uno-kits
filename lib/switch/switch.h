#ifndef SWITCH_H
#define SWITCH_H

#include <stdint.h>

// "switch" is a C++ keyword, so the namespace is switches
namespace switches {
    // a switch on one digital pin: a 3-pin module (GND, VCC, OUT: button, touch, tilt, reed ...)
    // or a bare button to GND (with the internal pull-up of the Pin)
    //
    // Pin: a struct providing these static functions (see lib/Uno/Uno_pin.h)
    //   void init()
    //   bool read()          true = HIGH
    //   uint32_t nowMs()
    //
    // ACTIVE_HIGH: true if OUT is HIGH while the switch is on (touch modules, most button modules);
    //              false if it is LOW while on (a bare button to GND, some modules): see src/week_4/test_2.cpp
    //
    // debounce: a contact bounces for a few ms when it moves; a new level counts only after it
    // stayed the same for DEBOUNCE_MS (so a change is seen DEBOUNCE_MS late)
    //
    // update() must be called often (every loop), it never waits
    template <class Pin, bool ACTIVE_HIGH = true, uint8_t DEBOUNCE_MS = 20>
    class Switch {
    public:
        // reads the pin once: the switch starts in the state it is in (no press at power on)
        inline void init();

        // call as often as possible; true if the (debounced) state changed just now
        inline bool update();

        // the debounced state
        inline bool isOn() const;

        // the pin right now, not debounced: true = HIGH (to find ACTIVE_HIGH)
        inline bool readLevel() const;

        // true once after each change to on / off (cleared by the call)
        inline bool wasPressed();
        inline bool wasReleased();

        // how many times it went on since init()
        inline uint16_t pressCount() const;

        // how long it has been in the current state, ms
        inline uint32_t stateMs() const;

        // how long the last state lasted, ms (e.g. how long the last press was, after a release)
        inline uint32_t lastStateMs() const;

    private:
        bool _state = false;
        bool _lastRaw = false;
        uint32_t _rawSinceMs = 0;
        uint32_t _stateSinceMs = 0;
        uint32_t _lastStateMs = 0;
        uint16_t _pressCount = 0;
        bool _pressed = false;
        bool _released = false;

        inline bool readOn() const;
    };

    // ================= definitions =================

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline void Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::init() {
        Pin::init();
        _state = readOn();
        _lastRaw = _state;
        _rawSinceMs = Pin::nowMs();
        _stateSinceMs = _rawSinceMs;
        _lastStateMs = 0;
        _pressCount = 0;
        _pressed = false;
        _released = false;
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline bool Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::update() {
        uint32_t now = Pin::nowMs();
        bool raw = readOn();
        if (raw != _lastRaw) {
            // moved (or bounced): wait until it stays
            _lastRaw = raw;
            _rawSinceMs = now;
            return false;
        }
        if (raw == _state || now - _rawSinceMs < DEBOUNCE_MS) {
            return false;
        }
        _state = raw;
        _lastStateMs = now - _stateSinceMs;
        _stateSinceMs = now;
        if (_state) {
            _pressed = true;
            _pressCount++;
        } else {
            _released = true;
        }
        return true;
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline bool Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::isOn() const {
        return _state;
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline bool Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::readLevel() const {
        return Pin::read();
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline bool Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::wasPressed() {
        bool pressed = _pressed;
        _pressed = false;
        return pressed;
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline bool Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::wasReleased() {
        bool released = _released;
        _released = false;
        return released;
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline uint16_t Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::pressCount() const {
        return _pressCount;
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline uint32_t Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::stateMs() const {
        return Pin::nowMs() - _stateSinceMs;
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline uint32_t Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::lastStateMs() const {
        return _lastStateMs;
    }

    template <class Pin, bool ACTIVE_HIGH, uint8_t DEBOUNCE_MS>
    inline bool Switch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>::readOn() const {
        return Pin::read() == ACTIVE_HIGH;
    }
}

#endif
