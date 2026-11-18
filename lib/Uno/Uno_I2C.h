#ifndef UNO_I2C_H
#define UNO_I2C_H

#include "Uno.h"

namespace Uno {
    // I2C master on any two pins, bit-banged (e.g. an SSD1306 OLED on D13 / D11)
    // the UNO's hardware I2C (TWI) is only on A4 (SDA) / A5 (SCL); this one works on every pin
    //
    // both lines are open drain: LOW = driven by the board, HIGH = released and pulled up.
    // they need pull-ups to VCC: most modules have 4.7 ~ 10 kΩ on the board;
    // INTERNAL_PULLUP also turns on the ~35 kΩ internal ones, enough for a short wire only
    //
    // timing (I2C spec, fast mode): SCL LOW >= 1.3 µs, HIGH >= 0.6 µs, so at most 400 kHz.
    // after SCL is released the board waits until it really reads HIGH, so a slow rise
    // (weak pull-up) or a device holding SCL LOW (clock stretching) only slows the bus down;
    // the wait gives up after about STRETCH_LIMIT polls (~0.1 ms) and goes on
    //
    // KHZ: the clock it aims at, 1 ~ 400: the shortest LOW / HIGH times it waits.
    // the code itself adds ~56 cycles per bit (calls into the Uno.h pin helpers), so 400 gives
    // about 160 kHz on a 16 MHz UNO (measured: 1024 bytes of a 128x64 frame in 64 ms)
    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP = true, uint16_t KHZ = 400>
    struct UnoI2C : UnoBoard {
        static_assert(exists(SCL), "Uno::UnoI2C: SCL must be 0 ~ 19");
        static_assert(exists(SDA), "Uno::UnoI2C: SDA must be 0 ~ 19");
        static_assert(SCL != SDA, "Uno::UnoI2C: SCL and SDA must be different pins");
        static_assert(KHZ >= 1 && KHZ <= 400, "Uno::UnoI2C: KHZ must be 1 ~ 400 (fast mode)");
        static_assert(warn::SerialPin<(SCL <= 1 || SDA <= 1)>::ok, "");

        // flash strings of this board, for graphics::Text (F("..."))
        using FlashString = __FlashStringHelper;

        // ---------- Bus functions ----------

        // nowMs, nowUs, delayMs, delayUs, readFlash8 / 16, stopInterrupts, restoreInterrupts: from UnoBoard

        // releases both lines; frees a device stuck in the middle of a byte
        // (it holds SDA LOW: up to 9 clocks, then a STOP)
        static inline void init();

        // START + address (7 bits) + read / write bit; true if a device answered (ACK)
        static inline bool start(uint8_t address, bool read);

        // most significant bit first; true if the device answered (ACK)
        static inline bool write8(uint8_t byte);

        // ack: true = more bytes wanted, false = the last one (NACK)
        static inline uint8_t read8(bool ack);

        static inline void stop();


    private:
        // half clock periods in CPU cycles: LOW >= 1.3 µs, HIGH >= 0.6 µs (fast mode minimum);
        // whole cycles known at compile time (__builtin_avr_delay_cycles refuses anything else)
        static constexpr uint32_t cyclesOf(uint32_t nanoseconds) {
            return (uint32_t)(((uint64_t)F_CPU * nanoseconds + 999999999ULL) / 1000000000ULL);
        }
        static constexpr uint32_t LOW_NS = 550000UL / KHZ > 1300 ? 550000UL / KHZ : 1300;
        static constexpr uint32_t HIGH_NS = 450000UL / KHZ > 600 ? 450000UL / KHZ : 600;
        static constexpr uint32_t LOW_CYCLES = cyclesOf(LOW_NS);
        static constexpr uint32_t HIGH_CYCLES = cyclesOf(HIGH_NS);
        static constexpr uint8_t STRETCH_LIMIT = 200;

        static inline void waitLow();
        static inline void waitHigh();

        static inline void low(bool isScl);
        static inline void release(bool isScl);

        // releases SCL and waits until it is HIGH
        static inline void sclHigh();

        // one bit: SCL is LOW before and after
        static inline void writeBit(bool bit);
        static inline bool readBit();
    };

    // ================= definitions =================

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline void UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::init() {
        release(false);
        release(true);
        _delay_us(10);
        // a device left in the middle of a byte (e.g. the board was reset) holds SDA LOW:
        // clock it out, then STOP
        for (uint8_t i = 0; i < 9 && !pinRead<SDA>(); i++) {
            low(true);
            waitLow();
            sclHigh();
            waitHigh();
        }
        low(true);
        waitLow();
        stop();
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline bool UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::start(uint8_t address, bool read) {
        // both HIGH (bus free), then SDA falls while SCL is HIGH
        release(false);
        sclHigh();
        waitHigh();
        low(false);
        waitHigh();
        low(true);
        return write8((uint8_t)(address << 1) | (read ? 1 : 0));
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline bool UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::write8(uint8_t byte) {
        for (uint8_t i = 0; i < 8; i++) {
            writeBit(byte & 0x80);
            byte <<= 1;
        }
        // the device pulls SDA LOW for ACK
        return !readBit();
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline uint8_t UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::read8(bool ack) {
        uint8_t byte = 0;
        for (uint8_t i = 0; i < 8; i++) {
            byte = (uint8_t)(byte << 1) | (readBit() ? 1 : 0);
        }
        writeBit(!ack);
        return byte;
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline void UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::stop() {
        // SDA rises while SCL is HIGH
        low(false);
        waitLow();
        sclHigh();
        waitHigh();
        release(false);
        // bus free time before the next START
        waitLow();
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline void UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::waitLow() {
        __builtin_avr_delay_cycles(LOW_CYCLES);
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline void UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::waitHigh() {
        __builtin_avr_delay_cycles(HIGH_CYCLES);
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline void UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::low(bool isScl) {
        // PORT low first: switching to output with the pull-up on would drive HIGH for a moment
        if (isScl) {
            pinLow<SCL>();
            pinOutput<SCL>();
        } else {
            pinLow<SDA>();
            pinOutput<SDA>();
        }
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline void UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::release(bool isScl) {
        if (isScl) {
            pinInput<SCL>(INTERNAL_PULLUP);
        } else {
            pinInput<SDA>(INTERNAL_PULLUP);
        }
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline void UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::sclHigh() {
        release(true);
        for (uint8_t i = 0; i < STRETCH_LIMIT && !pinRead<SCL>(); i++) {
        }
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline void UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::writeBit(bool bit) {
        // SDA changes only while SCL is LOW
        if (bit) {
            release(false);
        } else {
            low(false);
        }
        waitLow();
        sclHigh();
        waitHigh();
        low(true);
    }

    template <uint8_t SCL, uint8_t SDA, bool INTERNAL_PULLUP, uint16_t KHZ>
    inline bool UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>::readBit() {
        release(false);
        waitLow();
        sclHigh();
        waitHigh();
        bool bit = pinRead<SDA>();
        low(true);
        return bit;
    }
}

#endif
