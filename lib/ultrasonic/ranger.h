#ifndef ULTRASONIC_RANGER_H
#define ULTRASONIC_RANGER_H

#include <stdint.h>

namespace ultrasonic {
    // what a measurement found
    enum Result : uint8_t {
        RESULT_OK,         // distanceMm() is the distance
        RESULT_TOO_NEAR,   // closer than MIN_MM: the echo comes back while the module still sends
        RESULT_TOO_FAR,    // no echo within MAX_MM: nothing there, or a soft / slanted surface
        RESULT_NO_ECHO,    // Echo never went HIGH: no power, Trig / Echo swapped or not connected
        RESULT_BUSY,       // Echo was still HIGH from the last time: no new measurement (some clones)
        RESULT_NONE        // nothing measured yet
    };

    // a distance by sound, the trigger / echo way of the HC-SR04 and its relatives (see module/HC_SR04.h)
    //
    //   Trig: a HIGH pulse of at least 10 µs starts one measurement
    //   the module sends 8 cycles of 40 kHz sound (~0.2 ms), then sets Echo HIGH
    //   Echo goes LOW when the sound bounced back: Echo HIGH µs = the time there and back
    //   distance = Echo µs × speed of sound / 2   (343 m/s at 20 °C: 58 µs a cm)
    //   no echo: Echo goes LOW by itself after ~38 ms (some clones much later)
    //
    // Echo is timed by its interrupt (both edges), so the loop may be busy meanwhile: start it with init(),
    // then call update() every loop; it measures again every interval (setIntervalMs) by itself
    //
    // Trigger: a struct providing these static functions (see lib/Uno/Uno_pin.h, UnoOutputPin)
    //   void init()                    output, LOW
    //   void write(bool high)
    // Echo: a struct providing these static functions (see lib/Uno/Uno_interrupt_pin.h)
    //   void init(void (*onChange)())   input; onChange() runs inside an interrupt on every change
    //   bool read()                     true = HIGH
    //   uint32_t nowUs()                usable inside onChange()
    //   uint32_t nowMs()
    //   uint8_t stopInterrupts()        returns what restoreInterrupts() needs
    //   void restoreInterrupts(uint8_t saved)
    //
    // MIN_MM, MAX_MM: the range of the module (closer / farther: RESULT_TOO_NEAR / RESULT_TOO_FAR)
    //
    // everything is static (the interrupt has no object to call): one Ranger per Echo type
    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    class Ranger {
    public:
        static_assert(MIN_MM < MAX_MM, "ultrasonic::Ranger: MIN_MM must be less than MAX_MM");
        static_assert(MAX_MM <= 10000, "ultrasonic::Ranger: MAX_MM must be at most 10000 (10 m)");

        // the pulse on Trig, µs (at least 10; nowUs steps by 4 µs, so it is 12 ~ 16)
        static constexpr uint8_t TRIGGER_US = 12;

        // Echo must go HIGH this long after Trig at the latest (it takes ~0.5 ms; some clones ~2 ms)
        static constexpr uint16_t RISE_TIMEOUT_US = 10000;

        // a shorter interval lets the echo of the last measurement (from far walls) be taken for this one
        static constexpr uint16_t MIN_INTERVAL_MS = 60;

        // sets the pins and the interrupt; the first measurement starts at the next update()
        static inline void init();

        // call as often as possible: starts a measurement when the interval has passed, and finishes it;
        // true once a measurement has ended (result(), distanceMm()), never waits
        static inline bool update();

        // measures once and waits for it (at most ~40 ms; the interval is kept, so it may wait for that too)
        static inline Result measure();

        // the last measurement
        static inline Result result();

        // the distance of the last measurement, mm (only right if result() == RESULT_OK)
        static inline uint16_t distanceMm();

        // how long Echo was HIGH the last time, µs (0 if it never ended)
        static inline uint32_t echoUs();

        // time between the starts of two measurements: MIN_INTERVAL_MS (the default) ~ 65535 ms
        static inline void setIntervalMs(uint16_t ms);
        static inline uint16_t intervalMs();

        // the air temperature, 0.1 °C (default 200 = 20.0 °C): sound is faster in warm air
        // (331.3 + 0.606 × °C m/s: 10 °C off is ~1.7 % off, 1.7 cm at 1 m); -400 ~ 800 (-40 ~ 80 °C)
        static inline void setTemperature(int16_t tenthsC);

        // the speed of sound used now, 0.1 m/s (3434 at 20 °C)
        static inline uint16_t soundSpeed();

        // µs of Echo -> mm, with the current temperature
        static inline uint16_t toMm(uint32_t us);

        // measurements started since init() (stops at 65535)
        static inline uint16_t count();

        // the interrupt function init() gives the pin: public so a test can wrap it
        static inline void onChange();

    private:
        enum State : uint8_t { IDLE, WAIT_RISE, WAIT_FALL, DONE };

        // written inside the interrupt
        static inline volatile State _state = IDLE;
        static inline volatile uint32_t _riseUs = 0;
        static inline volatile uint32_t _echoUs = 0;

        static inline uint32_t _triggerUs = 0;
        static inline uint32_t _triggerMs = 0;
        static inline bool _started = false;
        static inline uint16_t _intervalMs = MIN_INTERVAL_MS;
        static inline uint16_t _speed = 3434;
        static inline uint32_t _maxEchoUs = 0;
        static inline Result _result = RESULT_NONE;
        static inline uint16_t _distanceMm = 0;
        static inline uint32_t _lastEchoUs = 0;
        static inline uint16_t _count = 0;

        static inline void start();
        static inline void finish(Result result, uint32_t us);
        static inline void setState(State state);
    };

