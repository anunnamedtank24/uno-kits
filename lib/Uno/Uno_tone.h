#ifndef UNO_TONE_H
#define UNO_TONE_H

#include "Uno.h"
#include <avr/interrupt.h>

namespace Uno {
    // a square wave on any pin with a volume, e.g. a passive buzzer (buzzer::PassiveBuzzer2Pins)
    //
    // Timer1 in fast PWM mode with ICR1 as TOP (WGM 1110): one period every ICR1 + 1 ticks, so
    //   frequency = F_CPU / (prescaler * (ICR1 + 1))
    //   prescaler 1 from 245 Hz up (error < 0.02 % at 4 kHz), 8 from 31 Hz, 64 below (down to 16 Hz)
    // the pin is "on" for the first part of the period: the duty sets the volume
    //   volume 255 = duty 50 %, the loudest a square wave gets (its fundamental is the biggest);
    //   lower: duty = asin(volume / 255) / pi, so the loudness of the fundamental follows the volume:
    //   128 = -6 dB (16.7 %), 64 = -12 dB (8.1 %), 16 = -24 dB (2.0 %), 1 = -48 dB (0.13 %); 0 = silent
    //   (the duty is whole ticks: at 20 kHz a period is 800 ticks, so the quietest steps get coarse)
    //
    // D9 (OC1A) / D10 (OC1B): the timer drives the pin itself, no CPU time at all
    // any other pin: two interrupts a period (TOP: on, compare A: off), about 2 µs each,
    // so 20 kHz takes about 16 % of the CPU; the duty comes out ~0.5 % of a period short (interrupt delay)
    // and the shortest "on" is the interrupt itself (~2 µs)
    //
    // Timer1 is then used by this only: no pwmSet() on D9 / D10 at the same time
    // (e.g. the BLK of Uno_ST7789 on D9), no Servo library, no Arduino tone(), no UnoPcmOut
    // the interrupts are defined in this header: include it in one .cpp file only
    //
    // IDLE_HIGH: the level of the pin while silent (and after stop()): false = LOW (a buzzer on the pin
    //            and GND: no DC through it), true = HIGH (a 3-pin module with a PNP transistor, on while LOW);
    //            "on" is then LOW
    template <uint8_t PIN, bool IDLE_HIGH = false>
    struct UnoTone {
        static_assert(exists(PIN), "UnoTone: PIN must be 0 ~ 19");
        static_assert(warn::SerialPin<(PIN == 0 || PIN == 1)>::ok, "");

        static constexpr uint16_t MIN_HZ = 16;
        static constexpr uint16_t MAX_HZ = 20000;

        // the level while silent (for modules that need one: buzzer::PassiveBuzzer3Pins)
        static constexpr bool IDLE_LEVEL_HIGH = IDLE_HIGH;

        // ---------- Tone functions ----------

        // pin at its idle level, timer stopped, volume 255
        static inline void init();

        // starts or changes the wave; hz is kept in MIN_HZ ~ MAX_HZ, 0 = stop()
        // a change while playing keeps the wave going (no click)
        static inline void start(uint16_t hz);

        // timer stopped, pin at its idle level (no current through the buzzer)
        static inline void stop();

        // 0 = silent ~ 255 = the loudest (duty 50 %), also while playing
        static inline void setVolume(uint8_t volume);
        static inline uint8_t volume();

        // the frequency really played (timer ticks are whole numbers), 0 if stopped
        static inline uint16_t realHz();

        // the duty really used, in 1 / 65536 of a period (32768 = 50 %), 0 if stopped
        static inline uint16_t realDuty();

        static inline uint32_t nowMs();
        static inline uint8_t readFlash8(const uint8_t *address);
        static inline uint16_t readFlash16(const uint16_t *address);

    private:
        static constexpr bool HARDWARE = (PIN == 9 || PIN == 10);

        // ticks "on" in a period of top + 1 ticks for the volume (at least 1)
        static inline uint16_t onTicks(uint16_t top);
        static inline void setOnTicks(uint16_t ticks);
    };

    namespace tone_state {
        inline uint8_t volume = 255;

        // the pin the interrupts drive (pins other than D9 / D10)
        inline volatile uint8_t *port = nullptr;
        inline uint8_t mask = 0;
        inline bool idleHigh = false;

