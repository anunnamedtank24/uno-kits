#ifndef UNO_ST7789_H
#define UNO_ST7789_H

#include "Uno.h"

namespace Uno {
    // Bus for ST7789 screens on UNO (see lib/ST7789/basic/datasheet.h)
    // SCL -> D13, SDA -> D11 (hardware SPI, fixed)
    //
    // DC   data / command pin (required)
    // RES  reset pin,     NONE = software reset only
    // CS   chip select,   NONE = module without CS (needs SPI mode 3)
    // BLK  backlight pin, NONE = tied to VCC; PWM pin (3, 5, 6, 9, 10, 11) for dimming
    // MODE SPI mode 0 ~ 3
    //
    // checks at compile time: illegal pins stop the build, suspicious pins warn
    //
    // writes do not wait for the byte to be sent: the next write (or a DC / CS change) waits first,
    // so the CPU prepares the next byte while SPI is still sending the current one
    template <uint8_t DC, uint8_t RES = NONE, uint8_t CS = NONE, uint8_t BLK = NONE, uint8_t MODE = 3>
    struct UnoST7789 : UnoBoard {
        // ---------- illegal: error ----------
        static_assert(exists(DC), "Uno::UnoST7789: DC is required and must be 0 ~ 19");
        static_assert(optional(RES), "Uno::UnoST7789: RES must be 0 ~ 19 or NONE");
        static_assert(optional(CS), "Uno::UnoST7789: CS must be 0 ~ 19 or NONE");
        static_assert(optional(BLK), "Uno::UnoST7789: BLK must be 0 ~ 19 or NONE");
        static_assert(!has(DC, function::SPI), "Uno::UnoST7789: DC cannot be D11 / D12 / D13 (hardware SPI)");
        static_assert(!has(RES, function::SPI), "Uno::UnoST7789: RES cannot be D11 / D12 / D13 (hardware SPI)");
        static_assert(!has(CS, function::SPI), "Uno::UnoST7789: CS cannot be D11 / D12 / D13 (hardware SPI)");
        static_assert(!has(BLK, function::SPI), "Uno::UnoST7789: BLK cannot be D11 / D12 / D13 (hardware SPI)");
        static_assert(MODE <= 3, "Uno::UnoST7789: SPI MODE must be 0 ~ 3");
        static_assert(CS != NONE || MODE == 3, "Uno::UnoST7789: without CS the screen needs SPI mode 3");

        // ---------- suspicious: warning ----------
        static_assert(warn::DuplicatePin<
            same(DC, RES) || same(DC, CS) || same(DC, BLK) ||
            same(RES, CS) || same(RES, BLK) || same(CS, BLK)>::ok, "");
        static_assert(warn::SerialPin<
            has(DC, function::UART) || has(RES, function::UART) ||
            has(CS, function::UART) || has(BLK, function::UART)>::ok, "");
        static_assert(warn::NotPwmPin<BLK != NONE && !has(BLK, function::PWM)>::ok, "");
        static_assert(warn::Timer0Pin<has(BLK, function::PWM_TIMER0)>::ok, "");

        // ---------- Bus constants ----------

        static constexpr bool HAS_RES = RES != NONE;

        // type of Arduino F("...") strings, used by graphics::Text
        using FlashString = __FlashStringHelper;

        // ---------- Bus functions ----------

        static inline void init(uint8_t blk = 255) {
            pinOutput<DC>();
            pinHigh<DC>();
            pinOutput<RES>();
            pinHigh<RES>();
            pinOutput<CS>();
            pinHigh<CS>();
            spiSet(MODE);
            // SPIF is 0 after power on and waitSent() would never return:
            // send one dummy byte (CS high, or followed by the reset in the screen's init)
            SPDR = 0x00;
            setBacklight(blk);
        }

        static inline void setBacklight(uint8_t blk) {
            pwmSet<BLK>(blk);
        }

        // nowMs, nowUs, delayMs, delayUs, readFlash8 / 16, stopInterrupts, restoreInterrupts: from UnoBoard

        // DC must not change while a byte is being sent
        static inline void cmdMode() {
            waitSent();
            pinLow<DC>();
        }

        static inline void dataMode() {
            waitSent();
            pinHigh<DC>();
        }

        // nothing if CS is NONE
        static inline void select() {
            pinLow<CS>();
        }

        // nothing if CS is NONE (only waits for the last byte)
        static inline void deselect() {
            waitSent();
            pinHigh<CS>();
        }

        // nothing if RES is NONE
        static inline void rstHigh() {
            pinHigh<RES>();
        }

        // nothing if RES is NONE
        static inline void rstLow() {
            pinLow<RES>();
        }

        // returns as soon as the byte starts sending
        // the wait is written out here (not waitSent()): the compiler may not inline a call, every cycle counts
        static inline void write8(uint8_t byte) {
            while (!(SPSR & (1 << SPIF)));
            SPDR = byte;
        }

        // high byte first, returns as soon as the low byte starts sending
        static inline void write16(uint16_t bytes) {
            while (!(SPSR & (1 << SPIF)));
            SPDR = (uint8_t)(bytes >> 8);
            uint8_t lowByte = (uint8_t)(bytes & 0xFF);
            while (!(SPSR & (1 << SPIF)));
            SPDR = lowByte;
        }

        // wait until the last byte is out
        // SPIF stays 1 until the next SPDR write, so waiting twice is fine
        static inline void waitSent() {
            while (!(SPSR & (1 << SPIF)));
        }

        // ---------- read: not supported (write-only, no read line) ----------

        static inline void readMode() {
        }

        static inline void writeMode() {
        }

        static inline void readDummy() {
        }

        static inline uint8_t read8() {
            return 0;
        }
    };
}

#endif
