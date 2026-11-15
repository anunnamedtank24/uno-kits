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
        static inline uint16_t readFlash16(const uint16_t *address);
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
    inline uint16_t UnoOutputPin<PIN>::readFlash16(const uint16_t *address) {
        return Uno::readFlash16(address);
    }
}

#endif
