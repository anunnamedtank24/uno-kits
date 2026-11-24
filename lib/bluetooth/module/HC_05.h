#ifndef BLUETOOTH_HC_05_H
#define BLUETOOTH_HC_05_H

#include "../port.h"

namespace bluetooth {
    // HC-05 Bluetooth serial module (Bluetooth 2.0 + EDR, SPP), on a breakout board (often printed ZS-040)
    // 6 pins: EN (or KEY), VCC, GND, TXD, RXD, STATE
    //   VCC    3.6 ~ 6 V: the board's regulator makes the module's 3.3 V (UNO: 5V)
    //   TXD    3.3 V out -> the board's RX: read as HIGH directly
    //   RXD    3.3 V in, no pull-up <- the board's TX through a divider (1 kΩ in series, 2 kΩ to GND)
    //   EN     on most boards the module's PIO11 / pin 34 (KEY): HIGH = AT commands -> a pin (Key), through
    //          a divider too; on some boards the module's power switch instead: HIGH = on, LOW = off, open =
    //          whatever it was -> a pin (Power), kept HIGH (src/week_9_bluetooth/tests/3_diagnose.cpp tells which)
    //   STATE  the module's pin 32: LOW until paired, HIGH while connected -> a pin (State), or not wired
    //   about 30 ~ 40 mA while waiting / pairing, about 8 mA connected (from the 5V pin, not from a pin)
    //
    // the two AT modes (from the datasheet):
    //   EN HIGH while powering on   38400 baud whatever the data baud is, every command; the LED blinks slowly (1 Hz)
    //   EN HIGH after powering on   the data baud (the program's BAUD), most commands: used here, so the UNO's
    //                               UART never changes its speed; works while connected too, EN LOW goes back
    //   no KEY (Key = NoPin)        firmware "3.0-…" takes AT commands at the data baud whenever nothing is
    //                               connected (and sends them on as data while connected: with State wired,
    //                               the AT functions then return ERROR_CONNECTED instead)
    // a command ends with "\r\n"; the answer: "+NAME:xxx" lines for questions, then "OK", or "ERROR:(n)" / "FAIL"
    //
    // defaults: name HC-05, PIN 1234, 9600 baud 8N1, slave
    // the LED: fast blinking (2 Hz) waiting to be connected, two blinks a second connected, slow (1 Hz) AT mode
    //
    // Link: Uno::UnoSoftSerial<…> or Uno::UnoSerial<…>; Key, Power: Uno::UnoOutputPin<PIN> or NoPin;
    // State: Uno::UnoInputPin<PIN> or NoPin; EN is either Key or Power: HC05ByEn picks by a bool
    template <class Link, class Key, class State = NoPin, class Power = NoPin>
    struct HC05 : Port<Link, Key, State, Power> {
        using Base = Port<Link, Key, State, Power>;

        // the module starting up after power on (or after Power HIGH in init()), before it takes AT commands
        static constexpr uint16_t BOOT_MS = 1500;
        // EN HIGH until the module takes AT commands
        static constexpr uint16_t KEY_SETTLE_MS = 100;
        // the longest wait for "OK"
        static constexpr uint16_t REPLY_MS = 1000;
        // restart(): until it takes data again
        static constexpr uint16_t RESTART_MS = 1000;
        // the longest name (the datasheet's limit of the HC-06; kept for both)
        static constexpr uint8_t MAX_NAME_LENGTH = 20;

        // setRole()
        static constexpr uint8_t ROLE_SLAVE = 0;        // waits for a computer / phone / master (default)
        static constexpr uint8_t ROLE_MASTER = 1;       // connects to a slave itself (two HC-05s, UNO to UNO)
        static constexpr uint8_t ROLE_SLAVE_LOOP = 2;   // sends back whatever it receives (a test)

        // ---------- functions ----------

        // Port::init(): the UART, EN LOW (data mode), STATE as an input
        static inline void init();

        // every AT function returns OK (0) or ERROR_NO_ANSWER / ERROR_REFUSED / ERROR_BAD_VALUE (see Port);
        // with Key: each one sets EN HIGH, waits KEY_SETTLE_MS, sends, waits for the answer and sets EN LOW again
        // (unless setCommandMode(true) was called first: then EN stays HIGH, for several commands in a row);
        // without Key: sends at once, ERROR_CONNECTED while State says connected

