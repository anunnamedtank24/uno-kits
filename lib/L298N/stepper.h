#ifndef L298N_STEPPER_H
#define L298N_STEPPER_H

#include <stdint.h>
#include "bridge.h"

namespace L298N {
    // a bipolar stepper motor (4 wires, 2 coils) on both H-bridges: coil A on OUT1 / OUT2, coil B on OUT3 / OUT4
    //
    // BridgeA, BridgeB: structs providing the Bridge functions (see bridge.h; the board: module/L298N_board.h)
    //
    // a step = the next pattern of current in the two coils; the rotor follows the magnetic field
    //   WAVE: one coil at a time          A+  B+  A-  B-             least torque, least current
    //   FULL: both coils at a time        A+B+  A-B+  A-B-  A+B-     the most torque (the default)
    //   HALF: the two mixed, 8 patterns   A+ A+B+ B+ A-B+ A- A-B- B- A+B-   twice the steps (smoother), uneven torque
    // a 1.8° motor (NEMA 17 and most others): 200 steps a turn (WAVE / FULL), 400 (HALF)
    //
    // the L298N limits no current: a coil gets supply / coil resistance (the duty lowers the average);
    // a low-resistance motor (e.g. 1.5 Ω, 1.7 A NEMA 17) on 12 V would take far too much: use setRunDuty,
    // or a motor rated for the supply voltage (e.g. 12 V, ~ 30 Ω)
    //
    // update() must be called often (every loop, or in Uno::UnoBackground: every function turns interrupts off
    // for its few µs, so that is safe): it steps when the time comes, never waits
    template <class BridgeA, class BridgeB>
    class Stepper {
    public:
        enum Mode : uint8_t { WAVE, FULL, HALF };

        // coils off, position 0, FULL, 100 steps/s
        inline void init();

        // the step pattern (also while moving; the position keeps counting steps of the new size)
        inline void setMode(Mode mode);
        inline Mode mode() const;

        // steps a second: 1 ~ 2000 (too fast for the motor: it buzzes in place, "stalls"; start slow)
        inline void setSpeed(uint16_t stepsPerSecond);
        inline uint16_t speed() const;

        // relative: steps > 0 forward, < 0 backward; the target is position() + steps
        inline void move(int32_t steps);

        // absolute: to this position
        inline void moveTo(int32_t position);

        // turns without end: 1 = forward, -1 = backward, 0 = stop
        inline void run(int8_t direction);

        // stops where it is (the coils keep the hold duty)
        inline void stop();

        // call as often as possible; true if it stepped just now
        inline bool update();

        // true while there are steps to go (or run() is on), and one interval after the last step
        // (the rotor needs that long to get there before the coils change to the hold duty)
        inline bool isMoving() const;

        // steps from 0 (forward +); setPosition: calls the current place this number
        inline int32_t position() const;
        inline void setPosition(int32_t position);
        inline int32_t target() const;

        // the duty of the coils while stepping (default 255) and while standing (default 0 = off, no holding
        // force, no heat); a standing motor with the coils on holds its place but gets hot
        inline void setRunDuty(uint8_t duty);
        inline void setHoldDuty(uint8_t duty);

        // both coils off at once (the shaft turns freely); the next step turns them on again
        inline void release();

    private:
        // the 8 half-step patterns: coil A, coil B (+1, -1, 0 = off)
        static constexpr int8_t PATTERN_A[8] = {1, 1, 0, -1, -1, -1, 0, 1};
        static constexpr int8_t PATTERN_B[8] = {0, 1, 1, 1, 0, -1, -1, -1};

        Mode _mode = FULL;
        uint8_t _phase = 1;               // 0 ~ 7: even = one coil, odd = both
        int32_t _position = 0;
        int32_t _target = 0;
        int8_t _run = 0;
        uint16_t _speed = 100;
        uint32_t _intervalUs = 10000;
        uint32_t _lastStepUs = 0;
        bool _moving = false;
        bool _released = true;
        uint8_t _runDuty = 255;
        uint8_t _holdDuty = 0;

        inline void step(int8_t direction);
        inline void energize(uint8_t duty);
        inline void startMoving();
        inline void arrive();
        inline void alignPhase();
    };

