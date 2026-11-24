#ifndef UNO_SERIAL_H
#define UNO_SERIAL_H

#include "Uno.h"

namespace Uno {
    // the UART interrupts exist only when Uno_serial_buffer.h is included (it defines this first): programs
    // without buffers pay nothing for them
#ifdef UNO_SERIAL_BUFFER_H
    constexpr bool SERIAL_INTERRUPTS = true;
#else
    constexpr bool SERIAL_INTERRUPTS = false;
#endif

    // Link over the UART: USART0 on D0 (RX) / D1 (TX), 8N1
    // used by lib/stream (stream::Puller) to talk to lib/stream/source/stream.py on the computer,
    // and by lib/bluetooth (the Link of bluetooth::Port, HC05, HC06) on the TXD / RXD of a Bluetooth module
    //
    // BAUD: 1000000 (default) and 2000000 are exact at 16 MHz, 115200 is 2.1 % off (warning)
    // RX_SIZE, TX_SIZE: 0 (default) = polling: no interrupt, no buffer; the hardware keeps only 2 received bytes,
    //   so the program must read at least every 2 bytes' time (stream::Puller asks and then reads at once)
    //   4 ~ 256 (a power of 2) = a buffer of RX_SIZE - 1 / TX_SIZE - 1 bytes in RAM, filled / emptied by
    //   interrupts: nothing is lost while loop() is busy, and write8() returns at once while there is room
    //   (each byte costs about 3 µs in the interrupt: at 1000000 baud a third of the CPU)
    //   the interrupts are in Uno_serial_buffer.h: include it instead of this file (a compile error otherwise)
    // broken bytes (frame error: noise, a wrong baud) are dropped and counted (brokenCount()); bytes the
    // hardware or the buffer had no room for are counted too (lostCount())
    //
    // Arduino Serial must not be used at the same time (same USART; with buffers, the same interrupts too)
    // the board resets when the computer opens the port (DTR), the bootloader takes ~1 s
    template <uint32_t BAUD = 1000000, uint16_t RX_SIZE = 0, uint16_t TX_SIZE = 0>
    struct UnoSerial : UnoBoard {
        static_assert(BAUD >= 2000 && BAUD <= 2000000, "Uno::UnoSerial: BAUD must be 2000 ~ 2000000");
        static_assert(uartErrorPermille(BAUD) <= 50, "Uno::UnoSerial: BAUD is more than 5% off at 16 MHz");
        static_assert(warn::BaudError<(uartErrorPermille(BAUD) > 20)>::ok, "");
        static_assert(RX_SIZE == 0 || (RX_SIZE >= 4 && RX_SIZE <= 256 && (RX_SIZE & (RX_SIZE - 1)) == 0),
                      "Uno::UnoSerial: RX_SIZE must be 0 (polling) or a power of 2, 4 ~ 256");
        static_assert(TX_SIZE == 0 || (TX_SIZE >= 4 && TX_SIZE <= 256 && (TX_SIZE & (TX_SIZE - 1)) == 0),
                      "Uno::UnoSerial: TX_SIZE must be 0 (waits for each byte) or a power of 2, 4 ~ 256");
        static_assert((RX_SIZE == 0 && TX_SIZE == 0) || SERIAL_INTERRUPTS,
                      "Uno::UnoSerial: RX_SIZE / TX_SIZE need the UART interrupts: "
                      "#include \"Uno_serial_buffer.h\" instead of \"Uno_serial.h\"");

        // ---------- Link functions ----------

        // nowMs, nowUs, delayMs, delayUs, readFlash8 / 16, stopInterrupts, restoreInterrupts: from UnoBoard

        static inline void init();

        // a received byte is waiting
        static inline bool available();

        // false if nothing arrives within timeoutMs
        static inline bool read8(uint8_t &byte, uint16_t timeoutMs);

        // TX_SIZE = 0: waits until the byte can be sent; else waits only while the buffer is full
        static inline void write8(uint8_t byte);

        // drop every received byte
        static inline void clearInput();

        // ---------- more ----------

        // waits until every byte written has left the board
        static inline void flush();

        // received bytes dropped since init(): frame error (noise on RX, a wrong baud)
        static inline uint16_t brokenCount();

