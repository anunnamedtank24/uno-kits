#ifndef L298N_DC_MOTOR_H
#define L298N_DC_MOTOR_H

#include <stdint.h>
#include "bridge.h"

namespace L298N {
    // a DC motor (2 wires) on one H-bridge: signed speed, ramp, dead zone, reversed wiring, brake / coast
    //
    // Bridge: a struct providing the Bridge functions (see bridge.h: Bridge, InputPwmBridge;
    //         the board's channels: module/L298N_board.h)
    //
    // speed -255 ~ 255 (> 0 forward), 0 = stopped (coasting)
    // ramp: the speed moves to the target at most 255 per rampMs (protects gears, the supply and the UNO
    //       from the current surge of a sudden start or reversal); update() must be called often (every loop)
    // background: every function turns interrupts off for its few µs, so update() may run in
    //       Uno::UnoBackground while loop() calls setSpeed() ...
    // dead zone: below some duty a motor only hums (it needs ~ 30 ~ 50 % to start, more under load):
    //       setMinDuty(d) maps speeds 1 ~ 255 onto duties d ~ 255 (0: the duty is the speed)
    template <class Bridge>
    class DCMotor {
    public:
        // coasting, speed 0
        inline void init();

        // the target speed -255 ~ 255; reached through the ramp in update()
        inline void setSpeed(int16_t speed);

        // the same, at once (no ramp)
        inline void setSpeedNow(int16_t speed);

        // call as often as possible: moves the speed toward the target; true while still ramping
        inline bool update();

        // stops at once, both ends shorted (duty 255 = the hardest), and stays braked; target = 0
        inline void brake(uint8_t duty = 255);

        // stops driving at once, the motor rolls out (with the EN jumper on it brakes); target = 0
        inline void coast();

        // the speed driven now, and the target
        inline int16_t speed() const;
        inline int16_t targetSpeed() const;

        // true while braked (until a speed is set)
        inline bool isBraking() const;

        // ms from 0 to 255 (default 300); 0 = no ramp
        inline void setRampMs(uint16_t ms);
        inline uint16_t rampMs() const;

        // the duty of speed 1 (default 0): the dead zone of the motor, 0 ~ 254
        inline void setMinDuty(uint8_t duty);
        inline uint8_t minDuty() const;

        // the motor's wires the other way round (forward turned out backward): swaps the direction
        inline void setReversed(bool reversed);
        inline bool isReversed() const;

        // the duty a speed gives, -255 ~ 255 (dead zone and reversed applied)
        inline int16_t dutyFor(int16_t speed) const;

    private:
        int16_t _speed = 0;
        int16_t _target = 0;
        uint16_t _rampMs = 300;
        uint8_t _minDuty = 0;
        bool _reversed = false;
        bool _braking = false;
        uint32_t _lastMs = 0;
        uint16_t _remainder = 0;        // ramp steps smaller than 1, carried over (× rampMs)

        inline void apply();
        static inline int16_t clamp(int16_t speed);
    };

    // ================= definitions =================

    template <class Bridge>
    inline void DCMotor<Bridge>::init() {
        InterruptGuard<Bridge> guard;
        Bridge::init();
        _speed = 0;
        _target = 0;
        _braking = false;
        _remainder = 0;
        _lastMs = Bridge::nowMs();
    }

    template <class Bridge>
    inline void DCMotor<Bridge>::setSpeed(int16_t speed) {
        InterruptGuard<Bridge> guard;
        _target = clamp(speed);
        if (_braking) {
            // from a brake: the ramp starts at 0
            _braking = false;
            _speed = 0;
            apply();
        }
        _lastMs = Bridge::nowMs();
        if (_rampMs == 0) {
            _speed = _target;
            apply();
        }
    }

    template <class Bridge>
    inline void DCMotor<Bridge>::setSpeedNow(int16_t speed) {
        InterruptGuard<Bridge> guard;
        _target = clamp(speed);
        _speed = _target;
        _braking = false;
        _remainder = 0;
        _lastMs = Bridge::nowMs();
        apply();
    }

