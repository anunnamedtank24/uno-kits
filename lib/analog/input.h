#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H

#include <stdint.h>

namespace analog {
    // one analog sensor on an ADC pin: smoothing, a calibrated range, percent / any range, a threshold with
    // hysteresis (like a switch), change detection; the core of the modules (potentiometer, photoresistor ...)
    //
    // AnalogPin: a struct providing these static functions (see lib/Uno/Uno_analog_pin.h, UnoAnalogPin)
    //   void init()  /  uint16_t readAnalog()   0 ~ MAX  /  uint16_t referenceMillivolts()
    //   static constexpr uint16_t MAX            (1023 on the UNO)
    //   uint32_t nowMs()
    //
    // SMOOTHING: 0 = every reading as it is; n = an average over ~ 2^n readings (exponential): steadier,
    //            slower (n = 3: ~ 8 readings, the default)
    //
    // update() reads once (~ 112 µs on the UNO) and must be called often (every loop)
    template <class AnalogPin, uint8_t SMOOTHING = 3>
    class Input {
    public:
        static_assert(SMOOTHING <= 6, "analog::Input: SMOOTHING must be 0 ~ 6");

        // reads once: the smoothing starts from there
        inline void init();

        // one reading, smoothed; true if the threshold was crossed just now
        inline bool update();

        // the last reading as it was (0 ~ MAX), and smoothed
        inline uint16_t raw() const;
        inline uint16_t value() const;

        // the smoothed value in mV (with the pin's reference)
        inline uint16_t millivolts() const;

        // ---------- range ----------

        // what counts as 0 % and 100 % (default 0, MAX); low > high reverses it (e.g. a sensor that falls
        // as its quantity rises); values outside are clipped
        inline void setRange(uint16_t low, uint16_t high);

        // learns the range from what it sees (call it, then move the knob / light the sensor end to end);
        // a reversed range (low > high, e.g. Photoresistor<..., false>) stays reversed
        inline void startLearning();
        inline void stopLearning();
        inline uint16_t rangeLow() const;
        inline uint16_t rangeHigh() const;

        // 0 ~ 100, 0 ~ 1000, and any range: map(-100, 100), map(0, 180) ...
        inline uint8_t percent() const;
        inline uint16_t permille() const;
        inline int32_t map(int32_t outLow, int32_t outHigh) const;

        // ---------- threshold (a switch made of an analog value) ----------

        // on when value() rises to on, off when it falls to off (on > off: the gap stops flicker);
        // on < off works the other way round (on when it falls); default off: on = MAX + 1
        inline void setThreshold(uint16_t on, uint16_t off);
        inline bool isOn() const;
        inline bool wasTurnedOn();             // once after each change (cleared by the call)
        inline bool wasTurnedOff();

        // ---------- change ----------

        // true if value() moved by at least delta since the last time this returned true
        // (a knob that was turned; ignores the noise below delta)
        inline bool hasChanged(uint16_t delta = 8);

    private:
        uint32_t _sum = 0;                 // value << SMOOTHING
        uint16_t _raw = 0;
        uint16_t _low = 0;
        uint16_t _high = AnalogPin::MAX;
        bool _learning = false;
        bool _reversed = false;            // learning a reversed range: _low holds the highest value seen
        uint16_t _on = AnalogPin::MAX + 1;
        uint16_t _off = AnalogPin::MAX + 1;
        bool _state = false;
        bool _turnedOn = false;
        bool _turnedOff = false;
        uint16_t _lastReported = 0;

        inline bool checkThreshold();
    };

    // ================= definitions =================

