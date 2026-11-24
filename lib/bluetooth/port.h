#ifndef BLUETOOTH_PORT_H
#define BLUETOOTH_PORT_H

#include <stdint.h>

namespace bluetooth {
    // a Bluetooth serial module (SPP): a UART on one side, the radio on the other; whatever goes into its RXD
    // comes out of the computer's (or phone's) Bluetooth serial port, and the other way round
    //
    //   the board  --TXD/RXD-->  module  ~~ radio (RFCOMM / SPP) ~~  computer: a serial port (/dev/cu.HC-05, COM5)
    //
    // the module does all of Bluetooth itself (being found, pairing, the connection); the board sees only bytes,
    // so the format of the data is the program's own (see tool/packet.h for one)
    //
    // Port<Link, Key, State, Power> is the module seen from the board: the Link (the UART) plus its optional pins
    //   Key    EN / KEY: HIGH = AT command mode, LOW = data mode (the HC-05's KEY, on most boards)
    //   State  STATE: HIGH while a computer / phone is connected
    //   Power  EN as the module's power switch: HIGH = on, LOW = off (some HC-05 boards wire EN so, not to KEY;
    //          src/week_9_bluetooth/tests/3_diagnose.cpp tells which): init() turns it on and keeps it on
    // any one not wired: NoPin; what needs it is then a compile error
    //
    // a Port is a Link itself (it inherits it): stream::Puller, Packet and anything else taking a Link take it too
    //
    // Link: a struct providing these static functions (see lib/Uno/Uno_serial.h: Uno::UnoSerial<BAUD>)
    //   void init()
    //   bool available()                             a received byte is waiting
    //   bool read8(uint8_t &byte, uint16_t timeoutMs)  false if nothing arrives within timeoutMs
    //   void write8(uint8_t byte)
    //   void clearInput()                            drops every received byte
    //   void flush()                                 waits until every byte written has left (a send buffer)
    //   uint32_t nowMs()  /  void delayMs(uint16_t ms)
    // Key, Power: void init(), void write(bool high)   (Uno::UnoOutputPin<PIN>)
    // State: void init(), bool read()               (Uno::UnoInputPin<PIN>)

    // a pin of the module not wired to the board
    struct NoPin {
        static constexpr bool IS_NO_PIN = true;
        static inline void init() {
        }
        static inline void write(bool) {
        }
        static inline bool read() {
            return false;
        }
    };

    // true for NoPin
    template <class Pin, class = void> struct IsNoPin { static constexpr bool value = false; };
    template <class Pin> struct IsNoPin<Pin, decltype((void)Pin::IS_NO_PIN)> {
        static constexpr bool value = Pin::IS_NO_PIN;
    };

    template <class Link, class Key = NoPin, class State = NoPin, class Power = NoPin>
    struct Port : Link {
        static constexpr bool HAS_KEY = !IsNoPin<Key>::value;
        static constexpr bool HAS_STATE = !IsNoPin<State>::value;
        static constexpr bool HAS_POWER = !IsNoPin<Power>::value;

        // isConnected(): STATE must stay HIGH this long to count as connected, LOW this long as disconnected
        // (longer than any blinking: some boards wire STATE to the LED, which blinks while unconnected;
        // the HC-05's slowest blink is HIGH for 500 ms)
        static constexpr uint16_t CONNECTED_MS = 600;
        static constexpr uint16_t DISCONNECTED_MS = 100;

        // ---------- results of the AT commands (the modules' functions) ----------

        static constexpr uint8_t OK = 0;
        static constexpr uint8_t ERROR_NO_ANSWER = 1;   // no answer, or one that was not understood, in time
        static constexpr uint8_t ERROR_REFUSED = 2;     // the module answered with an error
        static constexpr uint8_t ERROR_BAD_VALUE = 3;   // the value is not allowed: not sent
        static constexpr uint8_t ERROR_CONNECTED = 4;   // connected: the module takes no AT commands now

        // ---------- Link functions ----------

        // available, read8, write8, clearInput, nowMs, delayMs, readFlash8 ...: from Link

        // Power HIGH (the module on), the Link, Key LOW (data mode), State as an input
        static inline void init();

        // ---------- more ----------

        // every byte of the text, without the '\0'
        static inline void writeText(const char *text);

        // decimal digits
        static inline void writeNumber(uint32_t value);

        static inline void write(const uint8_t *data, uint8_t length);

        // a computer / phone is connected: STATE filtered by time (CONNECTED_MS, DISCONNECTED_MS), so a glitch,
        // noise or a blinking STATE never counts; call it often (each call samples the pin)
        // false for CONNECTED_MS after init(); a compile error without State
        static inline bool isConnected();

        // STATE as it is now, unfiltered (a compile error without State)
        static inline bool readState();

        // Key HIGH: AT command mode; LOW: data mode (a compile error without Key)
        // flushes first: data still in a send buffer must not arrive as a command
        // the module needs a moment to notice: the modules' AT functions wait for it themselves
        static inline void setCommandMode(bool on);

        // Key is HIGH now
        static inline bool isCommandMode();

        // Power HIGH: the module on (it starts again: about a second); LOW: off (a compile error without Power)
        static inline void setPower(bool on);

        // reads one line ending with '\n' into buffer ('\r' and '\n' dropped, '\0' added; cut at size - 1,
        // the rest of the line dropped); false if no '\n' came within timeoutMs (what came is still in buffer)
        static inline bool readLine(char *buffer, uint8_t size, uint16_t timeoutMs);