        // duty for volume 0, 8, 16, ..., 256 (255) in 1 / 65536 of a period: asin(volume / 255) / pi
        constexpr uint16_t DUTY[33] PROGMEM = {
            0, 655, 1310, 1966, 2625, 3286, 3950, 4619, 5292, 5971, 6657, 7350, 8052, 8763, 9486, 10221,
            10970, 11735, 12518, 13322, 14149, 15004, 15891, 16815, 17784, 18808, 19899, 21078, 22375, 23840, 25577, 27869,
            32768,
        };

        // the duty of a volume, interpolated (at most 0.04 dB off), in 1 / 65536 of a period;
        // the last step is 248 ~ 255 (7 apart, the table ends at 255, not 256)
        inline uint16_t duty(uint8_t volume) {
            uint8_t index = volume >> 3;
            uint16_t low = pgm_read_word(&DUTY[index]);
            uint16_t high = pgm_read_word(&DUTY[index + 1]);
            if (index == 31) {
                return (uint16_t)(low + (uint32_t)(high - low) * (uint8_t)(volume - 248) / 7);
            }
            return (uint16_t)(low + (uint32_t)(high - low) * (volume & 7) / 8);
        }
    }

    // ================= definitions =================

    template <uint8_t PIN, bool IDLE_HIGH>
    inline void UnoTone<PIN, IDLE_HIGH>::init() {
        stop();
        pinOutput<PIN>();
        tone_state::volume = 255;
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline void UnoTone<PIN, IDLE_HIGH>::start(uint16_t hz) {
        if (hz == 0 || tone_state::volume == 0) {
            stop();
            return;
        }
        if (hz < MIN_HZ) {
            hz = MIN_HZ;
        } else if (hz > MAX_HZ) {
            hz = MAX_HZ;
        }

        // the smallest prescaler whose period fits in 16 bits; rounded to the nearest tick
        uint32_t ticksPerSecond = F_CPU;
        uint8_t clockBits = (1 << CS10);
        if (hz < 245) {
            ticksPerSecond = F_CPU / 8UL;
            clockBits = (1 << CS11);
        }
        if (hz < 31) {
            ticksPerSecond = F_CPU / 64UL;
            clockBits = (1 << CS11) | (1 << CS10);
        }
        uint16_t top = (uint16_t)((ticksPerSecond + hz / 2) / hz - 1);

        bool running = (TCCR1B & ((1 << CS12) | (1 << CS11) | (1 << CS10))) != 0;
        if (running) {
            // change on the fly: ICR1 is not buffered, past the new top the counter would run to 65535 first
            uint8_t saved = SREG;
            cli();
            ICR1 = top;
            setOnTicks(onTicks(top));
            if (TCNT1 > top) {
                TCNT1 = 0;
            }
            TCCR1B = (1 << WGM13) | (1 << WGM12) | clockBits;
            SREG = saved;
            return;
        }

        TCCR1B = 0;
        TCNT1 = 0;
        ICR1 = top;
        setOnTicks(onTicks(top));
        if (HARDWARE) {
            // OC1A / OC1B: "on" from BOTTOM to the compare match (inverted for IDLE_HIGH)
            uint8_t output = (PIN == 9) ? (1 << COM1A1) | (IDLE_HIGH ? (1 << COM1A0) : 0)
                                        : (1 << COM1B1) | (IDLE_HIGH ? (1 << COM1B0) : 0);
            TCCR1A = (1 << WGM11) | output;
        } else {
            TCCR1A = (1 << WGM11);
            tone_state::port = PIN < 8 ? &PORTD : PIN < 14 ? &PORTB : &PORTC;
            tone_state::mask = mask(PIN);
            tone_state::idleHigh = IDLE_HIGH;
            TIFR1 = (1 << OCF1A) | (1 << TOV1);
            TIMSK1 |= (1 << OCIE1A) | (1 << TOIE1);
        }
        TCCR1B = (1 << WGM13) | (1 << WGM12) | clockBits;
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline void UnoTone<PIN, IDLE_HIGH>::stop() {
        TCCR1B = 0;
        TCCR1A = 0;
        TIMSK1 &= ~((1 << OCIE1A) | (1 << TOIE1));
        if (IDLE_HIGH) {
            pinHigh<PIN>();
        } else {
            pinLow<PIN>();
        }
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline void UnoTone<PIN, IDLE_HIGH>::setVolume(uint8_t volume) {
        tone_state::volume = volume;
        bool running = (TCCR1B & ((1 << CS12) | (1 << CS11) | (1 << CS10))) != 0;
        if (!running) {
            return;
        }
        if (volume == 0) {
            stop();
            return;
        }
        uint8_t saved = SREG;
        cli();
        setOnTicks(onTicks(ICR1));
        SREG = saved;
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline uint8_t UnoTone<PIN, IDLE_HIGH>::volume() {
        return tone_state::volume;
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline uint16_t UnoTone<PIN, IDLE_HIGH>::realHz() {
        uint8_t clockBits = TCCR1B & ((1 << CS12) | (1 << CS11) | (1 << CS10));
        if (clockBits == 0) {
            return 0;
        }
        uint32_t ticksPerSecond = clockBits == (1 << CS10) ? F_CPU : clockBits == (1 << CS11) ? F_CPU / 8UL : F_CPU / 64UL;
        uint32_t ticks = (uint32_t)ICR1 + 1;
        return (uint16_t)((ticksPerSecond + ticks / 2) / ticks);
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline uint16_t UnoTone<PIN, IDLE_HIGH>::realDuty() {
        if ((TCCR1B & ((1 << CS12) | (1 << CS11) | (1 << CS10))) == 0) {
            return 0;
        }
        uint16_t on = (uint16_t)((HARDWARE && PIN == 10 ? OCR1B : OCR1A) + 1);
        return (uint16_t)(((uint32_t)on << 16) / ((uint32_t)ICR1 + 1));
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline uint16_t UnoTone<PIN, IDLE_HIGH>::onTicks(uint16_t top) {
        uint32_t ticks = ((uint32_t)top + 1) * tone_state::duty(tone_state::volume);
        uint16_t on = (uint16_t)((ticks + 32768UL) >> 16);
        return on == 0 ? 1 : on;
    }

    // the output is "on" for OCR1x + 1 ticks
    template <uint8_t PIN, bool IDLE_HIGH>
    inline void UnoTone<PIN, IDLE_HIGH>::setOnTicks(uint16_t ticks) {
        if (HARDWARE && PIN == 10) {
            OCR1B = ticks - 1;
        } else {
            OCR1A = ticks - 1;
        }
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline uint32_t UnoTone<PIN, IDLE_HIGH>::nowMs() {
        return Uno::nowMs();
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline uint8_t UnoTone<PIN, IDLE_HIGH>::readFlash8(const uint8_t *address) {
        return Uno::readFlash8(address);
    }

    template <uint8_t PIN, bool IDLE_HIGH>
    inline uint16_t UnoTone<PIN, IDLE_HIGH>::readFlash16(const uint16_t *address) {
        return Uno::readFlash16(address);
    }
}

// Timer1 at TOP: the period starts, the pin of UnoTone "on" (only for pins other than D9 / D10)
// a very short "on" (low volume): compare A came before this interrupt was done (it even runs first, it has
// the higher priority), so the "on" would stay a whole period: then "off" at once, and its interrupt dropped
ISR(TIMER1_OVF_vect) {
    if (Uno::tone_state::idleHigh) {
        *Uno::tone_state::port &= (uint8_t)~Uno::tone_state::mask;
    } else {
        *Uno::tone_state::port |= Uno::tone_state::mask;
    }
    if (TCNT1 >= OCR1A) {
        if (Uno::tone_state::idleHigh) {
            *Uno::tone_state::port |= Uno::tone_state::mask;
        } else {
            *Uno::tone_state::port &= (uint8_t)~Uno::tone_state::mask;
        }
        TIFR1 = (1 << OCF1A);
    }
}

// Timer1 compare A: the "on" part is over
ISR(TIMER1_COMPA_vect) {
    if (Uno::tone_state::idleHigh) {
        *Uno::tone_state::port |= Uno::tone_state::mask;
    } else {
        *Uno::tone_state::port &= (uint8_t)~Uno::tone_state::mask;
    }
}

#endif