    template <class AnalogPin, uint8_t SMOOTHING>
    inline void Input<AnalogPin, SMOOTHING>::init() {
        AnalogPin::init();
        _raw = AnalogPin::readAnalog();
        _sum = (uint32_t)_raw << SMOOTHING;
        _lastReported = _raw;
        _state = false;
        checkThreshold();
        _turnedOn = false;
        _turnedOff = false;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool Input<AnalogPin, SMOOTHING>::update() {
        _raw = AnalogPin::readAnalog();
        // exponential average: sum += raw - sum / 2^n
        _sum = _sum - (_sum >> SMOOTHING) + _raw;
        if (_learning) {
            uint16_t v = value();
            uint16_t &lowest = _reversed ? _high : _low;
            uint16_t &highest = _reversed ? _low : _high;
            if (v < lowest) {
                lowest = v;
            }
            if (v > highest) {
                highest = v;
            }
        }
        return checkThreshold();
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint16_t Input<AnalogPin, SMOOTHING>::raw() const {
        return _raw;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint16_t Input<AnalogPin, SMOOTHING>::value() const {
        return (uint16_t)((_sum + ((1UL << SMOOTHING) >> 1)) >> SMOOTHING);
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint16_t Input<AnalogPin, SMOOTHING>::millivolts() const {
        return (uint16_t)(((uint32_t)value() * AnalogPin::referenceMillivolts() + AnalogPin::MAX / 2) / AnalogPin::MAX);
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline void Input<AnalogPin, SMOOTHING>::setRange(uint16_t low, uint16_t high) {
        _low = low;
        _high = high;
        _learning = false;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline void Input<AnalogPin, SMOOTHING>::startLearning() {
        if (_low != _high) {
            _reversed = _low > _high;      // low == high (learned nothing yet): the direction stays as it was
        }
        _low = value();
        _high = value();
        _learning = true;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline void Input<AnalogPin, SMOOTHING>::stopLearning() {
        _learning = false;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint16_t Input<AnalogPin, SMOOTHING>::rangeLow() const {
        return _low;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint16_t Input<AnalogPin, SMOOTHING>::rangeHigh() const {
        return _high;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint8_t Input<AnalogPin, SMOOTHING>::percent() const {
        return (uint8_t)map(0, 100);
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline uint16_t Input<AnalogPin, SMOOTHING>::permille() const {
        return (uint16_t)map(0, 1000);
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline int32_t Input<AnalogPin, SMOOTHING>::map(int32_t outLow, int32_t outHigh) const {
        if (_low == _high) {
            return outLow;
        }
        int32_t v = value();
        int32_t low = _low;
        int32_t high = _high;
        // clip into the range (either direction)
        if (low < high) {
            v = v < low ? low : v > high ? high : v;
        } else {
            v = v > low ? low : v < high ? high : v;
        }
        int32_t span = high - low;
        int32_t out = outHigh - outLow;
        // rounded: (v - low) × out / span
        int32_t num = (v - low) * out;
        int32_t q = (num + (((num < 0) != (span < 0)) ? -(span / 2) : span / 2)) / span;
        return outLow + q;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline void Input<AnalogPin, SMOOTHING>::setThreshold(uint16_t on, uint16_t off) {
        _on = on;
        _off = off;
        checkThreshold();
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool Input<AnalogPin, SMOOTHING>::isOn() const {
        return _state;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool Input<AnalogPin, SMOOTHING>::wasTurnedOn() {
        bool turned = _turnedOn;
        _turnedOn = false;
        return turned;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool Input<AnalogPin, SMOOTHING>::wasTurnedOff() {
        bool turned = _turnedOff;
        _turnedOff = false;
        return turned;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool Input<AnalogPin, SMOOTHING>::hasChanged(uint16_t delta) {
        uint16_t v = value();
        uint16_t difference = v > _lastReported ? (uint16_t)(v - _lastReported) : (uint16_t)(_lastReported - v);
        if (difference < delta) {
            return false;
        }
        _lastReported = v;
        return true;
    }

    template <class AnalogPin, uint8_t SMOOTHING>
    inline bool Input<AnalogPin, SMOOTHING>::checkThreshold() {
        if (_on > AnalogPin::MAX && _off > AnalogPin::MAX) {
            return false;                  // no threshold set
        }
        uint16_t v = value();
        bool rising = _on >= _off;         // on when it rises
        bool next = _state;
        if (!_state && (rising ? v >= _on : v <= _on)) {
            next = true;
        } else if (_state && (rising ? v <= _off : v >= _off)) {
            next = false;
        }
        if (next == _state) {
            return false;
        }
        _state = next;
        if (_state) {
            _turnedOn = true;
        } else {
            _turnedOff = true;
        }
        return true;
    }
}

#endif
