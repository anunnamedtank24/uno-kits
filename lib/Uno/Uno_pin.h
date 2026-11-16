#ifndef UNO_PIN_H
#define UNO_PIN_H

#include "Uno.h"

namespace Uno {
    // one digital input pin, e.g. the OUT of a switch module (switches::Switch)
    // INTERNAL_PULLUP: the ~35 kΩ internal pull-up, for a bare button or switch to GND
    // (modules with GND / VCC / OUT drive OUT themselves: leave it off)
    template <uint8_t PIN, bool INTERNAL_PULLUP = false>
    struct UnoPin {
        static_assert(exists(PIN), "UnoPin: PIN must be 0 ~ 19");
        static_assert(warn::SerialPin<(PIN == 0 || PIN == 1)>::ok, "");

        // ---------- Pin functions ----------

        static inline void init();

        // true = HIGH
        static inline bool read();

        static inline uint32_t nowMs();
    };

    // one digital output pin, e.g. an active buzzer (buzzer::ActiveBuzzer): LOW after init()
    // a pin gives at most 20 mA (40 mA absolute): drive anything hungrier through a transistor
    template <uint8_t PIN>
    struct UnoOutputPin {
        static_assert(exists(PIN), "UnoOutputPin: PIN must be 0 ~ 19");
        static_assert(warn::SerialPin<(PIN == 0 || PIN == 1)>::ok, "");

        // ---------- Pin functions ----------

        static inline void init();

        // true = HIGH
        static inline void write(bool high);

        static inline uint32_t nowMs();
        static inline uint8_t readFlash8(const uint8_t *address);
        static inline uint16_t readFlash16(const uint16_t *address);
    };

    // one PWM output pin (3, 5, 6, 9, 10, 11), e.g. an LED (LED::SingleLED, LED::RGBLED): LOW after init()
    // ~976 Hz, duty 0 ~ 255 (see Uno::pwmSet): D9 / D10 use Timer1 (not with UnoTone), D3 / D11 Timer2,
    // D5 / D6 Timer0 (shared with millis, fine for LEDs)
    template <uint8_t PIN>
    struct UnoPwmPin {
        static_assert(exists(PIN), "UnoPwmPin: PIN must be 0 ~ 19");
        static_assert(has(PIN, function::PWM), "UnoPwmPin: PIN must be a PWM pin (3, 5, 6, 9, 10, 11)");

        // ---------- Pwm functions ----------

        static inline void init();

        // 0 = LOW all the time, 255 = HIGH all the time
        static inline void write(uint8_t duty);
    };

    // ================= definitions =================

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoPin<PIN, INTERNAL_PULLUP>::init() {
        pinInput<PIN>(INTERNAL_PULLUP);
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline bool UnoPin<PIN, INTERNAL_PULLUP>::read() {
        return pinRead<PIN>();
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline uint32_t UnoPin<PIN, INTERNAL_PULLUP>::nowMs() {
        return Uno::nowMs();
    }

    template <uint8_t PIN>
    inline void UnoOutputPin<PIN>::init() {
        pinLow<PIN>();
        pinOutput<PIN>();
    }

    template <uint8_t PIN>
    inline void UnoOutputPin<PIN>::write(bool high) {
        if (high) {
            pinHigh<PIN>();
        } else {
            pinLow<PIN>();
        }
    }

    template <uint8_t PIN>
    inline uint32_t UnoOutputPin<PIN>::nowMs() {
        return Uno::nowMs();
    }

    template <uint8_t PIN>
    inline uint8_t UnoOutputPin<PIN>::readFlash8(const uint8_t *address) {
        return Uno::readFlash8(address);
    }

    template <uint8_t PIN>
    inline uint16_t UnoOutputPin<PIN>::readFlash16(const uint16_t *address) {
        return Uno::readFlash16(address);
    }

    template <uint8_t PIN>
    inline void UnoPwmPin<PIN>::init() {
        pwmSet<PIN>(0);
    }

    template <uint8_t PIN>
    inline void UnoPwmPin<PIN>::write(uint8_t duty) {
        pwmSet<PIN>(duty);
    }
}

#endif
