#ifndef BUZZER_PASSIVE_BUZZER_H
#define BUZZER_PASSIVE_BUZZER_H

#include <stdint.h>

namespace buzzer {
    // passive buzzer: no oscillator inside, the board sends the square wave, so it plays any
    // pitch (loudest near its resonance, often 2 ~ 4 kHz; weak under ~200 Hz)
    //
    // Tone: a struct providing these static functions (e.g. Uno::UnoTone of lib/Uno/Uno_tone.h)
    //   void init()
    //   void start(uint16_t hz)     square wave, hz > 0; called again to change it without a click
    //   void stop()                 pin LOW
    //   uint16_t realHz()           the frequency really played, 0 if stopped
    //   uint32_t nowMs()
    //   uint16_t readFlash16(const uint16_t *address)
    //
    // stop() leaves the pin LOW: a magnetic buzzer must not get DC for long (its coil heats up)
    // only one at a time: a Tone usually owns a timer (Uno::UnoTone: Timer1)
    template <class Tone>
    struct PassiveBuzzer {
        // ---------- Sound functions (audio::Player) ----------

        // pin as output, silent
        static inline void init();

        // square wave at hz, or change it while playing
        static inline void start(uint16_t hz);

        // silent, pin LOW
        static inline void stop();

        static inline uint32_t nowMs();
        static inline uint16_t readFlash16(const uint16_t *address);

        // ---------- extra ----------

        // the frequency really played (whole timer ticks), 0 = silent
        static inline uint16_t realHz();
    };

    // ================= definitions =================

    template <class Tone>
    inline void PassiveBuzzer<Tone>::init() {
        Tone::init();
    }

    template <class Tone>
    inline void PassiveBuzzer<Tone>::start(uint16_t hz) {
        Tone::start(hz);
    }

    template <class Tone>
    inline void PassiveBuzzer<Tone>::stop() {
        Tone::stop();
    }

    template <class Tone>
    inline uint32_t PassiveBuzzer<Tone>::nowMs() {
        return Tone::nowMs();
    }

    template <class Tone>
    inline uint16_t PassiveBuzzer<Tone>::readFlash16(const uint16_t *address) {
        return Tone::readFlash16(address);
    }

    template <class Tone>
    inline uint16_t PassiveBuzzer<Tone>::realHz() {
        return Tone::realHz();
    }
}

#endif
