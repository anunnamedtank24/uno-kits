#ifndef NEC_TRANSMITTER_H
#define NEC_TRANSMITTER_H

#include <stdint.h>
#include "protocol.h"
#include "receiver.h"
#include "key.h"

namespace NEC {
    // sends NEC codes with an infrared LED: acts as a remote (a TV, an air conditioner, another UNO's receiver)
    //
    // Carrier: a struct providing these static functions (see lib/Uno/Uno_ir_carrier.h, UnoIrCarrier)
    //   void init()  /  void on()   the LED flashes at 38 kHz  /  void off()
    //   void delayUs(uint16_t us)  /  uint32_t nowMs()
    //
    // the sending waits: a press is 67.5 ms, a repeat 12 ms (every 108 ms while "held");
    // interrupts stay on (millis, the serial go on); their few µs are well inside the ± 30 % receivers allow
    template <class Carrier>
    class Transmitter {
    public:
        // LED off
        static inline void init();

        // one press: address 0 ~ 255 (NEC: address, ~address) or 16 bits (extended NEC, address > 255)
        static inline void send(uint16_t address, uint8_t command);
        static inline void send(const Code &code);

        // the key of a remote (Remote17KeyNumbersTop ...): false if that remote has no such key
        template <class Remote>
        static inline bool sendKey(Key key);

        // one repeat code: "still held" (send it 108 ms after the press / the last repeat)
        static inline void sendRepeat();

        // a press, then repeats every 108 ms until ms have passed (a key held down; ms < 108: just the press)
        static inline void hold(uint16_t address, uint8_t command, uint16_t ms);

        template <class Remote>
        static inline bool holdKey(Key key, uint16_t ms);

    private:
        static inline void mark(uint16_t us);
        static inline void space(uint16_t us);
        static inline void sendByte(uint8_t byte);
        static inline void waitUntil(uint32_t startMs, uint16_t ms);
    };

    // ================= definitions =================

    template <class Carrier>
    inline void Transmitter<Carrier>::init() {
        Carrier::init();
        Carrier::off();
    }

    template <class Carrier>
    inline void Transmitter<Carrier>::send(uint16_t address, uint8_t command) {
        mark(timing::LEADER_MARK_US);
        space(timing::LEADER_SPACE_US);
        if (address > 0xFF) {
            sendByte((uint8_t)(address & 0xFF));
            sendByte((uint8_t)(address >> 8));
        } else {
            sendByte((uint8_t)address);
            sendByte((uint8_t)~address);
        }
        sendByte(command);
        sendByte((uint8_t)~command);
        mark(timing::BIT_MARK_US);
        Carrier::off();
    }

    template <class Carrier>
    inline void Transmitter<Carrier>::send(const Code &code) {
        send(code.address, code.command);
    }

    template <class Carrier>
    template <class Remote>
    inline bool Transmitter<Carrier>::sendKey(Key key) {
        uint8_t command;
        if (!Remote::toCommand(key, command)) {
            return false;
        }
        send(Remote::ADDRESS, command);
        return true;
    }

    template <class Carrier>
    inline void Transmitter<Carrier>::sendRepeat() {
        mark(timing::LEADER_MARK_US);
        space(timing::REPEAT_SPACE_US);
        mark(timing::BIT_MARK_US);
        Carrier::off();
    }

    template <class Carrier>
    inline void Transmitter<Carrier>::hold(uint16_t address, uint8_t command, uint16_t ms) {
        uint32_t start = Carrier::nowMs();
        send(address, command);
        uint16_t next = timing::PERIOD_MS;
        while ((uint32_t)next + timing::PERIOD_MS / 2 <= ms) {
            waitUntil(start, next);
            sendRepeat();
            next = (uint16_t)(next + timing::PERIOD_MS);
        }
    }

    template <class Carrier>
    template <class Remote>
    inline bool Transmitter<Carrier>::holdKey(Key key, uint16_t ms) {
        uint8_t command;
        if (!Remote::toCommand(key, command)) {
            return false;
        }
        hold(Remote::ADDRESS, command, ms);
        return true;
    }

    template <class Carrier>
    inline void Transmitter<Carrier>::mark(uint16_t us) {
        Carrier::on();
        Carrier::delayUs(us);
    }

    template <class Carrier>
    inline void Transmitter<Carrier>::space(uint16_t us) {
        Carrier::off();
        Carrier::delayUs(us);
    }

    // least significant bit first
    template <class Carrier>
    inline void Transmitter<Carrier>::sendByte(uint8_t byte) {
        for (uint8_t i = 0; i < 8; i++) {
            mark(timing::BIT_MARK_US);
            space((byte & 1) ? timing::ONE_SPACE_US : timing::ZERO_SPACE_US);
            byte >>= 1;
        }
    }

    template <class Carrier>
    inline void Transmitter<Carrier>::waitUntil(uint32_t startMs, uint16_t ms) {
        while (Carrier::nowMs() - startMs < ms) {
        }
    }
}

#endif
