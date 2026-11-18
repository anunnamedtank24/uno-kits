#ifndef UNO_PCM_OUT_H
#define UNO_PCM_OUT_H

#include "Uno.h"
#include <avr/interrupt.h>

namespace Uno {
    // sound samples (PCM) on D9 or D10, e.g. a passive buzzer or a speaker (audio::SamplePlayer)
    //
    //   Timer1: fast PWM, 8 bits, no prescaler: 62.5 kHz on OC1A (D9) / OC1B (D10); its duty is the sample
    //           (far above hearing: the buzzer / speaker only follows the average)
    //   Timer2: CTC at the sample rate; its interrupt asks next() for the next sample (0 ~ 255, 128 = silence)
    //     rate -> 2 MHz / n (prescaler 8, n <= 256: 7813 Hz and up) or 500 kHz / n (prescaler 32: 1954 ~ 7812 Hz),
    //     n rounded: 8000 and 16000 are exact, 11025 -> 11050, 22050 -> 21978 (realRate(); convert.py uses the same)
    //
    // Timer1 and Timer2 are then used by this only: no UnoTone (Timer1), no pwmSet / UnoPwmPin on
    // D3 / D9 / D10 / D11, no Servo, no Arduino tone()
    // the interrupt is defined in this header: include it in one .cpp file only
    //
    // IDLE_HIGH: the level of the pin while stopped: false = LOW (a buzzer or speaker on the pin and GND),
    //            true = HIGH (a 3-pin module with a PNP transistor, on while LOW)
    template <uint8_t PIN, bool IDLE_HIGH = false>
    struct UnoPcmOut : UnoBoard {
        static_assert(PIN == 9 || PIN == 10, "Uno::UnoPcmOut: PIN must be 9 (OC1A) or 10 (OC1B)");

        static constexpr uint16_t MIN_RATE = 1954;
        static constexpr uint16_t MAX_RATE = 32000;

        // ---------- SampleOut functions ----------

        // pin at its idle level, timers stopped
        static inline void init();

        // starts the output: next() is called rate times a second, inside the interrupt (keep it short);
        // rate is kept in MIN_RATE ~ MAX_RATE
        static inline void start(uint16_t rate, uint8_t (*next)());

        // timers stopped, pin at its idle level
        static inline void stop();

        // the rate really used (whole timer ticks), 0 if stopped
        static inline uint16_t realRate();

        // nowMs, nowUs, delayMs, delayUs, readFlash8 / 16, stopInterrupts, restoreInterrupts: from UnoBoard

    };

    namespace pcm_out_state {
        inline uint8_t (*volatile next)() = nullptr;
        inline volatile uint8_t *duty = nullptr;       // OCR1AL or OCR1BL
    }

    // ================= definitions =================

    template <uint8_t PIN, bool IDLE_HIGH>
    inline void UnoPcmOut<PIN, IDLE_HIGH>::init() {
        stop();
        pinOutput<PIN>();
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline void UnoPcmOut<PIN, IDLE_HIGH>::start(uint16_t rate, uint8_t (*next)()) {
        if (rate < MIN_RATE) {
            rate = MIN_RATE;
        } else if (rate > MAX_RATE) {
            rate = MAX_RATE;
        }
        stop();
        pcm_out_state::next = next;
        pcm_out_state::duty = (PIN == 9) ? &OCR1AL : &OCR1BL;

        // Timer1: fast PWM 8-bit (WGM 0101), clk / 1, non-inverting output, starting at silence
        TCCR1A = (1 << WGM10) | ((PIN == 9) ? (1 << COM1A1) : (1 << COM1B1));
        if (PIN == 9) {
            OCR1A = 128;
        } else {
            OCR1B = 128;
        }
        TCNT1 = 0;
        TCCR1B = (1 << WGM12) | (1 << CS10);

        // Timer2: CTC (WGM 010), compare A interrupt at the sample rate
        uint16_t ticks = (uint16_t)((2000000UL + rate / 2) / rate);
        uint8_t clockBits = (1 << CS21);                       // clk / 8
        if (ticks > 256) {
            ticks = (uint16_t)((500000UL + rate / 2) / rate);
            clockBits = (1 << CS21) | (1 << CS20);             // clk / 32
        }
        TCCR2A = (1 << WGM21);
        TCCR2B = 0;
        TCNT2 = 0;
        OCR2A = (uint8_t)(ticks - 1);
        TIFR2 = (1 << OCF2A);
        TIMSK2 = (1 << OCIE2A);
        TCCR2B = clockBits;
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline void UnoPcmOut<PIN, IDLE_HIGH>::stop() {
        TIMSK2 &= ~(1 << OCIE2A);
        TCCR2B = 0;
        TCCR2A = 0;
        TCCR1B = 0;
        TCCR1A = 0;
        if (IDLE_HIGH) {
            pinHigh<PIN>();
        } else {
            pinLow<PIN>();
        }
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline uint16_t UnoPcmOut<PIN, IDLE_HIGH>::realRate() {
        uint8_t clockBits = TCCR2B & ((1 << CS22) | (1 << CS21) | (1 << CS20));
        if (clockBits == 0) {
            return 0;
        }
        uint32_t clock = clockBits == (1 << CS21) ? 2000000UL : 500000UL;
        uint32_t ticks = (uint32_t)OCR2A + 1;
        return (uint16_t)((clock + ticks / 2) / ticks);
    }

}

// Timer2 compare A: the next sample into the PWM duty of UnoPcmOut
ISR(TIMER2_COMPA_vect) {
    *Uno::pcm_out_state::duty = Uno::pcm_out_state::next();
}

#endif