    // ================= definitions =================

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline void Ranger<Trigger, Echo, MIN_MM, MAX_MM>::init() {
        Trigger::init();
        Trigger::write(false);
        setTemperature(200);
        _result = RESULT_NONE;
        _distanceMm = 0;
        _lastEchoUs = 0;
        _count = 0;
        _started = false;
        _state = IDLE;
        Echo::init(&onChange);
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline bool Ranger<Trigger, Echo, MIN_MM, MAX_MM>::update() {
        uint8_t saved = Echo::stopInterrupts();
        State state = _state;
        uint32_t riseUs = _riseUs;
        uint32_t echo = _echoUs;
        Echo::restoreInterrupts(saved);

        if (state == IDLE) {
            if (_started && Echo::nowMs() - _triggerMs < _intervalMs) {
                return false;
            }
            _started = true;
            _triggerMs = Echo::nowMs();
            if (_count < 65535) {
                _count++;
            }
            if (Echo::read()) {
                // still HIGH from the last one: a trigger now would be ignored
                finish(RESULT_BUSY, 0);
                return true;
            }
            start();
            return false;
        }
        uint32_t now = Echo::nowUs();
        if (state == WAIT_RISE) {
            if (now - _triggerUs > RISE_TIMEOUT_US) {
                setState(IDLE);
                finish(RESULT_NO_ECHO, 0);
                return true;
            }
            return false;
        }
        if (state == WAIT_FALL) {
            if (now - riseUs > _maxEchoUs) {
                // nothing within MAX_MM: stop waiting (the late fall is ignored)
                setState(IDLE);
                finish(RESULT_TOO_FAR, 0);
                return true;
            }
            return false;
        }
        // DONE
        setState(IDLE);
        uint16_t mm = toMm(echo);
        finish(mm < MIN_MM ? RESULT_TOO_NEAR : mm > MAX_MM ? RESULT_TOO_FAR : RESULT_OK, echo);
        return true;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline Result Ranger<Trigger, Echo, MIN_MM, MAX_MM>::measure() {
        while (!update()) {
        }
        return _result;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline Result Ranger<Trigger, Echo, MIN_MM, MAX_MM>::result() {
        return _result;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline uint16_t Ranger<Trigger, Echo, MIN_MM, MAX_MM>::distanceMm() {
        return _distanceMm;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline uint32_t Ranger<Trigger, Echo, MIN_MM, MAX_MM>::echoUs() {
        return _lastEchoUs;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline void Ranger<Trigger, Echo, MIN_MM, MAX_MM>::setIntervalMs(uint16_t ms) {
        _intervalMs = ms < MIN_INTERVAL_MS ? MIN_INTERVAL_MS : ms;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline uint16_t Ranger<Trigger, Echo, MIN_MM, MAX_MM>::intervalMs() {
        return _intervalMs;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline void Ranger<Trigger, Echo, MIN_MM, MAX_MM>::setTemperature(int16_t tenthsC) {
        if (tenthsC < -400) {
            tenthsC = -400;
        } else if (tenthsC > 800) {
            tenthsC = 800;
        }
        // 0.1 m/s: 3313 + 0.606 × tenthsC / 10 (rounded)
        int32_t delta = (int32_t)606 * tenthsC;
        _speed = (uint16_t)(3313 + (delta + (delta >= 0 ? 500 : -500)) / 1000);
        // the Echo time of MAX_MM, and 10 % more for the margin
        uint32_t us = (uint32_t)MAX_MM * 20000UL / _speed;
        _maxEchoUs = us + us / 10;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline uint16_t Ranger<Trigger, Echo, MIN_MM, MAX_MM>::soundSpeed() {
        return _speed;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline uint16_t Ranger<Trigger, Echo, MIN_MM, MAX_MM>::toMm(uint32_t us) {
        // there and back: mm = µs × (0.1 m/s) / 20000; µs < ~120 ms keeps it in 32 bits
        if (us > 120000UL) {
            return 65535;
        }
        uint32_t mm = (us * _speed + 10000UL) / 20000UL;
        return mm > 65535UL ? 65535 : (uint16_t)mm;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline uint16_t Ranger<Trigger, Echo, MIN_MM, MAX_MM>::count() {
        return _count;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline void Ranger<Trigger, Echo, MIN_MM, MAX_MM>::onChange() {
        uint32_t now = Echo::nowUs();
        if (Echo::read()) {
            if (_state == WAIT_RISE) {
                _riseUs = now;
                _state = WAIT_FALL;
            }
        } else if (_state == WAIT_FALL) {
            _echoUs = now - _riseUs;
            _state = DONE;
        }
    }

    // the pulse on Trig, then wait for Echo
    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline void Ranger<Trigger, Echo, MIN_MM, MAX_MM>::start() {
        setState(WAIT_RISE);
        Trigger::write(true);
        uint32_t begin = Echo::nowUs();
        while (Echo::nowUs() - begin < TRIGGER_US) {
        }
        Trigger::write(false);
        _triggerUs = Echo::nowUs();
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline void Ranger<Trigger, Echo, MIN_MM, MAX_MM>::finish(Result result, uint32_t us) {
        _result = result;
        _lastEchoUs = us;
        _distanceMm = result == RESULT_OK ? toMm(us) : 0;
    }

    template <class Trigger, class Echo, uint16_t MIN_MM, uint16_t MAX_MM>
    inline void Ranger<Trigger, Echo, MIN_MM, MAX_MM>::setState(State state) {
        uint8_t saved = Echo::stopInterrupts();
        _state = state;
        Echo::restoreInterrupts(saved);
    }
}

#endif
