#ifndef UNO_SOFT_SERIAL_H
#define UNO_SOFT_SERIAL_H

#include "Uno_interrupt_pin.h"

namespace Uno {
    namespace warn {
        template <bool> struct SoftSerialFast { static constexpr bool ok = true; };
        template <> struct SoftSerialFast<true> {
            __attribute__((deprecated("Uno::UnoSoftSerial above 19200 baud: other interrupts may break bytes "
                                      "(the hardware UnoSerial on D0 / D1 has no such limit)")))
            static constexpr bool ok = true;
        };
    }

    // Link over any two pins: a UART made in software, 8N1, so D0 / D1 stay free for USB
    // (upload and pio device monitor while e.g. a Bluetooth module is connected)
    //
    // RX (into the board): a pin change interrupt on RX_PIN; every edge is timed (nowUs) and the bits are
    //   counted from the time between edges, so the interrupt is short (a few µs) and interrupts are never
    //   turned off for a whole byte (unlike Arduino SoftwareSerial): the infrared receiver, ultrasonic Echo,
    //   tones and samples keep working; received bytes go into a buffer of RX_SIZE - 1 bytes
    // TX (out of the board): each bit timed with nowUs, interrupts on: waits about 10 bits per byte
    //   (9600 baud: ~1 ms), and receiving goes on meanwhile (full duplex)
    //
    // against noise: a pulse shorter than half a bit is undone (the byte around it still arrives), a byte
    // whose start or stop bit is wrong is dropped (brokenCount()); RX has the internal pull-up, so a loose wire reads as idle (HIGH), not as data
    //
    // BAUD 1200 ~ 38400; above 19200 a compile warning: other interrupts (millis, background, the infrared
    // receiver) can shift an edge by ~5 ~ 10 µs, a third of a bit at 38400
    // RX_PIN uses its group's pin change interrupt (D0 ~ D7, D8 ~ D13, A0 ~ A5): no other UnoInterruptPin in
    // that group (e.g. the infrared receiver on D2, ultrasonic Echo on D8)
    // the interrupts are those of Uno_interrupt_pin.h: include it (or this file) in one .cpp file only
    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD = 9600, uint16_t RX_SIZE = 64>
    struct UnoSoftSerial : UnoBoard {
        static_assert(exists(RX_PIN) && exists(TX_PIN), "Uno::UnoSoftSerial: RX_PIN, TX_PIN must be 0 ~ 19");
        static_assert(RX_PIN != TX_PIN, "Uno::UnoSoftSerial: RX_PIN and TX_PIN must differ");
        static_assert(BAUD >= 1200 && BAUD <= 38400, "Uno::UnoSoftSerial: BAUD must be 1200 ~ 38400");
        static_assert(RX_SIZE >= 4 && RX_SIZE <= 256 && (RX_SIZE & (RX_SIZE - 1)) == 0,
                      "Uno::UnoSoftSerial: RX_SIZE must be a power of 2, 4 ~ 256");
        static_assert(warn::SoftSerialFast<(BAUD > 19200)>::ok, "");
        static_assert(warn::SerialPin<(RX_PIN <= 1 || TX_PIN <= 1)>::ok, "");

        // ---------- Link functions ----------

        // nowMs, nowUs, delayMs, delayUs, readFlash8 / 16, stopInterrupts, restoreInterrupts: from UnoBoard

        // TX HIGH (idle), RX input with pull-up and its interrupt; the buffer emptied
        static inline void init();

        // a received byte is waiting
        static inline bool available();

        // false if nothing arrives within timeoutMs
        static inline bool read8(uint8_t &byte, uint16_t timeoutMs);

        // sends one byte: waits until it is out (about 10 bits)
        static inline void write8(uint8_t byte);

        // drops every received byte
        static inline void clearInput();

        // write8 already waits: nothing left to send
        static inline void flush();

        // ---------- more ----------

        // received bytes dropped since init(): a wrong start or stop bit (noise longer than half a bit,
        // a wrong BAUD)
        static inline uint16_t brokenCount();

        // received bytes lost since init(): the buffer was full
        static inline uint16_t lostCount();

        // the RX interrupt (public so a test can wrap it)
        static inline void onEdge();

    private:
        // one bit in 1/16 µs (9600 baud: 1667 = 104.2 µs)
        static constexpr uint32_t BIT_X16 = (16000000UL + BAUD / 2) / BAUD;
        // 0: D8 ~ D13, 1: A0 ~ A5, 2: D0 ~ D7 (the PCINT numbers, as in UnoInterruptPin)
        static constexpr uint8_t GROUP = RX_PIN < 8 ? 2 : RX_PIN < 14 ? 0 : 1;

        // the byte being received: _position = the bit the line is in now (0 start, 1 ~ 8 data, 9 stop, 10 done)
        static inline bool _receiving = false;
        static inline bool _level = true;           // the line since _lastUs
        static inline uint32_t _lastUs = 0;
        static inline uint8_t _position = 0;
        static inline uint8_t _value = 0;
        static inline bool _startLow = true;
        static inline bool _stopHigh = true;

        // the state before the last edge: a second edge within half a bit means both were a spike
        struct Saved {
            bool receiving;
            bool level;
            uint32_t lastUs;
            uint8_t position;
            uint8_t value;
            bool startLow;
            bool stopHigh;
        };
        static inline Saved _saved = {false, true, 0, 0, 0, true, true};

        static inline uint8_t _buffer[RX_SIZE];
        static inline volatile uint8_t _head = 0;   // written by the interrupt
        static inline volatile uint8_t _tail = 0;   // written by read8
        static inline volatile uint16_t _broken = 0;
        static inline volatile uint16_t _lost = 0;

