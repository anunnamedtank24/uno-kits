#ifndef ULTRASONIC_MEDIAN_H
#define ULTRASONIC_MEDIAN_H

#include <stdint.h>

namespace ultrasonic {
    // a steadier distance: the median of the last N good measurements of a Ranger (HCSR04 ...)
    // one wrong echo (a stray reflection, a missed one) does not move the median; it lags ~ N / 2 measurements
    //
    // Ranger: a Ranger type of this library (everything static): using Sensor = ultrasonic::HCSR04<...>;
    //   ultrasonic::Median<Sensor, 5> distance;   init(), then update() every loop (it calls Sensor::update())
    template <class Ranger, uint8_t N = 5>
    class Median {
    public:
        static_assert(N >= 3 && N <= 15 && (N & 1) == 1, "ultrasonic::Median: N must be odd, 3 ~ 15");

        // inits the Ranger too
        inline void init();

        // true when a new measurement came (good or not)
        inline bool update();

        // the median of the last good ones, mm (0 until the first one)
        inline uint16_t distanceMm() const;

        // how many of the last N measurements were good (RESULT_OK)
        inline uint8_t goodCount() const;

        // true if the last N had at least half good ones (else: nothing in front, or something wrong)
        inline bool isValid() const;

    private:
        uint16_t _values[N] = {};
        bool _good[N] = {};
        uint8_t _next = 0;
        uint8_t _filled = 0;
        uint16_t _median = 0;
    };

    // ================= definitions =================

    template <class Ranger, uint8_t N>
    inline void Median<Ranger, N>::init() {
        Ranger::init();
        _filled = 0;
        _next = 0;
        _median = 0;
    }

    template <class Ranger, uint8_t N>
    inline bool Median<Ranger, N>::update() {
        if (!Ranger::update()) {
            return false;
        }
        bool good = Ranger::result() == RESULT_OK;
        _values[_next] = Ranger::distanceMm();
        _good[_next] = good;
        _next = (uint8_t)((_next + 1) % N);
        if (_filled < N) {
            _filled++;
        }
        // the median of the good ones (insertion sort of a copy: N is small)
        uint16_t sorted[N];
        uint8_t count = 0;
        for (uint8_t i = 0; i < _filled; i++) {
            if (!_good[i]) {
                continue;
            }
            uint16_t v = _values[i];
            uint8_t j = count;
            while (j > 0 && sorted[j - 1] > v) {
                sorted[j] = sorted[j - 1];
                j--;
            }
            sorted[j] = v;
            count++;
        }
        if (count > 0) {
            _median = sorted[count / 2];
        }
        return true;
    }

    template <class Ranger, uint8_t N>
    inline uint16_t Median<Ranger, N>::distanceMm() const {
        return _median;
    }

    template <class Ranger, uint8_t N>
    inline uint8_t Median<Ranger, N>::goodCount() const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < _filled; i++) {
            if (_good[i]) {
                count++;
            }
        }
        return count;
    }

    template <class Ranger, uint8_t N>
    inline bool Median<Ranger, N>::isValid() const {
        return _filled > 0 && goodCount() * 2 >= _filled;
    }
}

#endif
