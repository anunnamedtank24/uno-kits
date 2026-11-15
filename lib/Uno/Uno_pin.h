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
}

#endif