    template <class Bridge>
    inline bool DCMotor<Bridge>::update() {
        InterruptGuard<Bridge> guard;
        uint32_t now = Bridge::nowMs();
        uint32_t elapsed = now - _lastMs;
        _lastMs = now;
        if (elapsed > 60000UL) {
            elapsed = 60000UL;          // keeps elapsed × 255 in 32 bits
        }
        if (_braking || _speed == _target) {
            _remainder = 0;
            return false;
        }
        if (_rampMs == 0) {
            _speed = _target;
            apply();
            return false;
        }
        // 255 per rampMs: elapsed × 255 / rampMs, the rest kept for the next time
        uint32_t scaled = elapsed * 255UL + _remainder;
        uint32_t step = scaled / _rampMs;
        _remainder = (uint16_t)(scaled % _rampMs);
        if (step == 0) {
            return true;
        }
        int16_t difference = (int16_t)(_target - _speed);
        if ((uint32_t)(difference < 0 ? -difference : difference) <= step) {
            _speed = _target;
        } else {
            _speed = (int16_t)(_speed + (difference > 0 ? (int16_t)step : -(int16_t)step));
        }
        apply();
        return _speed != _target;
    }

    template <class Bridge>
    inline void DCMotor<Bridge>::brake(uint8_t duty) {
        InterruptGuard<Bridge> guard;
        _speed = 0;
        _target = 0;
        _braking = true;
        Bridge::brake(duty);
    }

    template <class Bridge>
    inline void DCMotor<Bridge>::coast() {
        InterruptGuard<Bridge> guard;
        _speed = 0;
        _target = 0;
        _braking = false;
        Bridge::coast();
    }

    template <class Bridge>
    inline int16_t DCMotor<Bridge>::speed() const {
        return _speed;
    }

    template <class Bridge>
    inline int16_t DCMotor<Bridge>::targetSpeed() const {
        return _target;
    }

    template <class Bridge>
    inline bool DCMotor<Bridge>::isBraking() const {
        return _braking;
    }

    template <class Bridge>
    inline void DCMotor<Bridge>::setRampMs(uint16_t ms) {
        InterruptGuard<Bridge> guard;
        _rampMs = ms;
        _remainder = 0;
    }

    template <class Bridge>
    inline uint16_t DCMotor<Bridge>::rampMs() const {
        return _rampMs;
    }

    template <class Bridge>
    inline void DCMotor<Bridge>::setMinDuty(uint8_t duty) {
        InterruptGuard<Bridge> guard;
        _minDuty = duty > 254 ? 254 : duty;
        if (!_braking) {
            apply();
        }
    }

    template <class Bridge>
    inline uint8_t DCMotor<Bridge>::minDuty() const {
        return _minDuty;
    }

    template <class Bridge>
    inline void DCMotor<Bridge>::setReversed(bool reversed) {
        InterruptGuard<Bridge> guard;
        _reversed = reversed;
        if (!_braking) {
            apply();
        }
    }

    template <class Bridge>
    inline bool DCMotor<Bridge>::isReversed() const {
        return _reversed;
    }

    template <class Bridge>
    inline int16_t DCMotor<Bridge>::dutyFor(int16_t speed) const {
        speed = clamp(speed);
        if (speed == 0) {
            return 0;
        }
        uint8_t size = (uint8_t)(speed < 0 ? -speed : speed);
        // 1 ~ 255 -> just above minDuty ~ 255 (minDuty 0: the duty is the speed)
        uint8_t duty = (uint8_t)(_minDuty + ((uint16_t)size * (uint16_t)(255 - _minDuty) + 127) / 255);
        bool forward = (speed > 0) != _reversed;
        return forward ? (int16_t)duty : (int16_t)-duty;
    }

    template <class Bridge>
    inline void DCMotor<Bridge>::apply() {
        Bridge::drive(dutyFor(_speed));
    }

    template <class Bridge>
    inline int16_t DCMotor<Bridge>::clamp(int16_t speed) {
        return speed > 255 ? 255 : speed < -255 ? -255 : speed;
    }
}

#endif
