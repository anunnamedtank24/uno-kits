#ifndef BLUETOOTH_BT05_H
#define BLUETOOTH_BT05_H

#include "../port.h"

namespace bluetooth {
    // BT05-type BLE module (Bluetooth 4.0 low energy, the HM-10 family of copies), often sold on a board that
    // looks like an HC-05's (6 pins: EN, VCC, GND, TXD, RXD, STATE); tells itself by "+UUID=FFE0" to "AT+UUID"
    //   the UART side is the same as an HC-05's: transparent serial, 3.3 V logic (divider on RXD), 9600 8N1
    //   the radio side is BLE, not SPP: no serial port appears on the computer; the computer or phone
    //   connects with GATT: service FFE0, characteristic FFE1 (write = to the board, notify = from the board),
    //   at most 20 bytes a write (lib/bluetooth/source/ble.py does it with bleak; iPhone apps work too)
    //   EN     on the board measured: the module's power switch (HIGH = on, LOW = off, open = as it was): Power
    //   STATE  HIGH while connected
    //
    // the AT commands (firmware "v6.1", measured): only while nothing is connected (afterwards every byte is
    // data), at the data baud, ending with "\r\n"
    //   "AT"              -> "OK"
    //   "AT+NAMExxx"      -> "+NAME=xxx", "OK"          (no "=": "AT+NAME=x" names it "=x")
    //   "AT+NAME"         -> "+NAME=xxx"                (asking: no "OK")
    //   "AT+PINxxxxxx"    -> "+PIN=xxxxxx", "OK"        (6 digits; 4 digits: "ERROR")
    //   "AT+BAUD"         -> "+BAUD=4"                  (the HC-06 codes: 4 = 9600)
    //   "AT+VERSION"      -> "+VERSION=v6.1"
    //   "AT+LADDR"        -> "+LADD=C8FD1995E541"       (its Bluetooth address)
    //   anything unknown  -> "ERROR"
    //   "AT+HELP" lists the rest: RESET, DEFAULT, ROLE, UUID, CHAR, POWE, PWRM, SLEEP, INQ, CONN ...
    //
    // Link: Uno::UnoSoftSerial<…> or Uno::UnoSerial<…>; State: Uno::UnoInputPin<PIN> or NoPin;
    // Power: Uno::UnoOutputPin<PIN> for EN as the power switch, or NoPin (EN not wired, the module always on)
    template <class Link, class State = NoPin, class Power = NoPin>
    struct BT05 : Port<Link, NoPin, State, Power> {
        using Base = Port<Link, NoPin, State, Power>;

        // the module starting up after power on (or after Power HIGH in init()), before it takes AT commands
        static constexpr uint16_t BOOT_MS = 1500;
        // the longest wait for the answer
        static constexpr uint16_t REPLY_MS = 1500;
        // from one answer to the next command
        static constexpr uint16_t GAP_MS = 100;
        // no answer at all: wait this long and send it once more (measured: right after "AT+NAMExxx" the module
        // ignores commands for a while, setting up its new advertisement; a command broken on the wire too)
        static constexpr uint16_t RETRY_MS = 1500;
        static constexpr uint8_t TRIES = 2;
        // restart(): until it takes commands again
        static constexpr uint16_t RESTART_MS = 1500;
        // the longest name (a BLE advertisement has room for about 20 characters next to the rest)
        static constexpr uint8_t MAX_NAME_LENGTH = 18;
        // the PIN's digits
        static constexpr uint8_t PIN_LENGTH = 6;

        // ---------- functions ----------

        // Port::init(): Power HIGH (the module on), the UART, STATE as an input
        static inline void init();

        // every AT function returns OK (0) or ERROR_NO_ANSWER / ERROR_REFUSED / ERROR_BAD_VALUE /
        // ERROR_CONNECTED (with State wired: connected, nothing sent; see Port); no answer at all: sent again
        // after RETRY_MS (TRIES times in all)

        // "AT": the module answers "OK"
        static inline uint8_t check();

        // "AT+NAMEname": the name in its advertisement; 1 ~ MAX_NAME_LENGTH printable characters
        static inline uint8_t setName(const char *name);

        // "AT+NAME": the name now into reply
        static inline uint8_t name(char *reply, uint8_t size);

        // "AT+PIN123456": exactly PIN_LENGTH digits (used only by apps that pair)
        static inline uint8_t setPin(const char *pin);

        // "AT+BAUDn": the data baud from its next start; then change the program's BAUD the same
        // 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 (the HC-06 codes; 4 = 9600 measured)
        static inline uint8_t setBaud(uint32_t baud);

        // "AT+VERSION": the firmware into reply (e.g. "v6.1")
        static inline uint8_t version(char *reply, uint8_t size);

        // "AT+LADDR": its Bluetooth address into reply (12 hex digits)
        static inline uint8_t address(char *reply, uint8_t size);

        // "AT+RESET": starts again, waits RESTART_MS; a new name or baud takes effect
        static inline uint8_t restart();

        // any command, "\r\n" added: the value of the first "+XXX=value" line into reply (may be nullptr);
        // OK at "OK", or at that line when no "OK" follows within the time
        static inline uint8_t command(const char *text, char *reply = nullptr, uint8_t size = 0);

