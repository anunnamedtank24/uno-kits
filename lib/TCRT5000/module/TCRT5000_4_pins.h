#ifndef TCRT5000_TCRT5000_4_PINS_H
#define TCRT5000_TCRT5000_4_PINS_H

#include <stdint.h>
#include "../reflector.h"

namespace TCRT5000 {
    // the 4-pin TCRT5000 boards (VCC, GND, D0, A0): D0 as on HW-006 (Reflector), and A0, the phototransistor's
    // voltage itself: how much comes back, not only "past the potentiometer's point"
    //   more reflection (white, near) = lower A0 on these boards
    //
    // AnalogPin: Uno::UnoAnalogPin<A0 ~ A5> (init, readAnalog, MAX); A0 of the board on it
    // (D0 of the board, if used: Reflector<Uno::UnoInputPin<PIN>> as for HW-006)
    //
    //   reflection(): 0 (nothing comes back) ~ MAX (all of it), smoothed
    //   setThreshold(on, off): reflecting from on, dark again under off (on > off: no flicker)
    //   with two or more of them under a car: the difference of their reflections tells how far it is off the line
    template <class AnalogPin, uint8_t SMOOTHING = 2>
    class AnalogReflector {
    public:
        inline void init();

        // one reading (~ 112 µs); true if isReflecting() changed just now
        inline bool update();

        // 0 ~ MAX, more = more light back (white, nearer)
        inline uint16_t reflection() const;

        // 0 ~ 100 of reflection() between the darkest and the brightest seen so far (learns by itself)
        inline uint8_t percent() const;

        // reflecting from on, dark under off (in reflection() units); default MAX / 2 + 50, MAX / 2 - 50
        inline void setThreshold(uint16_t on, uint16_t off);
        inline bool isReflecting() const;
        inline bool isDark() const;

    private:
        uint32_t _sum = 0;
        uint16_t _darkest = 0xFFFF;
        uint16_t _brightest = 0;
        uint16_t _on = AnalogPin::MAX / 2 + 50;
        uint16_t _off = AnalogPin::MAX / 2 - 50;
        bool _reflecting = false;
    };

    // ================= definitions =================

    template <class AnalogPin, uint8_t SMOOTHING>
    inline void AnalogReflector<AnalogPin, SMOOTHING>::init() {
        AnalogPin::init();
        uint16_t first = (uint16_t)(AnalogPin::MAX - AnalogPin::readAnalog());
        _sum = (uint32_t)first << SMOOTHING;
        _reflecting = first >= _on;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool AnalogReflector<AnalogPin, SMOOTHING>::update() {
        uint16_t reading = (uint16_t)(AnalogPin::MAX - AnalogPin::readAnalog());
        _sum = _sum - (_sum >> SMOOTHING) + reading;
        uint16_t value = reflection();
        if (value < _darkest) {
            _darkest = value;
        }
        if (value > _brightest) {
            _brightest = value;
        }
        bool next = _reflecting ? value > _off : value >= _on;
        bool changed = next != _reflecting;
        _reflecting = next;
        return changed;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint16_t AnalogReflector<AnalogPin, SMOOTHING>::reflection() const {
        return (uint16_t)(_sum >> SMOOTHING);
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint8_t AnalogReflector<AnalogPin, SMOOTHING>::percent() const {
        if (_brightest <= _darkest) {
            return 0;
        }
        return (uint8_t)((uint32_t)(reflection() - _darkest) * 100 / (_brightest - _darkest));
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline void AnalogReflector<AnalogPin, SMOOTHING>::setThreshold(uint16_t on, uint16_t off) {
        _on = on;
        _off = off < on ? off : on;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool AnalogReflector<AnalogPin, SMOOTHING>::isReflecting() const {
        return _reflecting;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool AnalogReflector<AnalogPin, SMOOTHING>::isDark() const {
        return !_reflecting;
    }
}

#endif
