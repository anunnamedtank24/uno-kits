#ifndef LED_EFFECT_H
#define LED_EFFECT_H

#include <stdint.h>

namespace LED {
    // what an Effect is doing
    enum EffectKind : uint8_t { STEADY, BLINK, BREATHE, FADE, HEARTBEAT };

    // blink, breathe, fade and heartbeat on one LED channel, without waiting: start one, call update()
    //
    // Led: one channel with set(brightness) and a static nowMs() (SingleLED, LED2Pins)
    //   LED::LED2Pins<Uno::UnoPwmPin<6>> led;
    //   LED::Effect<decltype(led)> effect(led);
    //   effect.breathe(2000);   ... loop: effect.update();
    // brightness is as the eye sees it (0 ~ 255, see brightness.h): a breath looks even
    template <class Led>
    class Effect {
    public:
        inline explicit Effect(Led &led);

        // on at brightness, no change
        inline void steady(uint8_t brightness);

        // on (brightness) for onMs, off for offMs; count times, then off (0 = for ever)
        inline void blink(uint16_t onMs, uint16_t offMs, uint8_t count = 0, uint8_t brightness = 255);

        // up and down smoothly, a whole breath in periodMs, between low and high
        inline void breathe(uint16_t periodMs, uint8_t low = 0, uint8_t high = 255);

        // from now to brightness in ms, then stays
        inline void fadeTo(uint8_t brightness, uint16_t ms);

        // two quick beats, then a pause (a heart), periodMs a cycle
        inline void heartbeat(uint16_t periodMs = 1200, uint8_t brightness = 255);

        // call as often as possible
        inline void update();

        inline EffectKind kind() const;

        // false when a blink with a count or a fade has ended
        inline bool isRunning() const;

    private:
        Led &_led;
        EffectKind _kind = STEADY;
        uint32_t _startMs = 0;
        uint16_t _a = 0;          // BLINK: on ms; BREATHE / HEARTBEAT: period; FADE: ms
        uint16_t _b = 0;          // BLINK: off ms
        uint8_t _count = 0;       // BLINK: blinks to go (0 = for ever)
        uint8_t _low = 0;
        uint8_t _high = 255;      // also the brightness of BLINK / HEARTBEAT; the target of FADE
        uint8_t _from = 0;        // FADE: the start
        uint8_t _current = 0;
        bool _running = false;

        inline void show(uint8_t brightness);
        static inline uint8_t triangle(uint32_t phase, uint32_t period, uint8_t low, uint8_t high);
    };

    // ================= definitions =================

    template <class Led>
    inline Effect<Led>::Effect(Led &led) : _led(led) {
    }

    template <class Led>
    inline void Effect<Led>::steady(uint8_t brightness) {
        _kind = STEADY;
        _running = false;
        show(brightness);
    }

    template <class Led>
    inline void Effect<Led>::blink(uint16_t onMs, uint16_t offMs, uint8_t count, uint8_t brightness) {
        _kind = BLINK;
        _a = onMs == 0 ? 1 : onMs;
        _b = offMs;
        _count = count;
        _high = brightness;
        _startMs = Led::nowMs();
        _running = true;
        show(brightness);
    }

    template <class Led>
    inline void Effect<Led>::breathe(uint16_t periodMs, uint8_t low, uint8_t high) {
        _kind = BREATHE;
        _a = periodMs < 2 ? 2 : periodMs;
        _low = low;
        _high = high;
        _startMs = Led::nowMs();
        _running = true;
        show(low);
    }

    template <class Led>
    inline void Effect<Led>::fadeTo(uint8_t brightness, uint16_t ms) {
        _kind = FADE;
        _from = _current;
        _high = brightness;
        _a = ms;
        _startMs = Led::nowMs();
        _running = true;
        if (ms == 0) {
            show(brightness);
            _running = false;
        }
    }

    template <class Led>
    inline void Effect<Led>::heartbeat(uint16_t periodMs, uint8_t brightness) {
        _kind = HEARTBEAT;
        _a = periodMs < 400 ? 400 : periodMs;
        _high = brightness;
        _startMs = Led::nowMs();
        _running = true;
    }

    template <class Led>
    inline void Effect<Led>::update() {
        if (!_running) {
            return;
        }
        uint32_t elapsed = Led::nowMs() - _startMs;
        if (_kind == BLINK) {
            uint32_t period = (uint32_t)_a + _b;
            uint32_t blinks = elapsed / period;
            if (_count != 0 && blinks >= _count) {
                show(0);
                _running = false;
                return;
            }
            show(elapsed % period < _a ? _high : 0);
        } else if (_kind == BREATHE) {
            show(triangle(elapsed % _a, _a, _low, _high));
        } else if (_kind == FADE) {
            if (elapsed >= _a) {
                show(_high);
                _running = false;
                return;
            }
            int32_t change = ((int32_t)_high - _from) * (int32_t)elapsed / _a;
            show((uint8_t)(_from + change));
        } else if (_kind == HEARTBEAT) {
            // two beats of 1/8 period each (rising and falling), 1/8 apart, then rest
            uint32_t phase = elapsed % _a;
            uint32_t beat = _a / 8;
            if (phase < beat) {
                show(triangle(phase, beat, 0, _high));
            } else if (phase >= 2 * beat && phase < 3 * beat) {
                show(triangle(phase - 2 * beat, beat, 0, (uint8_t)(_high * 3 / 4)));
            } else {
                show(0);
            }
        }
    }

    template <class Led>
    inline EffectKind Effect<Led>::kind() const {
        return _kind;
    }

    template <class Led>
    inline bool Effect<Led>::isRunning() const {
        return _running;
    }

    template <class Led>
    inline void Effect<Led>::show(uint8_t brightness) {
        if (brightness != _current) {
            _current = brightness;
            _led.set(brightness);
        }
    }

    // low -> high -> low over period
    template <class Led>
    inline uint8_t Effect<Led>::triangle(uint32_t phase, uint32_t period, uint8_t low, uint8_t high) {
        uint32_t half = period / 2;
        if (half == 0) {
            return high;
        }
        uint32_t up = phase < half ? phase : period - phase;
        // an odd period: its middle phase gives half + 1, which would go past high (and wrap around)
        if (up > half) {
            up = half;
        }
        return (uint8_t)(low + ((int32_t)high - low) * (int32_t)up / (int32_t)half);
    }
}

#endif
