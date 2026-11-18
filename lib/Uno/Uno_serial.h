#ifndef UNO_SERIAL_H
#define UNO_SERIAL_H

#include "Uno.h"

namespace Uno {
    // Link over USB serial: USART0 on D0 (RX) / D1 (TX), 8N1, polling (no interrupt, no buffer)
    // used by lib/stream (stream::Puller) to talk to lib/stream/source/stream.py on the computer
    //
    // BAUD: 1000000 (default) and 2000000 are exact at 16 MHz, 115200 is 2.1 % off (warning)
    //
    // Arduino Serial must not be used at the same time (same USART)
    // the board resets when the computer opens the port (DTR), the bootloader takes ~1 s
    template <uint32_t BAUD = 1000000>
    struct UnoSerial : UnoBoard {
        static_assert(BAUD >= 2000 && BAUD <= 2000000, "Uno::UnoSerial: BAUD must be 2000 ~ 2000000");
        static_assert(uartErrorPermille(BAUD) <= 50, "Uno::UnoSerial: BAUD is more than 5% off at 16 MHz");
        static_assert(warn::BaudError<(uartErrorPermille(BAUD) > 20)>::ok, "");

        // ---------- Link functions ----------

        // nowMs, nowUs, delayMs, delayUs, readFlash8 / 16, stopInterrupts, restoreInterrupts: from UnoBoard

        static inline void init();

        // a received byte is waiting
        static inline bool available();

        // false if nothing arrives within timeoutMs
        static inline bool read8(uint8_t &byte, uint16_t timeoutMs);

        // waits until the byte can be sent
        static inline void write8(uint8_t byte);

        // drop every received byte
        static inline void clearInput();

        // ---------- text for pio device monitor (not part of Link) ----------

        static inline void printText(const char *text);

        // decimal; width > 0: right-aligned with spaces to width characters
        static inline void printNumber(uint32_t value, uint8_t width = 0);
        static inline void printNumber(int32_t value, uint8_t width = 0);
        static inline void printNumber(uint16_t value, uint8_t width = 0);
        static inline void printNumber(int16_t value, uint8_t width = 0);
        static inline void printNumber(uint8_t value, uint8_t width = 0);
        static inline void printNumber(int8_t value, uint8_t width = 0);

        // a fixed-point number: value / 10^decimals with `decimals` digits after the point (0 ~ 9);
        // width as printNumber, the point and the sign counted: printDecimal(2350, 2) -> 23.50, (-5, 1) -> -0.5
        static inline void printDecimal(int32_t value, uint8_t decimals, uint8_t width = 0);

        // "0x" and the lowest `digits` hex digits (1 ~ 8), e.g. printHex(0x1F, 4) -> 0x001F
        static inline void printHex(uint32_t value, uint8_t digits = 2);

        // the pin's name on the board: 0 ~ 13 -> "D0" ~ "D13", 14 ~ 19 -> "A0" ~ "A5"
        static inline void printPin(uint8_t pin);

    private:
        static inline void printDigits(uint32_t value, bool negative, uint8_t width, uint8_t decimals = 0);
    };

    // ================= definitions =================

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::init() {
        // already running at this rate: nothing to do (setting it again would cut off a byte being sent)
        bool running = (UCSR0B & ((1 << RXEN0) | (1 << TXEN0))) == ((1 << RXEN0) | (1 << TXEN0));
        if (running && UBRR0 == uartUbrr(BAUD) && (UCSR0A & (1 << U2X0))) {
            return;
        }
        uartSet(uartUbrr(BAUD));
    }

    template <uint32_t BAUD>
    inline bool UnoSerial<BAUD>::available() {
        return (UCSR0A & (1 << RXC0)) != 0;
    }

    template <uint32_t BAUD>
    inline bool UnoSerial<BAUD>::read8(uint8_t &byte, uint16_t timeoutMs) {
        // the hardware keeps only 2 bytes: check often, nowMs() only when nothing is there
        uint32_t startMs = nowMs();
        while (!available()) {
            if (nowMs() - startMs >= timeoutMs) {
                return false;
            }
        }
        byte = UDR0;
        return true;
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::write8(uint8_t byte) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = byte;
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::clearInput() {
        while (available()) {
            (void)UDR0;
        }
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printText(const char *text) {
        while (*text) {
            write8((uint8_t)*text);
            text++;
        }
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printDigits(uint32_t value, bool negative, uint8_t width, uint8_t decimals) {
        // 10 digits, '.', '-', '\0'
        char digits[13];
        uint8_t i = sizeof(digits) - 1;
        digits[i] = '\0';
        uint8_t count = 0;
        // at least one digit before the point: 5 with 1 decimal -> 0.5
        do {
            if (decimals > 0 && count == decimals) {
                i--;
                digits[i] = '.';
            }
            i--;
            digits[i] = (char)('0' + value % 10);
            value /= 10;
            count++;
        } while (value > 0 || count <= decimals);
        if (negative) {
            i--;
            digits[i] = '-';
        }
        for (uint8_t n = (uint8_t)(sizeof(digits) - 1 - i); n < width; n++) {
            write8(' ');
        }
        printText(&digits[i]);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printNumber(uint32_t value, uint8_t width) {
        printDigits(value, false, width);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printNumber(int32_t value, uint8_t width) {
        // the magnitude as unsigned: -2147483648 has no positive int32_t
        printDigits(value < 0 ? 0u - (uint32_t)value : (uint32_t)value, value < 0, width);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printNumber(uint16_t value, uint8_t width) {
        printNumber((uint32_t)value, width);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printNumber(int16_t value, uint8_t width) {
        printNumber((int32_t)value, width);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printNumber(uint8_t value, uint8_t width) {
        printNumber((uint32_t)value, width);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printNumber(int8_t value, uint8_t width) {
        printNumber((int32_t)value, width);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printDecimal(int32_t value, uint8_t decimals, uint8_t width) {
        printDigits(value < 0 ? 0u - (uint32_t)value : (uint32_t)value, value < 0, width, decimals > 9 ? 9 : decimals);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printPin(uint8_t pin) {
        write8(pin >= 14 ? 'A' : 'D');
        printNumber(pin >= 14 ? (uint8_t)(pin - 14) : pin);
    }

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::printHex(uint32_t value, uint8_t digits) {
        printText("0x");
        for (int8_t shift = (int8_t)(4 * (digits - 1)); shift >= 0; shift -= 4) {
            uint8_t nibble = (uint8_t)((value >> shift) & 0x0F);
            write8((uint8_t)(nibble < 10 ? '0' + nibble : 'A' + nibble - 10));
        }
    }
}

#endif