        // "AT": the module answers
        static inline uint8_t check();

        // "AT+NAME=name": the name the computer / phone sees; 1 ~ MAX_NAME_LENGTH printable characters
        static inline uint8_t setName(const char *name);

        // "AT+PSWD=1234" (firmware 2.0), and if refused "AT+PSWD=\"1234\"" (firmware 3.0): exactly 4 digits
        static inline uint8_t setPin(const char *pin);

        // "AT+UART=baud,0,0": the data baud after restart(); then change the program's BAUD the same
        // 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400
        static inline uint8_t setBaud(uint32_t baud);

        // "AT+ROLE=n": ROLE_SLAVE, ROLE_MASTER, ROLE_SLAVE_LOOP
        static inline uint8_t setRole(uint8_t role);

        // "AT+VERSION?": the firmware into reply (e.g. "2.0-20100601"; "" if there was none)
        static inline uint8_t version(char *reply, uint8_t size);

        // "AT+RESET": starts again in data mode (EN LOW), waits RESTART_MS; the new baud and role take effect
        static inline uint8_t restart();

        // any command: "AT+ADDR?", "AT+STATE?" ...; "\r\n" is added; the first line that is not "OK" goes into
        // reply without the "+XXX:" in front (reply may be nullptr); timeoutMs: the longest wait for "OK"
        static inline uint8_t command(const char *text, char *reply = nullptr, uint8_t size = 0,
                                      uint16_t timeoutMs = REPLY_MS);

    private:
        static inline uint32_t _initMs = 0;

        // head + (value, in quotes if quoted) + "\r\n", then the answer
        static inline uint8_t run(const char *head, const char *value, bool quoted, char *reply, uint8_t size,
                                  uint16_t timeoutMs);
        static inline uint8_t readAnswer(char *reply, uint8_t size, uint16_t timeoutMs);
    };

    // an HC-05 board whose EN is the module's power switch (HIGH = on), not KEY: EN kept HIGH, no AT mode pin,
    // so the AT commands need firmware that takes them while nothing is connected ("3.0-…")
    template <class Link, class Power, class State = NoPin>
    using HC05PowerSwitch = HC05<Link, NoPin, State, Power>;

    // the HC-05 type by what EN does on the board: HC05ByEn<Link, En, State, EN_IS_POWER>::Type
    //   EN_IS_POWER false: EN is KEY (HC05<Link, En, State>); true: EN is the power switch (HC05PowerSwitch)
    template <class Link, class En, class State, bool EN_IS_POWER>
    struct HC05ByEn {
        using Type = HC05<Link, En, State>;
    };
    template <class Link, class En, class State>
    struct HC05ByEn<Link, En, State, true> {
        using Type = HC05PowerSwitch<Link, En, State>;
    };

    // ================= definitions =================

