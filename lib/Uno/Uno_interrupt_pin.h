#ifndef UNO_INTERRUPT_PIN_H
#define UNO_INTERRUPT_PIN_H

#include "Uno_pin.h"
#include <avr/interrupt.h>

namespace Uno {
    // one digital input pin that calls a function on every change (HIGH -> LOW and LOW -> HIGH),
    // e.g. the OUT of an infrared receiver (NEC::Receiver), whose pulses are too short to poll
    //
    // pin change interrupt (PCINT): works on every pin; the pins share three interrupts by port:
    //   D0 ~ D7 (PCINT2), D8 ~ D13 (PCINT0), A0 ~ A5 (PCINT1)
    // only one UnoInterruptPin per group: a second one in the same group replaces the first one's function
    // the interrupts are defined in this header: include it in one .cpp file only
    // an UnoInputPin with one more init: read() and the rest come from UnoInputPin / UnoBasicPin / UnoBoard
    template <uint8_t PIN, bool INTERNAL_PULLUP = false>
    struct UnoInterruptPin : UnoInputPin<PIN, INTERNAL_PULLUP> {
        // ---------- InterruptPin functions ----------

        // input; onChange() runs inside the interrupt after every change of the pin
        // (keep it short: other interrupts, e.g. millis, wait meanwhile)
        static inline void init(void (*onChange)());

        // input without a function (as UnoInputPin::init): the pin can still be read
        static inline void init() {
            UnoInputPin<PIN, INTERNAL_PULLUP>::init();
        }

        // nowMs, nowUs, delayMs, delayUs, readFlash8 / 16, stopInterrupts, restoreInterrupts: from UnoBoard

    private:
        // 0: D8 ~ D13, 1: A0 ~ A5, 2: D0 ~ D7 (the PCINT numbers of the ATmega328P)
        static constexpr uint8_t GROUP = PIN < 8 ? 2 : PIN < 14 ? 0 : 1;
    };

    namespace interrupt_pin_state {
        // the function of each group's pin
        inline void (*volatile onChange[3])() = {nullptr, nullptr, nullptr};
    }

    // ================= definitions =================

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoInterruptPin<PIN, INTERNAL_PULLUP>::init(void (*onChange)()) {
        pinInput<PIN>(INTERNAL_PULLUP);
        uint8_t sreg = SREG;
        cli();
        interrupt_pin_state::onChange[GROUP] = onChange;
        if (GROUP == 2) {
            PCMSK2 |= mask(PIN);
        } else if (GROUP == 0) {
            PCMSK0 |= mask(PIN);
        } else {
            PCMSK1 |= mask(PIN);
        }
        PCIFR = (1 << GROUP);   // forget a change from before
        PCICR |= (1 << GROUP);
        SREG = sreg;
    }

    namespace interrupt_pin_state {
        inline void call(uint8_t group) {
            void (*onChange)() = interrupt_pin_state::onChange[group];
            if (onChange != nullptr) {
                onChange();
            }
        }
    }
}

// pin change interrupts: call the function of the group's UnoInterruptPin
ISR(PCINT0_vect) {
    Uno::interrupt_pin_state::call(0);
}

ISR(PCINT1_vect) {
    Uno::interrupt_pin_state::call(1);
}

ISR(PCINT2_vect) {
    Uno::interrupt_pin_state::call(2);
}

#endif