    private:
        static inline uint32_t _initMs = 0;
        static inline uint32_t _lastMs = 0;

        // head + value + "\r\n"; waitOk: a "+…" line is followed by "OK" (setting) or not (asking)
        static inline uint8_t run(const char *head, const char *value, char *reply, uint8_t size, bool waitOk);
        // the answer: "OK", "ERROR", or a "+XXX=value" line (then "OK" if waitOk)
        static inline uint8_t readAnswer(char *reply, uint8_t size, bool waitOk);
    };

    // ================= definitions =================

    template <class Link, class State, class Power>
    inline void BT05<Link, State, Power>::init() {
        Base::init();
        _initMs = Link::nowMs();
        _lastMs = _initMs;
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::check() {
        return run("AT", nullptr, nullptr, 0, true);
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::setName(const char *name) {
        if (!isName(name, MAX_NAME_LENGTH)) {
            return Base::ERROR_BAD_VALUE;
        }
        return run("AT+NAME", name, nullptr, 0, true);
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::name(char *reply, uint8_t size) {
        return run("AT+NAME", nullptr, reply, size, false);
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::setPin(const char *pin) {
        if (!isDigits(pin, PIN_LENGTH)) {
            return Base::ERROR_BAD_VALUE;
        }
        return run("AT+PIN", pin, nullptr, 0, true);
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::setBaud(uint32_t baud) {
        char code;
        switch (baud) {
            case 1200: code = '1'; break;
            case 2400: code = '2'; break;
            case 4800: code = '3'; break;
            case 9600: code = '4'; break;
            case 19200: code = '5'; break;
            case 38400: code = '6'; break;
            case 57600: code = '7'; break;
            case 115200: code = '8'; break;
            default: return Base::ERROR_BAD_VALUE;
        }
        char value[2] = {code, '\0'};
        return run("AT+BAUD", value, nullptr, 0, true);
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::version(char *reply, uint8_t size) {
        return run("AT+VERSION", nullptr, reply, size, false);
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::address(char *reply, uint8_t size) {
        return run("AT+LADDR", nullptr, reply, size, false);
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::restart() {
        uint8_t result = run("AT+RESET", nullptr, nullptr, 0, true);
        Link::delayMs(RESTART_MS);
        Link::clearInput();
        return result;
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::command(const char *text, char *reply, uint8_t size) {
        return run(text, nullptr, reply, size, true);
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::run(const char *head, const char *value, char *reply, uint8_t size,
                                                 bool waitOk) {
        if (reply && size > 0) {
            reply[0] = '\0';
        }
        // started up, and the last command's setting saved; STATE watched meanwhile (its filter needs time)
        while (Link::nowMs() - _initMs < BOOT_MS || Link::nowMs() - _lastMs < GAP_MS) {
            if constexpr (Base::HAS_STATE) {
                Base::isConnected();
            }
        }
        if constexpr (Base::HAS_STATE) {
            if (Base::isConnected()) {
                return Base::ERROR_CONNECTED;
            }
        }
        // data still in a send buffer would join the command
        Link::flush();
        uint8_t result = Base::ERROR_NO_ANSWER;
        for (uint8_t attempt = 0; attempt < TRIES && result == Base::ERROR_NO_ANSWER; attempt++) {
            if (attempt > 0) {
                Link::delayMs(RETRY_MS);
            }
            Link::clearInput();
            Base::writeText(head);
            if (value) {
                Base::writeText(value);
            }
            Base::writeText("\r\n");
            Link::flush();
            result = readAnswer(reply, size, waitOk);
        }
        _lastMs = Link::nowMs();
        return result;
    }

    template <class Link, class State, class Power>
    inline uint8_t BT05<Link, State, Power>::readAnswer(char *reply, uint8_t size, bool waitOk) {
        bool gotValue = false;
        char line[40];
        uint32_t startMs = Link::nowMs();
        while (true) {
            uint32_t waitedMs = Link::nowMs() - startMs;
            if (waitedMs >= REPLY_MS) {
                // asking (or a command) answered with only "+XXX=value": that is the whole answer
                return gotValue ? Base::OK : Base::ERROR_NO_ANSWER;
            }
            if (!Base::readLine(line, sizeof(line), (uint16_t)(REPLY_MS - waitedMs))) {
                continue;
            }
            if (startsWith(line, "OK")) {
                return Base::OK;
            }
            if (startsWith(line, "ERROR")) {
                return Base::ERROR_REFUSED;
            }
            if (line[0] != '+' || gotValue) {
                continue;
            }
            gotValue = true;
            // "+VERSION=v6.1" -> "v6.1"
            const char *from = line;
            while (*from && *from != '=') {
                from++;
            }
            if (*from == '=') {
                from++;
            }
            if (reply && size > 0) {
                uint8_t i = 0;
                while (from[i] && i + 1 < size) {
                    reply[i] = from[i];
                    i++;
                }
                reply[i] = '\0';
            }
            if (!waitOk) {
                return Base::OK;
            }
        }
    }
}

#endif