        // received bytes lost since init(): read too late (polling) or the buffer full
        static inline uint16_t lostCount();

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
        static inline uint8_t _rx[RX_SIZE > 0 ? RX_SIZE : 1];
        static inline uint8_t _tx[TX_SIZE > 0 ? TX_SIZE : 1];

        static inline void printDigits(uint32_t value, bool negative, uint8_t width, uint8_t decimals = 0);
    };

    namespace serial_state {
        // the buffers of the UnoSerial in use (one USART, so one of each); nullptr = polling
        // head: where the next byte goes in, tail: where the next one comes out; head == tail: empty
        inline uint8_t *rxData = nullptr;
        inline uint8_t rxMask = 0;
        inline volatile uint8_t rxHead = 0;
        inline volatile uint8_t rxTail = 0;
        inline uint8_t *txData = nullptr;
        inline uint8_t txMask = 0;
        inline volatile uint8_t txHead = 0;
        inline volatile uint8_t txTail = 0;
        inline volatile uint16_t broken = 0;
        inline volatile uint16_t lost = 0;

        inline void count(volatile uint16_t &counter) {
            if (counter != 0xFFFF) {
                counter++;
            }
        }

        // one received byte from the hardware: false if it was broken (dropped)
        inline bool take(uint8_t &byte) {
            uint8_t status = UCSR0A;   // before UDR0: the flags belong to the byte in UDR0
            byte = UDR0;
            if (status & (1 << DOR0)) {
                count(lost);
            }
            if (status & (1 << FE0)) {
                count(broken);
                return false;
            }
            return true;
        }

        // the USART_RX interrupt: into the buffer
        inline void receive() {
            uint8_t byte;
            if (!take(byte)) {
                return;
            }
            uint8_t next = (uint8_t)((rxHead + 1) & rxMask);
            if (next == rxTail) {
                count(lost);
                return;
            }
            rxData[rxHead] = byte;
            rxHead = next;
        }

        // the USART_UDRE interrupt: the next byte out, or stop when the buffer is empty
        inline void sendNext() {
            if (txHead == txTail) {
                UCSR0B &= (uint8_t)~(1 << UDRIE0);
                return;
            }
            UDR0 = txData[txTail];
            txTail = (uint8_t)((txTail + 1) & txMask);
        }

        inline uint16_t read16(volatile uint16_t &counter) {
            uint8_t sreg = SREG;
            cli();
            uint16_t value = counter;
            SREG = sreg;
            return value;
        }
    }