        // reads whatever comes into buffer ('\0' added) until nothing more comes for quietMs; waits at most
        // firstMs for the first byte; returns how many bytes were kept (cut at size - 1, the rest dropped)
        static inline uint8_t readUntilQuiet(char *buffer, uint8_t size, uint16_t firstMs, uint16_t quietMs);

    private:
        static inline bool _commandMode = false;
        static inline bool _connected = false;
        static inline bool _stateLevel = false;
        static inline uint32_t _stateSinceMs = 0;
    };

    // true if text starts with head
    inline bool startsWith(const char *text, const char *head) {
        while (*head) {
            if (*text != *head) {
                return false;
            }
            text++;
            head++;
        }
        return true;
    }

    // 1 ~ maxLength printable ASCII characters (space ~ '~'): a name the modules take
    inline bool isName(const char *text, uint8_t maxLength) {
        uint8_t length = 0;
        while (text[length]) {
            if (text[length] < ' ' || text[length] > '~' || length >= maxLength) {
                return false;
            }
            length++;
        }
        return length > 0;
    }

    // exactly count digits '0' ~ '9': a PIN the modules take
    inline bool isDigits(const char *text, uint8_t count) {
        for (uint8_t i = 0; i < count; i++) {
            if (text[i] < '0' || text[i] > '9') {
                return false;
            }
        }
        return text[count] == '\0';
    }

    // ================= definitions =================

    template <class Link, class Key, class State, class Power>
    inline void Port<Link, Key, State, Power>::init() {
        Power::init();
        Power::write(true);
        Key::init();
        Key::write(false);
        _commandMode = false;
        State::init();
        Link::init();
        _connected = false;
        _stateLevel = State::read();
        _stateSinceMs = Link::nowMs();
    }

    template <class Link, class Key, class State, class Power>
    inline void Port<Link, Key, State, Power>::writeText(const char *text) {
        while (*text) {
            Link::write8((uint8_t)*text);
            text++;
        }
    }

    template <class Link, class Key, class State, class Power>
    inline void Port<Link, Key, State, Power>::writeNumber(uint32_t value) {
        char digits[11];
        uint8_t i = sizeof(digits) - 1;
        digits[i] = '\0';
        do {
            i--;
            digits[i] = (char)('0' + value % 10);
            value /= 10;
        } while (value > 0);
        writeText(&digits[i]);
    }

    template <class Link, class Key, class State, class Power>
    inline void Port<Link, Key, State, Power>::write(const uint8_t *data, uint8_t length) {
        for (uint8_t i = 0; i < length; i++) {
            Link::write8(data[i]);
        }
    }

    template <class Link, class Key, class State, class Power>
    inline bool Port<Link, Key, State, Power>::isConnected() {
        static_assert(HAS_STATE, "bluetooth::Port: isConnected() needs State (wire STATE to a pin)");
        bool level = State::read();
        uint32_t nowMs = Link::nowMs();
        if (level != _stateLevel) {
            _stateLevel = level;
            _stateSinceMs = nowMs;
        }
        uint16_t steadyMs = level ? CONNECTED_MS : DISCONNECTED_MS;
        if (level != _connected && nowMs - _stateSinceMs >= steadyMs) {
            _connected = level;
        }
        return _connected;
    }

    template <class Link, class Key, class State, class Power>
    inline bool Port<Link, Key, State, Power>::readState() {
        static_assert(HAS_STATE, "bluetooth::Port: readState() needs State (wire STATE to a pin)");
        return State::read();
    }

    template <class Link, class Key, class State, class Power>
    inline void Port<Link, Key, State, Power>::setCommandMode(bool on) {
        static_assert(HAS_KEY, "bluetooth::Port: setCommandMode() needs Key (wire EN / KEY to a pin)");
        Link::flush();
        Key::write(on);
        _commandMode = on;
    }

    template <class Link, class Key, class State, class Power>
    inline bool Port<Link, Key, State, Power>::isCommandMode() {
        return _commandMode;
    }

    template <class Link, class Key, class State, class Power>
    inline void Port<Link, Key, State, Power>::setPower(bool on) {
        static_assert(HAS_POWER, "bluetooth::Port: setPower() needs Power (EN as the power switch, wired to a pin)");
        Power::write(on);
    }

    template <class Link, class Key, class State, class Power>
    inline bool Port<Link, Key, State, Power>::readLine(char *buffer, uint8_t size, uint16_t timeoutMs) {
        uint8_t length = 0;
        uint32_t startMs = Link::nowMs();
        while (true) {
            uint32_t passedMs = Link::nowMs() - startMs;
            if (passedMs >= timeoutMs) {
                break;
            }
            uint8_t byte;
            if (!Link::read8(byte, (uint16_t)(timeoutMs - passedMs))) {
                break;
            }
            if (byte == '\n') {
                if (size > 0) {
                    buffer[length] = '\0';
                }
                return true;
            }
            if (byte != '\r' && length + 1 < size) {
                buffer[length] = (char)byte;
                length++;
            }
        }
        if (size > 0) {
            buffer[length] = '\0';
        }
        return false;
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t Port<Link, Key, State, Power>::readUntilQuiet(char *buffer, uint8_t size, uint16_t firstMs,
                                                         uint16_t quietMs) {
        uint8_t length = 0;
        uint8_t byte;
        bool got = Link::read8(byte, firstMs);
        while (got) {
            if (length + 1 < size) {
                buffer[length] = (char)byte;
                length++;
            }
            got = Link::read8(byte, quietMs);
        }
        if (size > 0) {
            buffer[length] = '\0';
        }
        return length;
    }
}

#endif
