#ifndef BUZZER_ACTIVE_BUZZER_H
#define BUZZER_ACTIVE_BUZZER_H

#include <stdint.h>

namespace buzzer {
    // active buzzer: it has its own oscillator and sounds at its own pitch (most about 2.3 kHz)
    // whenever it gets DC. the board can only turn it on and off: start(hz) with any hz > 0 is on,
    // so a melody keeps its rhythm but not its notes
    //
    // Pin: a struct providing these static functions (e.g. Uno::UnoOutputPin of lib/Uno/Uno_pin.h)
    //   void init()
    //   void write(bool high)
    //   uint32_t nowMs()
    //   uint16_t readFlash16(const uint16_t *address)
    //
    // ACTIVE_HIGH: true if + is on the pin and - on GND (on = HIGH);
    //              false if + is on 5V and - on the pin (on = LOW)
    //
    // never PWM (analogWrite) on its pin: the oscillator restarts every period, it rattles
    template <class Pin, bool ACTIVE_HIGH = true>
    struct ActiveBuzzer {
        // its own pitch is unknown: a typical one, for audio::Player::beep(HZ, ms) and currentHz()
        static constexpr uint16_t HZ = 2300;

        // ---------- Sound functions (audio::Player) ----------

        // pin as output, silent
        static inline void init();

        // on (hz only has to be > 0)
        static inline void start(uint16_t hz);

        // off
        static inline void stop();

        static inline uint32_t nowMs();
        static inline uint16_t readFlash16(const uint16_t *address);
    };

    // ================= definitions =================

    template <class Pin, bool ACTIVE_HIGH>
    inline void ActiveBuzzer<Pin, ACTIVE_HIGH>::init() {
        Pin::init();
        Pin::write(!ACTIVE_HIGH);
    }

    template <class Pin, bool ACTIVE_HIGH>
    inline void ActiveBuzzer<Pin, ACTIVE_HIGH>::start(uint16_t hz) {
        (void)hz;
        Pin::write(ACTIVE_HIGH);
    }

    template <class Pin, bool ACTIVE_HIGH>
    inline void ActiveBuzzer<Pin, ACTIVE_HIGH>::stop() {
        Pin::write(!ACTIVE_HIGH);
    }

    template <class Pin, bool ACTIVE_HIGH>
    inline uint32_t ActiveBuzzer<Pin, ACTIVE_HIGH>::nowMs() {
        return Pin::nowMs();
    }

    template <class Pin, bool ACTIVE_HIGH>
    inline uint16_t ActiveBuzzer<Pin, ACTIVE_HIGH>::readFlash16(const uint16_t *address) {
        return Pin::readFlash16(address);
    }
}

#endif
