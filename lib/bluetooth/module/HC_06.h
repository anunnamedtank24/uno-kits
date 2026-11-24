#ifndef BLUETOOTH_HC_06_H
#define BLUETOOTH_HC_06_H

#include "../port.h"

namespace bluetooth {
    // HC-06 Bluetooth serial module (Bluetooth 2.0, SPP, slave only), on a breakout board
    // 4 pins: VCC, GND, TXD, RXD (6-pin boards also have STATE, and an EN / KEY that does nothing on a slave)
    //   VCC    3.6 ~ 6 V: the board's regulator makes the module's 3.3 V (UNO: 5V)
    //   TXD    3.3 V out -> the UNO's RX (D0): read as HIGH directly
    //   RXD    3.3 V in, no pull-up <- the UNO's TX (D1) through a divider (1 kΩ in series, 2 kΩ to GND)
    //   STATE  (6-pin boards) HIGH while connected -> a pin (State), or not wired
    //   about 30 mA while waiting / pairing, about 10 mA connected
    //
    // no EN / KEY: AT commands work only while nothing is connected (after connecting every byte is data),
    // at the data baud (the program's BAUD)
    // a command has no "\r\n": the module takes a pause as its end, so at most one command a second (GAP_MS);
    // the answer has no "\r\n" either: "OK", "OKsetname", "OKsetPIN", "OK9600", "linvorV1.8"
    //
    // defaults: name HC-06 (older ones: linvor), PIN 1234, 9600 baud 8N1
    // the LED: blinking waiting to be connected, on while connected
    // firmware "3.0-…" (some newer copies) wants "\r\n" and "AT+NAME=…" like an HC-05: use command() for it
    //
    // Link: Uno::UnoSerial<BAUD>; State: Uno::UnoInputPin<PIN> or NoPin
    template <class Link, class State = NoPin>
    struct HC06 : Port<Link, NoPin, State> {
        using Base = Port<Link, NoPin, State>;

        // between the starts of two commands (and from init() to the first): the module ends a command by a pause
        static constexpr uint16_t GAP_MS = 1000;
        // the longest wait for the first byte of the answer
        static constexpr uint16_t REPLY_MS = 1500;
        // the answer has ended when nothing more comes for this long
        static constexpr uint16_t QUIET_MS = 100;
        // the longest name (from the datasheet)
        static constexpr uint8_t MAX_NAME_LENGTH = 20;
        static_assert(GAP_MS >= Base::CONNECTED_MS, "bluetooth::HC06: GAP_MS must cover CONNECTED_MS");

        // ---------- functions ----------

        // Port::init(): the UART, STATE as an input
        static inline void init();

        // every AT function returns OK (0) or ERROR_NO_ANSWER / ERROR_REFUSED / ERROR_BAD_VALUE /
        // ERROR_CONNECTED (with State wired: connected, nothing sent; see Port)
        // each one waits until GAP_MS has passed since the last command, then for the answer (about 1 s each)

        // "AT": the module answers "OK"
        static inline uint8_t check();

        // "AT+NAMEname": the name the computer / phone sees; 1 ~ MAX_NAME_LENGTH printable characters
        // (a computer that paired before may show the old name until it is removed and paired again)
        static inline uint8_t setName(const char *name);

        // "AT+PIN1234": exactly 4 digits; takes effect only after the module has been without power for a while
        static inline uint8_t setPin(const char *pin);

        // "AT+BAUDn": the data baud; then change the program's BAUD the same and restart both
        // 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400
        static inline uint8_t setBaud(uint32_t baud);

        // "AT+VERSION": the firmware into reply (e.g. "linvorV1.8"; an "OK" in front is dropped)
        static inline uint8_t version(char *reply, uint8_t size);

        // any command, sent as it is (nothing added); the whole answer goes into reply (may be nullptr):
        // OK if an answer came that does not start with "ERROR"
        static inline uint8_t command(const char *text, char *reply = nullptr, uint8_t size = 0);