        // whole bits in dtUs, rounded (at most 12)
        static inline uint8_t bitsIn(uint32_t dtUs);
        // the line was `level` for `count` bits from _position on
        static inline void fill(bool level, uint8_t count);
        static inline void start(uint32_t nowUs);
        static inline void save();
        static inline void restore();
        static inline void finish();
        // the line has been quiet long enough to end the byte being received (no edge comes to end it)
        static inline void finishIfQuiet();
        static inline void count(volatile uint16_t &counter);
        static inline uint16_t read16(volatile uint16_t &counter);
    };

    // ================= definitions =================

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::init() {
        // HIGH before output: no LOW glitch the other side could take for a start bit
        pinHigh<TX_PIN>();
        pinOutput<TX_PIN>();
        uint8_t sreg = SREG;
        cli();
        _receiving = false;
        _head = 0;
        _tail = 0;
        _broken = 0;
        _lost = 0;
        SREG = sreg;
        UnoInterruptPin<RX_PIN, true>::init(&onEdge);
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline bool UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::available() {
        finishIfQuiet();
        return _head != _tail;
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline bool UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::read8(uint8_t &byte, uint16_t timeoutMs) {
        uint32_t startMs = nowMs();
        while (!available()) {
            if (nowMs() - startMs >= timeoutMs) {
                return false;
            }
        }
        uint8_t tail = _tail;
        byte = _buffer[tail];
        _tail = (uint8_t)((tail + 1) & (RX_SIZE - 1));
        return true;
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::write8(uint8_t byte) {
        // every edge at its own time from the start (not bit after bit), so the errors never add up
        uint32_t startUs = nowUs();
        pinLow<TX_PIN>();
        for (uint8_t bit = 1; bit <= 10; bit++) {
            uint32_t atUs = (uint32_t)bit * BIT_X16 / 16;
            while (nowUs() - startUs < atUs) {
            }
            if (bit <= 8 && (byte & (1 << (bit - 1)))) {
                pinHigh<TX_PIN>();
            } else if (bit <= 8) {
                pinLow<TX_PIN>();
            } else {
                pinHigh<TX_PIN>();   // the stop bit, then idle
            }
        }
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::clearInput() {
        uint8_t sreg = SREG;
        cli();
        _tail = _head;
        SREG = sreg;
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::flush() {
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline uint16_t UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::brokenCount() {
        return read16(_broken);
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline uint16_t UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::lostCount() {
        return read16(_lost);
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::onEdge() {
        uint32_t nowUs = Uno::nowUs();
        bool level = pinRead<RX_PIN>();
        if (!_receiving) {
            if (!level) {
                _saved.receiving = false;
                start(nowUs);
            }
            return;
        }
        if (level == _level) {
            // already back: a spike shorter than the interrupt's delay, nothing changed
            return;
        }
        uint8_t bits = bitsIn(nowUs - _lastUs);
        if (bits == 0) {
            // less than half a bit after the last edge: the two together were a spike, undo the last one
            restore();
            return;
        }
        save();
        fill(_level, bits);
        _level = level;
        _lastUs = nowUs;
        if (_position >= 10) {
            finish();
            if (!level) {
                _saved.receiving = false;
                start(nowUs);   // this falling edge is the next byte's start bit
            }
        }
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline uint8_t UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::bitsIn(uint32_t dtUs) {
        if (dtUs >= 12 * BIT_X16 / 16) {
            return 12;
        }
        return (uint8_t)((dtUs * 16 + BIT_X16 / 2) / BIT_X16);
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::fill(bool level, uint8_t count) {
        while (count > 0 && _position < 10) {
            if (_position == 0) {
                _startLow = !level;
            } else if (_position <= 8) {
                if (level) {
                    _value |= (uint8_t)(1 << (_position - 1));
                }
            } else {
                _stopHigh = level;
            }
            _position++;
            count--;
        }
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::start(uint32_t nowUs) {
        _receiving = true;
        _level = false;
        _lastUs = nowUs;
        _position = 0;
        _value = 0;
        _startLow = true;
        _stopHigh = true;
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::save() {
        _saved = {_receiving, _level, _lastUs, _position, _value, _startLow, _stopHigh};
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::restore() {
        _receiving = _saved.receiving;
        _level = _saved.level;
        _lastUs = _saved.lastUs;
        _position = _saved.position;
        _value = _saved.value;
        _startLow = _saved.startLow;
        _stopHigh = _saved.stopHigh;
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::finish() {
        _receiving = false;
        if (!_startLow || !_stopHigh) {
            count(_broken);
            return;
        }
        uint8_t next = (uint8_t)((_head + 1) & (RX_SIZE - 1));
        if (next == _tail) {
            count(_lost);
            return;
        }
        _buffer[_head] = _value;
        _head = next;
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::finishIfQuiet() {
        // the last data bits and the stop bit may all be HIGH: no edge ends the byte, so time does
        uint8_t sreg = SREG;
        cli();
        // an edge not handled yet: the interrupt will see to it
        if (_receiving && !(PCIFR & (1 << GROUP))) {
            uint8_t rest = (uint8_t)(10 - _position);
            uint32_t restUs = ((uint32_t)rest * BIT_X16 + BIT_X16 / 2) / 16;
            if (Uno::nowUs() - _lastUs >= restUs) {
                fill(_level, rest);
                finish();
            }
        }
        SREG = sreg;
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline void UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::count(volatile uint16_t &counter) {
        if (counter != 0xFFFF) {
            counter++;
        }
    }

    template <uint8_t RX_PIN, uint8_t TX_PIN, uint32_t BAUD, uint16_t RX_SIZE>
    inline uint16_t UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>::read16(volatile uint16_t &counter) {
        uint8_t sreg = SREG;
        cli();
        uint16_t value = counter;
        SREG = sreg;
        return value;
    }
}

#endif