    template <class Link, class Key, class State, class Power>
    inline void HC05<Link, Key, State, Power>::init() {
        Base::init();
        _initMs = Link::nowMs();
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::check() {
        return run("AT", nullptr, false, nullptr, 0, REPLY_MS);
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::setName(const char *name) {
        if (!isName(name, MAX_NAME_LENGTH)) {
            return Base::ERROR_BAD_VALUE;
        }
        return run("AT+NAME=", name, false, nullptr, 0, REPLY_MS);
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::setPin(const char *pin) {
        if (!isDigits(pin, 4)) {
            return Base::ERROR_BAD_VALUE;
        }
        uint8_t result = run("AT+PSWD=", pin, false, nullptr, 0, REPLY_MS);
        if (result == Base::ERROR_REFUSED) {
            result = run("AT+PSWD=", pin, true, nullptr, 0, REPLY_MS);
        }
        return result;
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::setBaud(uint32_t baud) {
        switch (baud) {
            case 4800:
            case 9600:
            case 19200:
            case 38400:
            case 57600:
            case 115200:
            case 230400:
            case 460800:
            case 921600:
            case 1382400:
                break;
            default:
                return Base::ERROR_BAD_VALUE;
        }
        // "115200,0,0": the baud, 1 stop bit, no parity
        char value[16];
        uint8_t i = 0;
        char digits[8];
        uint8_t count = 0;
        do {
            digits[count] = (char)('0' + baud % 10);
            baud /= 10;
            count++;
        } while (baud > 0);
        while (count > 0) {
            count--;
            value[i] = digits[count];
            i++;
        }
        const char *tail = ",0,0";
        while (*tail) {
            value[i] = *tail;
            i++;
            tail++;
        }
        value[i] = '\0';
        return run("AT+UART=", value, false, nullptr, 0, REPLY_MS);
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::setRole(uint8_t role) {
        if (role > ROLE_SLAVE_LOOP) {
            return Base::ERROR_BAD_VALUE;
        }
        char value[2] = {(char)('0' + role), '\0'};
        return run("AT+ROLE=", value, false, nullptr, 0, REPLY_MS);
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::version(char *reply, uint8_t size) {
        return run("AT+VERSION?", nullptr, false, reply, size, REPLY_MS);
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::restart() {
        uint8_t result = run("AT+RESET", nullptr, false, nullptr, 0, REPLY_MS);
        if constexpr (Base::HAS_KEY) {
            // EN must be LOW when it starts again, or it starts in the 38400 AT mode
            Base::setCommandMode(false);
        }
        Link::delayMs(RESTART_MS);
        Link::clearInput();
        return result;
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::command(const char *text, char *reply, uint8_t size, uint16_t timeoutMs) {
        return run(text, nullptr, false, reply, size, timeoutMs);
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::run(const char *head, const char *value, bool quoted, char *reply,
                                               uint8_t size, uint16_t timeoutMs) {
        uint32_t passedMs = Link::nowMs() - _initMs;
        if (passedMs < BOOT_MS) {
            Link::delayMs((uint16_t)(BOOT_MS - passedMs));
        }
        [[maybe_unused]] bool wasCommandMode = Base::isCommandMode();
        if constexpr (Base::HAS_KEY) {
            if (!wasCommandMode) {
                Base::setCommandMode(true);
                Link::delayMs(KEY_SETTLE_MS);
            }
        } else {
            // no KEY: AT commands only while nothing is connected (else they would go out as data)
            if constexpr (Base::HAS_STATE) {
                if (Base::isConnected()) {
                    return Base::ERROR_CONNECTED;
                }
            }
            Link::flush();
        }
        Link::clearInput();
        Base::writeText(head);
        if (quoted) {
            Link::write8('"');
        }
        if (value) {
            Base::writeText(value);
        }
        if (quoted) {
            Link::write8('"');
        }
        Base::writeText("\r\n");
        Link::flush();
        uint8_t result = readAnswer(reply, size, timeoutMs);
        if constexpr (Base::HAS_KEY) {
            if (!wasCommandMode) {
                Base::setCommandMode(false);
            }
        }
        return result;
    }

    template <class Link, class Key, class State, class Power>
    inline uint8_t HC05<Link, Key, State, Power>::readAnswer(char *reply, uint8_t size, uint16_t timeoutMs) {
        if (reply && size > 0) {
            reply[0] = '\0';
        }
        bool replied = false;
        char line[40];
        uint32_t startMs = Link::nowMs();
        while (true) {
            uint32_t passedMs = Link::nowMs() - startMs;
            if (passedMs >= timeoutMs) {
                return Base::ERROR_NO_ANSWER;
            }
            if (!Base::readLine(line, sizeof(line), (uint16_t)(timeoutMs - passedMs))) {
                return Base::ERROR_NO_ANSWER;
            }
            if (startsWith(line, "OK")) {
                return Base::OK;
            }
            if (startsWith(line, "ERROR") || startsWith(line, "FAIL")) {
                return Base::ERROR_REFUSED;
            }
            if (line[0] == '\0' || replied || !reply || size == 0) {
                continue;
            }
            // "+VERSION:2.0-20100601" (firmware 3.0: "VERSION:3.0-...") -> "2.0-20100601"
            const char *from = line;
            for (const char *p = line; *p; p++) {
                if (*p == ':') {
                    from = p + 1;
                    break;
                }
            }
            uint8_t i = 0;
            while (from[i] && i + 1 < size) {
                reply[i] = from[i];
                i++;
            }
            reply[i] = '\0';
            replied = true;
        }
    }
}

#endif
