#ifndef UNO_SERIAL_H
#define UNO_SERIAL_H

#include "Uno.h"

namespace Uno {
    // Link over USB serial: USART0 on D0 (RX) / D1 (TX), 8N1, polling (no interrupt, no buffer)
    // used by video::StreamPlayer to talk to lib/video/source/stream.py on the computer
    //
    // BAUD: 1000000 (default) and 2000000 are exact at 16 MHz, 115200 is 2.1 % off (warning)
    //
    // Arduino Serial must not be used at the same time (same USART)
    // the board resets when the computer opens the port (DTR), the bootloader takes ~1 s
    template <uint32_t BAUD = 1000000>
    struct UnoSerial {
        static_assert(BAUD >= 2000 && BAUD <= 2000000, "UnoSerial: BAUD must be 2000 ~ 2000000");
        static_assert(uartErrorPermille(BAUD) <= 50, "UnoSerial: BAUD is more than 5% off at 16 MHz");
        static_assert(warn::BaudError<(uartErrorPermille(BAUD) > 20)>::ok, "");

        // ---------- Link functions ----------

        static inline void init();

        // a received byte is waiting
        static inline bool available();

        // false if nothing arrives within timeoutMs
        static inline bool read8(uint8_t &byte, uint16_t timeoutMs);

        // waits until the byte can be sent
        static inline void write8(uint8_t byte);

        // drop every received byte
        static inline void clearInput();
    };

    // ================= definitions =================

    template <uint32_t BAUD>
    inline void UnoSerial<BAUD>::init() {
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
}

#endif