    private:
        static inline uint32_t _lastMs = 0;

        // head + value, the answer into answer; ERROR_CONNECTED, ERROR_NO_ANSWER, ERROR_REFUSED ("ERROR…") or OK
        static inline uint8_t run(const char *head, const char *value, char *answer, uint8_t size);

        // run(), then OK only if the answer starts with "OK"
        static inline uint8_t runOk(const char *head, const char *value);
    };

    // ================= definitions =================

    template <class Link, class State>
    inline void HC06<Link, State>::init() {
        Base::init();
        _lastMs = Link::nowMs();
    }

    template <class Link, class State>
    inline uint8_t HC06<Link, State>::check() {
        return runOk("AT", nullptr);
    }

    template <class Link, class State>
    inline uint8_t HC06<Link, State>::setName(const char *name) {
        if (!isName(name, MAX_NAME_LENGTH)) {
            return Base::ERROR_BAD_VALUE;
        }
        return runOk("AT+NAME", name);
    }

    template <class Link, class State>
    inline uint8_t HC06<Link, State>::setPin(const char *pin) {
        if (!isDigits(pin, 4)) {
            return Base::ERROR_BAD_VALUE;
        }
        return runOk("AT+PIN", pin);
    }

    template <class Link, class State>
    inline uint8_t HC06<Link, State>::setBaud(uint32_t baud) {
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
            case 230400: code = '9'; break;
            case 460800: code = 'A'; break;
            case 921600: code = 'B'; break;
            case 1382400: code = 'C'; break;
            default: return Base::ERROR_BAD_VALUE;
        }
        char value[2] = {code, '\0'};
        return runOk("AT+BAUD", value);
    }

    template <class Link, class State>
    inline uint8_t HC06<Link, State>::version(char *reply, uint8_t size) {
        char answer[24];
        uint8_t result = run("AT+VERSION", nullptr, answer, sizeof(answer));
        if (reply && size > 0) {
            // "OKlinvorV1.8" -> "linvorV1.8"
            const char *from = startsWith(answer, "OK") ? answer + 2 : answer;
            uint8_t i = 0;
            while (from[i] && i + 1 < size) {
                reply[i] = from[i];
                i++;
            }
            reply[i] = '\0';
        }
        return result;
    }

    template <class Link, class State>
    inline uint8_t HC06<Link, State>::command(const char *text, char *reply, uint8_t size) {
        char answer[24];
        if (!reply || size == 0) {
            reply = answer;
            size = sizeof(answer);
        }
        return run(text, nullptr, reply, size);
    }

    template <class Link, class State>
    inline uint8_t HC06<Link, State>::run(const char *head, const char *value, char *answer, uint8_t size) {
        answer[0] = '\0';
        // data still in a send buffer would join the command
        Link::flush();
        // the gap since the last command (or init()); STATE is watched meanwhile, so the filter has seen
        // at least CONNECTED_MS of it before deciding
        while (Link::nowMs() - _lastMs < GAP_MS) {
            if constexpr (Base::HAS_STATE) {
                Base::isConnected();
            }
        }
        if constexpr (Base::HAS_STATE) {
            if (Base::isConnected()) {
                return Base::ERROR_CONNECTED;
            }
        }
        Link::clearInput();
        _lastMs = Link::nowMs();
        Base::writeText(head);
        if (value) {
            Base::writeText(value);
        }
        if (Base::readUntilQuiet(answer, size, REPLY_MS, QUIET_MS) == 0) {
            return Base::ERROR_NO_ANSWER;
        }
        if (startsWith(answer, "ERROR")) {
            return Base::ERROR_REFUSED;
        }
        return Base::OK;
    }

    template <class Link, class State>
    inline uint8_t HC06<Link, State>::runOk(const char *head, const char *value) {
        char answer[24];
        uint8_t result = run(head, value, answer, sizeof(answer));
        if (result == Base::OK && !startsWith(answer, "OK")) {
            return Base::ERROR_REFUSED;
        }
        return result;
    }
}

#endif
