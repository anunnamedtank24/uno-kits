#ifndef UNO_ONE_WIRE_H
#define UNO_ONE_WIRE_H

#include "Uno.h"

namespace Uno {
    // 1-Wire bus on one pin (e.g. the DQ pin of a DS18B20), bit-banged with the standard timings
    // used by DS18B20::Thermometer
    //
    // the line is open drain: LOW = driven by the board, HIGH = released and pulled up
    // it needs a pull-up to VCC: 4.7 kΩ (most 3-pin modules have one on the board);
    // INTERNAL_PULLUP also turns on the ~35 kΩ internal one, enough for a short wire only
    //
    // interrupts are off inside each time slot (about 70 µs per bit) so millis() cannot stretch it;
    // a whole byte takes about 0.6 ms, a reset about 1 ms
    template <uint8_t PIN, bool INTERNAL_PULLUP = true>
    struct UnoOneWire {
        static_assert(exists(PIN), "UnoOneWire: PIN must be 0 ~ 19");
        static_assert(warn::SerialPin<(PIN == 0 || PIN == 1)>::ok, "");

        // ---------- Wire functions ----------

        // releases the line
        static inline void init();

        // reset pulse; true if a device answered with a presence pulse
        // false also when the line is stuck LOW (no pull-up, short circuit)
        static inline bool reset();

        static inline void writeBit(bool bit);
        static inline bool readBit();

        // least significant bit first
        static inline void write8(uint8_t byte);
        static inline uint8_t read8();

        static inline void delayMs(uint16_t ms);
        static inline uint32_t nowMs();

    private:
        static inline void driveLow();
        static inline void release();
    };

    // ================= definitions =================

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoOneWire<PIN, INTERNAL_PULLUP>::init() {
        release();
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline bool UnoOneWire<PIN, INTERNAL_PULLUP>::reset() {
        // the released line must be HIGH, or nothing can answer
        release();
        _delay_us(5);
        if (!pinRead<PIN>()) {
            return false;
        }

        driveLow();
        _delay_us(480);

        uint8_t sreg = SREG;
        cli();
        release();
        _delay_us(70);
        bool present = !pinRead<PIN>();
        SREG = sreg;

        // rest of the presence time slot
        _delay_us(410);
        return present;
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoOneWire<PIN, INTERNAL_PULLUP>::writeBit(bool bit) {
        uint8_t sreg = SREG;
        cli();
        driveLow();
        if (bit) {
            _delay_us(6);
            release();
            _delay_us(64);
        } else {
            _delay_us(60);
            release();
            _delay_us(10);
        }
        SREG = sreg;
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline bool UnoOneWire<PIN, INTERNAL_PULLUP>::readBit() {
        uint8_t sreg = SREG;
        cli();
        driveLow();
        _delay_us(3);
        release();
        // the device holds the line LOW for a 0, sampled before 15 µs
        _delay_us(10);
        bool bit = pinRead<PIN>();
        SREG = sreg;
        _delay_us(53);
        return bit;
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoOneWire<PIN, INTERNAL_PULLUP>::write8(uint8_t byte) {
        for (uint8_t i = 0; i < 8; i++) {
            writeBit(byte & 0x01);
            byte >>= 1;
        }
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline uint8_t UnoOneWire<PIN, INTERNAL_PULLUP>::read8() {
        uint8_t byte = 0;
        for (uint8_t i = 0; i < 8; i++) {
            byte >>= 1;
            if (readBit()) {
                byte |= 0x80;
            }
        }
        return byte;
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoOneWire<PIN, INTERNAL_PULLUP>::delayMs(uint16_t ms) {
        Uno::delayMs(ms);
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline uint32_t UnoOneWire<PIN, INTERNAL_PULLUP>::nowMs() {
        return Uno::nowMs();
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoOneWire<PIN, INTERNAL_PULLUP>::driveLow() {
        // PORT low first: switching to output with the pull-up on would drive HIGH for a moment
        pinLow<PIN>();
        pinOutput<PIN>();
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoOneWire<PIN, INTERNAL_PULLUP>::release() {
        pinInput<PIN>(INTERNAL_PULLUP);
    }
}

#endif
