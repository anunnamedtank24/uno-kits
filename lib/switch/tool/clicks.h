#ifndef SWITCH_CLICKS_H
#define SWITCH_CLICKS_H

#include <stdint.h>

namespace switches {
    // what a button did (Clicks::update)
    enum Gesture : uint8_t {
        NOTHING,
        CLICK,           // pressed and let go once (reported when no second press followed in time)
        DOUBLE_CLICK,    // twice quickly
        MULTI_CLICK,     // 3 or more times quickly: count()
        LONG_PRESS,      // held for longMs (reported while still held)
        REPEAT,          // still held after LONG_PRESS: every repeatMs (like a held key)
        LONG_RELEASE     // let go after a LONG_PRESS
    };

    // clicks, double clicks, long presses and repeats from one button: one button, many commands
    //
    // Switch: a debounced switch of this library (DebouncedSwitch, Switch2Pins, Switch3Pins); Clicks calls its
    //         update(), so the program calls only Clicks::update()
    //   switches::Switch2Pins<Uno::UnoInputPin<2, true>> button;
    //   switches::Clicks<decltype(button)> clicks(button);
    //   ... switch (clicks.update()) { case switches::DOUBLE_CLICK: ...; }
    //
    // a single CLICK comes doubleMs after the release (it waits to see whether a second press follows);
    // setDoubleMs(0): every click at once, no double clicks
    template <class Switch>
    class Clicks {
    public:
        inline explicit Clicks(Switch &button);

        // inits the switch too
        inline void init();

        // call as often as possible; the gesture that just ended (or started: LONG_PRESS, REPEAT), else NOTHING
        inline Gesture update();

        // the presses of the last CLICK / DOUBLE_CLICK / MULTI_CLICK
        inline uint8_t count() const;

        // true while the button is down
        inline bool isPressed() const;

        // the longest gap between the clicks of a double click (default 300 ms; 0 = no double clicks)
        inline void setDoubleMs(uint16_t ms);

        // held this long = LONG_PRESS (default 800 ms), then REPEAT every repeatMs (default 150; 0 = none)
        inline void setLongMs(uint16_t ms);
        inline void setRepeatMs(uint16_t ms);

    private:
        Switch &_button;
        uint16_t _doubleMs = 300;
        uint16_t _longMs = 800;
        uint16_t _repeatMs = 150;
        uint8_t _presses = 0;          // presses of the clicks being counted
        uint8_t _count = 0;
        bool _long = false;            // the press now is a long one
        uint32_t _pressMs = 0;
        uint32_t _releaseMs = 0;
        uint32_t _repeatAtMs = 0;

        inline Gesture clicksDone();
    };

    // ================= definitions =================

    template <class Switch>
    inline Clicks<Switch>::Clicks(Switch &button) : _button(button) {
    }

    template <class Switch>
    inline void Clicks<Switch>::init() {
        _button.init();
        _presses = 0;
        _long = false;
    }

    template <class Switch>
    inline Gesture Clicks<Switch>::update() {
        uint32_t now = Switch::nowMs();
        if (_button.update()) {
            if (_button.isOn()) {
                _pressMs = now;
                _long = false;
            } else if (_long) {
                _long = false;
                _presses = 0;
                return LONG_RELEASE;
            } else {
                _presses++;
                _releaseMs = now;
                if (_doubleMs == 0) {
                    return clicksDone();
                }
            }
            return NOTHING;
        }
        if (_button.isOn()) {
            if (!_long && now - _pressMs >= _longMs) {
                // a long press ends the clicks before it
                _long = true;
                _presses = 0;
                _repeatAtMs = now + _repeatMs;
                return LONG_PRESS;
            }
            if (_long && _repeatMs != 0 && (int32_t)(now - _repeatAtMs) >= 0) {
                _repeatAtMs += _repeatMs;
                return REPEAT;
            }
            return NOTHING;
        }
        if (_presses > 0 && now - _releaseMs >= _doubleMs) {
            return clicksDone();
        }
        return NOTHING;
    }

    template <class Switch>
    inline Gesture Clicks<Switch>::clicksDone() {
        _count = _presses;
        _presses = 0;
        return _count == 1 ? CLICK : _count == 2 ? DOUBLE_CLICK : MULTI_CLICK;
    }

    template <class Switch>
    inline uint8_t Clicks<Switch>::count() const {
        return _count;
    }

    template <class Switch>
    inline bool Clicks<Switch>::isPressed() const {
        return _button.isOn();
    }

    template <class Switch>
    inline void Clicks<Switch>::setDoubleMs(uint16_t ms) {
        _doubleMs = ms;
    }

    template <class Switch>
    inline void Clicks<Switch>::setLongMs(uint16_t ms) {
        _longMs = ms;
    }

    template <class Switch>
    inline void Clicks<Switch>::setRepeatMs(uint16_t ms) {
        _repeatMs = ms;
    }
}

#endif