    // ================= definitions =================

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::init() {
        // already running at this rate: the speed is not set again (that would cut off a byte being sent)
        bool running = (UCSR0B & ((1 << RXEN0) | (1 << TXEN0))) == ((1 << RXEN0) | (1 << TXEN0));
        if (!(running && UBRR0 == uartUbrr(BAUD) && (UCSR0A & (1 << U2X0)))) {
            uartSet(uartUbrr(BAUD));
        }
        uint8_t sreg = SREG;
        cli();
        serial_state::rxData = nullptr;
        serial_state::txData = nullptr;
        if constexpr (RX_SIZE > 0) {
            serial_state::rxData = _rx;
        }
        if constexpr (TX_SIZE > 0) {
            serial_state::txData = _tx;
        }
        serial_state::rxMask = (uint8_t)(RX_SIZE - 1);
        serial_state::rxHead = 0;
        serial_state::rxTail = 0;
        serial_state::txMask = (uint8_t)(TX_SIZE - 1);
        serial_state::txHead = 0;
        serial_state::txTail = 0;
        serial_state::broken = 0;
        serial_state::lost = 0;
        if (RX_SIZE > 0) {
            UCSR0B |= (1 << RXCIE0);
        } else {
            UCSR0B &= (uint8_t)~(1 << RXCIE0);
        }
        UCSR0B &= (uint8_t)~(1 << UDRIE0);
        SREG = sreg;
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline bool UnoSerial<BAUD, RX_SIZE, TX_SIZE>::available() {
        if constexpr (RX_SIZE > 0) {
            return serial_state::rxHead != serial_state::rxTail;
        } else {
            return (UCSR0A & (1 << RXC0)) != 0;
        }
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline bool UnoSerial<BAUD, RX_SIZE, TX_SIZE>::read8(uint8_t &byte, uint16_t timeoutMs) {
        // polling: the hardware keeps only 2 bytes: check often, nowMs() only when nothing is there
        uint32_t startMs = nowMs();
        while (true) {
            if (available()) {
                if constexpr (RX_SIZE > 0) {
                    uint8_t tail = serial_state::rxTail;
                    byte = _rx[tail];
                    serial_state::rxTail = (uint8_t)((tail + 1) & (RX_SIZE - 1));
                    return true;
                } else {
                    if (serial_state::take(byte)) {
                        return true;
                    }
                    continue;
                }
            }
            if (nowMs() - startMs >= timeoutMs) {
                return false;
            }
        }
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::write8(uint8_t byte) {
        if constexpr (TX_SIZE == 0) {
            while (!(UCSR0A & (1 << UDRE0)));
            UDR0 = byte;
        } else {
            uint8_t next = (uint8_t)((serial_state::txHead + 1) & (TX_SIZE - 1));
            while (next == serial_state::txTail) {
                // full: the interrupt empties it; with interrupts off, send one here
                if (!(SREG & (1 << SREG_I)) && (UCSR0A & (1 << UDRE0))) {
                    serial_state::sendNext();
                }
            }
            _tx[serial_state::txHead] = byte;
            uint8_t sreg = SREG;
            cli();
            serial_state::txHead = next;
            UCSR0B |= (1 << UDRIE0);
            SREG = sreg;
        }
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::clearInput() {
        if constexpr (RX_SIZE > 0) {
            serial_state::rxTail = serial_state::rxHead;
        } else {
            while (available()) {
                (void)UDR0;
            }
        }
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::flush() {
        if (TX_SIZE > 0) {
            while (serial_state::txHead != serial_state::txTail) {
                if (!(SREG & (1 << SREG_I)) && (UCSR0A & (1 << UDRE0))) {
                    serial_state::sendNext();
                }
            }
        }
        // the last byte leaves the shift register: one byte's time at most
        uint32_t startUs = nowUs();
        while (!(UCSR0A & (1 << UDRE0)) && nowUs() - startUs < 20000000UL / BAUD) {
        }
        delayUs((uint16_t)(10000000UL / BAUD + 1));
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline uint16_t UnoSerial<BAUD, RX_SIZE, TX_SIZE>::brokenCount() {
        return serial_state::read16(serial_state::broken);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline uint16_t UnoSerial<BAUD, RX_SIZE, TX_SIZE>::lostCount() {
        return serial_state::read16(serial_state::lost);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printText(const char *text) {
        while (*text) {
            write8((uint8_t)*text);
            text++;
        }
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printDigits(uint32_t value, bool negative, uint8_t width, uint8_t decimals) {
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

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printNumber(uint32_t value, uint8_t width) {
        printDigits(value, false, width);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printNumber(int32_t value, uint8_t width) {
        // the magnitude as unsigned: -2147483648 has no positive int32_t
        printDigits(value < 0 ? 0u - (uint32_t)value : (uint32_t)value, value < 0, width);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printNumber(uint16_t value, uint8_t width) {
        printNumber((uint32_t)value, width);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printNumber(int16_t value, uint8_t width) {
        printNumber((int32_t)value, width);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printNumber(uint8_t value, uint8_t width) {
        printNumber((uint32_t)value, width);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printNumber(int8_t value, uint8_t width) {
        printNumber((int32_t)value, width);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printDecimal(int32_t value, uint8_t decimals, uint8_t width) {
        printDigits(value < 0 ? 0u - (uint32_t)value : (uint32_t)value, value < 0, width, decimals > 9 ? 9 : decimals);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printPin(uint8_t pin) {
        write8(pin >= 14 ? 'A' : 'D');
        printNumber(pin >= 14 ? (uint8_t)(pin - 14) : pin);
    }

    template <uint32_t BAUD, uint16_t RX_SIZE, uint16_t TX_SIZE>
    inline void UnoSerial<BAUD, RX_SIZE, TX_SIZE>::printHex(uint32_t value, uint8_t digits) {
        // 1 ~ 8: a shift of 32 bits or more is undefined
        digits = digits < 1 ? 1 : digits > 8 ? 8 : digits;
        printText("0x");
        for (int8_t shift = (int8_t)(4 * (digits - 1)); shift >= 0; shift -= 4) {
            uint8_t nibble = (uint8_t)((value >> shift) & 0x0F);
            write8((uint8_t)(nibble < 10 ? '0' + nibble : 'A' + nibble - 10));
        }
    }
}

#endif
