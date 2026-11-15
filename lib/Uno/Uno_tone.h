#ifndef UNO_TONE_H
#define UNO_TONE_H

#include "Uno.h"
#include <avr/interrupt.h>

namespace Uno {
    // a square wave (50 % duty) on any pin, e.g. a passive buzzer (buzzer::PassiveBuzzer)
    //
    // Timer1 in CTC mode: the pin flips every OCR1A + 1 timer ticks, so
    //   frequency = F_CPU / (2 * prescaler * (OCR1A + 1))
    //   prescaler 1 from 123 Hz up (error < 0.02 % at 4 kHz), 8 below (down to 16 Hz)
    //
    // D9 (OC1A) / D10 (OC1B): the timer flips the pin itself, no CPU time at all
    // any other pin: an interrupt flips it (about 2 µs each, so 20 kHz takes about 8 % of the CPU)
    //
    // Timer1 is then used by this only: no pwmSet() on D9 / D10 at the same time
    // (e.g. the BLK of Uno_ST7789 on D9), no Servo library, no Arduino tone()
    // the interrupt is defined in this header: include it in one .cpp file only
    template <uint8_t PIN>
    struct UnoTone {
        static_assert(exists(PIN), "UnoTone: PIN must be 0 ~ 19");
        static_assert(warn::SerialPin<(PIN == 0 || PIN == 1)>::ok, "");

        static constexpr uint16_t MIN_HZ = 16;
        static constexpr uint16_t MAX_HZ = 20000;

        // ---------- Tone functions ----------

        // pin LOW, timer stopped
        static inline void init();

        // starts or changes the wave; hz is kept in MIN_HZ ~ MAX_HZ, 0 = stop()
        // a change while playing keeps the wave going (no click)
        static inline void start(uint16_t hz);

        // timer stopped, pin LOW (no current through the buzzer)
        static inline void stop();

        // the frequency really played (timer ticks are whole numbers), 0 if stopped
        static inline uint16_t realHz();

        static inline uint32_t nowMs();
        static inline uint16_t readFlash16(const uint16_t *address);

    private:
        static constexpr bool HARDWARE = (PIN == 9 || PIN == 10);
    };

    namespace tone_state {
        // the pin the interrupt flips: writing its mask to PINx flips it (ATmega328P datasheet 14.2.2)
        inline volatile uint8_t *togglePort = nullptr;
        inline uint8_t toggleMask = 0;
    }

    // ================= definitions =================

    template <uint8_t PIN>
    inline void UnoTone<PIN>::init() {
        stop();
        pinOutput<PIN>();
    }

    template <uint8_t PIN>
    inline void UnoTone<PIN>::start(uint16_t hz) {
        if (hz == 0) {
            stop();
            return;
        }
        if (hz < MIN_HZ) {
            hz = MIN_HZ;
        } else if (hz > MAX_HZ) {
            hz = MAX_HZ;
        }

        // prescaler 1 if OCR1A fits in 16 bits (hz >= 123), else 8; rounded to the nearest tick
        bool slow = hz < 123;
        uint32_t ticksPerSecond = slow ? F_CPU / 16UL : F_CPU / 2UL;
        uint16_t top = (uint16_t)((ticksPerSecond + hz / 2) / hz - 1);
        uint8_t clockBits = slow ? (1 << CS11) : (1 << CS10);

        bool running = (TCCR1B & ((1 << CS12) | (1 << CS11) | (1 << CS10))) != 0;
        if (running) {
            // change on the fly: past the new top, the counter would run to 65535 first
            OCR1A = top;
            if (TCNT1 > top) {
                TCNT1 = 0;
            }
            TCCR1B = (1 << WGM12) | clockBits;
            return;
        }

        // CTC (WGM = 0100): count 0 ~ OCR1A
        TCCR1B = 0;
        TCNT1 = 0;
        OCR1A = top;
        if (HARDWARE) {
            // flip OC1A / OC1B on its compare match; OCR1B = 0 matches once per period too
            OCR1B = 0;
            TCCR1A = (PIN == 9) ? (1 << COM1A0) : (1 << COM1B0);
        } else {
            TCCR1A = 0;
            tone_state::togglePort = PIN < 8 ? &PIND : PIN < 14 ? &PINB : &PINC;
            tone_state::toggleMask = mask(PIN);
            TIFR1 = (1 << OCF1A);
            TIMSK1 |= (1 << OCIE1A);
        }
        TCCR1B = (1 << WGM12) | clockBits;
    }

    template <uint8_t PIN>
    inline void UnoTone<PIN>::stop() {
        TCCR1B = 0;
        TCCR1A = 0;
        TIMSK1 &= ~(1 << OCIE1A);
        pinLow<PIN>();
    }

    template <uint8_t PIN>
    inline uint16_t UnoTone<PIN>::realHz() {
        uint8_t clockBits = TCCR1B & ((1 << CS12) | (1 << CS11) | (1 << CS10));
        if (clockBits == 0) {
            return 0;
        }
        uint32_t ticksPerSecond = clockBits == (1 << CS10) ? F_CPU / 2UL : F_CPU / 16UL;
        uint32_t ticks = (uint32_t)OCR1A + 1;
        return (uint16_t)((ticksPerSecond + ticks / 2) / ticks);
    }

    template <uint8_t PIN>
    inline uint32_t UnoTone<PIN>::nowMs() {
        return Uno::nowMs();
    }

    template <uint8_t PIN>
    inline uint16_t UnoTone<PIN>::readFlash16(const uint16_t *address) {
        return Uno::readFlash16(address);
    }
}

// Timer1 compare A: flips the pin of UnoTone (only enabled for pins other than D9 / D10)
ISR(TIMER1_COMPA_vect) {
    *Uno::tone_state::togglePort = Uno::tone_state::toggleMask;
}

#endif