    // ================= definitions =================

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::init() {
        InterruptGuard<BridgeA> guard;
        BridgeA::init();
        BridgeB::init();
        _mode = FULL;
        _phase = 1;
        _position = 0;
        _target = 0;
        _run = 0;
        _moving = false;
        _released = true;
        setSpeed(100);
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::setMode(Mode mode) {
        InterruptGuard<BridgeA> guard;
        _mode = mode > HALF ? FULL : mode;
        alignPhase();
        if (!_released) {
            energize(_moving ? _runDuty : _holdDuty);
        }
    }

    template <class BridgeA, class BridgeB>
    inline typename Stepper<BridgeA, BridgeB>::Mode Stepper<BridgeA, BridgeB>::mode() const {
        return _mode;
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::setSpeed(uint16_t stepsPerSecond) {
        InterruptGuard<BridgeA> guard;
        if (stepsPerSecond < 1) {
            stepsPerSecond = 1;
        } else if (stepsPerSecond > 2000) {
            stepsPerSecond = 2000;
        }
        _speed = stepsPerSecond;
        _intervalUs = 1000000UL / stepsPerSecond;
    }

    template <class BridgeA, class BridgeB>
    inline uint16_t Stepper<BridgeA, BridgeB>::speed() const {
        return _speed;
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::move(int32_t steps) {
        moveTo(_position + steps);
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::moveTo(int32_t position) {
        InterruptGuard<BridgeA> guard;
        _run = 0;
        _target = position;
        if (_target != _position) {
            startMoving();
        } else if (_moving) {
            arrive();
        }
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::run(int8_t direction) {
        InterruptGuard<BridgeA> guard;
        _run = direction > 0 ? 1 : direction < 0 ? -1 : 0;
        if (_run != 0) {
            startMoving();
        } else {
            stop();
        }
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::stop() {
        InterruptGuard<BridgeA> guard;
        _run = 0;
        _target = _position;
        if (_moving) {
            arrive();
        }
    }

    template <class BridgeA, class BridgeB>
    inline bool Stepper<BridgeA, BridgeB>::update() {
        InterruptGuard<BridgeA> guard;
        if (!_moving) {
            return false;
        }
        uint32_t now = BridgeA::nowUs();
        if (now - _lastStepUs < _intervalUs) {
            return false;
        }
        if (_run == 0 && _position == _target) {
            // the last step has had its interval to settle: now the hold duty
            arrive();
            return false;
        }
        // on time: the next step exactly one interval later (no drift); far behind: from now
        _lastStepUs = now - _lastStepUs < 2 * _intervalUs ? _lastStepUs + _intervalUs : now;
        int8_t direction = _run != 0 ? _run : (_target > _position ? 1 : -1);
        step(direction);
        return true;
    }

    template <class BridgeA, class BridgeB>
    inline bool Stepper<BridgeA, BridgeB>::isMoving() const {
        return _moving;
    }

    template <class BridgeA, class BridgeB>
    inline int32_t Stepper<BridgeA, BridgeB>::position() const {
        InterruptGuard<BridgeA> guard;
        return _position;
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::setPosition(int32_t position) {
        InterruptGuard<BridgeA> guard;
        _position = position;
        _target = position;
        if (_moving && _run == 0) {
            arrive();
        }
    }

    template <class BridgeA, class BridgeB>
    inline int32_t Stepper<BridgeA, BridgeB>::target() const {
        InterruptGuard<BridgeA> guard;
        return _run != 0 ? _position : _target;
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::setRunDuty(uint8_t duty) {
        InterruptGuard<BridgeA> guard;
        _runDuty = duty;
        if (_moving && !_released) {
            energize(_runDuty);
        }
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::setHoldDuty(uint8_t duty) {
        InterruptGuard<BridgeA> guard;
        _holdDuty = duty;
        if (!_moving && !_released) {
            energize(_holdDuty);
        }
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::release() {
        InterruptGuard<BridgeA> guard;
        BridgeA::coast();
        BridgeB::coast();
        _released = true;
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::step(int8_t direction) {
        uint8_t size = _mode == HALF ? 1 : 2;
        _phase = (uint8_t)((_phase + (direction > 0 ? size : 8 - size)) & 7);
        _position += direction;
        energize(_runDuty);
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::energize(uint8_t duty) {
        if (duty == 0) {
            release();
            return;
        }
        BridgeA::drive((int16_t)(PATTERN_A[_phase] * (int16_t)duty));
        BridgeB::drive((int16_t)(PATTERN_B[_phase] * (int16_t)duty));
        _released = false;
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::startMoving() {
        if (_moving) {
            return;
        }
        _moving = true;
        // the coils first on the current pattern, the first step one interval later
        energize(_runDuty);
        _lastStepUs = BridgeA::nowUs();
    }

    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::arrive() {
        _moving = false;
        energize(_holdDuty);
    }

    // FULL uses the odd patterns (both coils), WAVE the even ones (one coil); HALF all
    template <class BridgeA, class BridgeB>
    inline void Stepper<BridgeA, BridgeB>::alignPhase() {
        if (_mode == FULL) {
            _phase |= 1;
        } else if (_mode == WAVE) {
            _phase &= 6;
        }
    }
}

#endif
