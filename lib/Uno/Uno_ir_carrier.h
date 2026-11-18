#ifndef UNO_IR_CARRIER_H
#define UNO_IR_CARRIER_H

#include "Uno.h"

namespace Uno {
    // the ~ 38 kHz square wave of an infrared LED (remotes switch it on and off: "marks" and "spaces"),
    // made by Timer2 in hardware on D3 (OC2B) or D11 (OC2A): no CPU while it runs
    //
    // KHZ: the carrier (36 ~ 40 kHz for most receivers; NEC remotes: 38)
    // Timer2 is this one's while used: not with UnoPcmOut, nor PWM on D3 / D11 (UnoPwmPin<3>, <11>)
    template <uint8_t PIN = 3, uint8_t KHZ = 38>
    struct UnoIrCarrier : UnoBoard {
        static_assert(PIN == 3 || PIN == 11, "Uno::UnoIrCarrier: PIN must be 3 (OC2B) or 11 (OC2A): Timer2's outputs");
        static_assert(KHZ >= 20 && KHZ <= 60, "Uno::UnoIrCarrier: KHZ must be 20 ~ 60");

        // ---------- Carrier functions ----------

        // pin LOW (LED off), Timer2 running at 2 × KHZ (CTC), output not connected
        static inline void init();

        // the LED flashes at KHZ (a mark) / stays off (a space)
        static inline void on();
        static inline void off();

        // the frequency really made, Hz (whole timer ticks)
        static constexpr uint32_t REAL_HZ = F_CPU / (2UL * ((F_CPU + KHZ * 1000UL) / (2UL * KHZ * 1000UL)));

    private:
        // ticks of a half period, minus 1 (prescaler 1)
        static constexpr uint8_t TOP = (uint8_t)((F_CPU + KHZ * 1000UL) / (2UL * KHZ * 1000UL) - 1);
    };

    // ================= definitions =================

    template <uint8_t PIN, uint8_t KHZ>
    inline void UnoIrCarrier<PIN, KHZ>::init() {
        pinLow<PIN>();
        pinOutput<PIN>();
        // CTC (WGM 010), TOP = OCR2A, prescaler 1; the outputs are connected only during a mark
        TCCR2A = (1 << WGM21);
        TCCR2B = (1 << CS20);
        OCR2A = TOP;
        OCR2B = 0;
        TIMSK2 = 0;
    }

    template <uint8_t PIN, uint8_t KHZ>
    inline void UnoIrCarrier<PIN, KHZ>::on() {
        // toggle on compare: a square wave of TOP + 1 ticks a half
        TCNT2 = 0;
        if (PIN == 3) {
            TCCR2A = (uint8_t)((TCCR2A & ~((1 << COM2B1) | (1 << COM2B0))) | (1 << COM2B0));
        } else {
            TCCR2A = (uint8_t)((TCCR2A & ~((1 << COM2A1) | (1 << COM2A0))) | (1 << COM2A0));
        }
    }

    template <uint8_t PIN, uint8_t KHZ>
    inline void UnoIrCarrier<PIN, KHZ>::off() {
        // disconnected: the pin's own LOW (the LED never stays on)
        if (PIN == 3) {
            TCCR2A &= (uint8_t)~((1 << COM2B1) | (1 << COM2B0));
        } else {
            TCCR2A &= (uint8_t)~((1 << COM2A1) | (1 << COM2A0));
        }
        pinLow<PIN>();
    }
}

#endif
