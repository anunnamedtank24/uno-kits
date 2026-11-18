#ifndef L298N_TWO_WHEEL_CAR_H
#define L298N_TWO_WHEEL_CAR_H

#include <stdint.h>
#include "../bridge.h"
#include "../dc_motor.h"

namespace L298N {
    // which wheel an IN pair drives
    enum Side : uint8_t { LEFT, RIGHT };

    // which way that wheel turns when the pair is positive (IN1 - IN2 > 0: IN1 HIGH, IN2 LOW; IN3 - IN4 the same)
    enum Turn : uint8_t { FORWARD, BACKWARD };

    // a car with two driven wheels (left, right) on the two channels of one L298N, steered by their speed
    // difference (differential drive); the moves are queued with their times and run one after another
    //
    // the class is one way of wiring; its parameters say what each IN pair does:
    //   Board:         the L298N board (module/L298N_board.h): Board::A = IN1, IN2 -> OUT1, OUT2;
    //                  Board::B = IN3, IN4 -> OUT3, OUT4 (an IN on GND: Ground, that wheel one way only)
    //   IN12_SIDE:     the wheel on IN1 / IN2 (LEFT or RIGHT); IN3 / IN4 drive the other one
    //   IN12_POSITIVE: how that wheel turns when IN1 - IN2 > 0 (FORWARD or BACKWARD)
    //   IN34_POSITIVE: how the other wheel turns when IN3 - IN4 > 0
    //   QUEUE:         moves that can wait (each 9 bytes of RAM)
    //   e.g. IN1 - IN2 > 0: the right wheel forward; IN3 - IN4 > 0: the left wheel forward:
    //     TwoWheelCar<Board, RIGHT, FORWARD, FORWARD>
    //   (the two motors of a car face opposite ways, so often one pair is BACKWARD: try it with the wheels up)
    //
    // a move: speeds 0 ~ 255 (signed where a wheel can go both ways) and ms, how long it lasts
    //   ms > 0:  then the next move; after the last one both wheels brake
    //   ms = 0:  no time: on and on, until a next move is given, stop(), until() comes true, or the failsafe
    //   until(condition): the move just given also ends as soon as condition() is true
    //            car.forward(180); car.until([] { return ranger.distanceMm() < 200; });
    //   setFailsafeMs(ms): a move without a time stops by itself when no new move came for that long
    //            (a car steered by a remote stops when the remote is lost)
    //
    // the moves never wait: they go into the queue and update() runs them; call update() often (every loop),
    // or let it run by itself in the background (lib/Uno/Uno_background.h):
    //   Uno::UnoBackground::add([] { car.update(); });
    // every function turns interrupts off for its few µs, so loop() and the background never collide
    template <class Board, Side IN12_SIDE, Turn IN12_POSITIVE, Turn IN34_POSITIVE, uint8_t QUEUE = 8>
    class TwoWheelCar {
        // the channel and the "positive is forward" of each wheel
        using LeftBridge = typename Pick<IN12_SIDE == LEFT, typename Board::A, typename Board::B>::Type;
        using RightBridge = typename Pick<IN12_SIDE == LEFT, typename Board::B, typename Board::A>::Type;
        static constexpr bool LEFT_POSITIVE_FORWARD =
            (IN12_SIDE == LEFT ? IN12_POSITIVE : IN34_POSITIVE) == FORWARD;
        static constexpr bool RIGHT_POSITIVE_FORWARD =
            (IN12_SIDE == LEFT ? IN34_POSITIVE : IN12_POSITIVE) == FORWARD;
        using Guard = InterruptGuard<LeftBridge>;

    public:
        static_assert(QUEUE >= 1, "L298N::TwoWheelCar: QUEUE must be at least 1");

        // a condition for until(): a function without arguments (a lambda without captures fits)
        using Condition = bool (*)();

        // false: that wheel only turns one way (an IN on GND, see Ground): the moves that need the other way
        // brake it instead
        static constexpr bool LEFT_BOTH_WAYS = LeftBridge::BOTH_WAYS;
        static constexpr bool RIGHT_BOTH_WAYS = RightBridge::BOTH_WAYS;

        // both wheels braked, nothing queued, ramp 200 ms, no failsafe
        inline void init();

        // ---------- moves (queued; false if the queue is full) ----------

        // straight on, both wheels the same
        inline bool forward(uint8_t speed, uint16_t ms = 0);
        inline bool backward(uint8_t speed, uint16_t ms = 0);

        // in place, the wheels opposite: speed > 0 clockwise (to the right, seen from above), < 0 to the left
        inline bool spin(int16_t speed, uint16_t ms = 0);

        // around one wheel, which stays braked; the other drives: speed > 0 forward, < 0 backward
        //   pivotLeft:  the left wheel stays, the car turns left (forward)
        //   pivotRight: the right wheel stays, the car turns right (forward)
        inline bool pivotLeft(int16_t speed, uint16_t ms = 0);
        inline bool pivotRight(int16_t speed, uint16_t ms = 0);

        // a curve by a speed difference: left = speed + difference / 2, right = speed - difference / 2
        //   difference > 0 curves to the right, < 0 to the left; 0 = straight; 2 × speed = pivot
        inline bool arc(int16_t speed, int16_t difference, uint16_t ms = 0);

        // each wheel its own speed, -255 ~ 255
        inline bool wheels(int16_t left, int16_t right, uint16_t ms = 0);

        // stands still for ms: braked (stops at once) or coasting (rolls out; with the EN jumpers on it brakes)
        inline bool brake(uint16_t ms = 0);
        inline bool coast(uint16_t ms = 0);

        // the move given last (waiting or running) also ends as soon as condition() is true
        // (checked at every update(); nullptr = only its time); false if there is no such move
        // condition() runs with interrupts off (maybe in the background): read values, do not wait
        // (e.g. ranger.distanceMm() < 200, sensor.isDark(), car.moveMs() > 2000; not Serial, not delay)
        inline bool until(Condition condition);

        // ---------- now ----------

        // clears the queue and brakes both wheels at once
        inline void stop();

        // call as often as possible (or in Uno::UnoBackground)
        inline void update();

        // true while a timed move runs or moves wait (a move without a time alone does not count)
        inline bool isBusy() const;

        // true while any move runs, also one without a time
        inline bool isRunning() const;

        // calls update() until isBusy() is false (blocks)
        inline void wait();

        // moves waiting in the queue (not counting the one running)
        inline uint8_t queued() const;

        // how long the running move has been going, ms (0 if none)
        inline uint32_t moveMs() const;

        // the speeds the wheels have now (after ramp and trim), -255 ~ 255, + = forward
        inline int16_t leftSpeed() const;
        inline int16_t rightSpeed() const;

        // ---------- settings ----------

        // ms from 0 to 255 for both wheels (default 200); 0 = at once; the ramp is part of a move's time
        inline void setRampMs(uint16_t ms);

        // the dead zone of the motors (see DCMotor::setMinDuty)
        inline void setMinDuty(uint8_t duty);

        // one motor faster than the other (the car drifts on forward): slow the faster one down,
        // percent of the speed each wheel gets, 1 ~ 100 (default 100, 100)
        inline void setTrim(uint8_t leftPercent, uint8_t rightPercent);

        // a move without a time brakes when no new move came for ms (default 0 = never)
        inline void setFailsafeMs(uint16_t ms);

    private:
        enum Kind : uint8_t { DRIVE, BRAKE, COAST };

        struct Move {
            Kind kind;
            int16_t left;
            int16_t right;
            uint16_t ms;
            Condition condition;
        };

        DCMotor<LeftBridge> _left;
        DCMotor<RightBridge> _right;
        Move _queue[QUEUE];
        uint8_t _head = 0;
        uint8_t _count = 0;
        Move _current = {BRAKE, 0, 0, 0, nullptr};
        bool _running = false;
        uint32_t _startMs = 0;
        uint32_t _lastMoveMs = 0;        // when the last move was given (failsafe)
        uint16_t _failsafeMs = 0;
        uint8_t _leftTrim = 100;
        uint8_t _rightTrim = 100;

        inline bool push(Kind kind, int16_t left, int16_t right, uint16_t ms);
        inline void start(const Move &move);
        inline void standStill();
        static inline int16_t clamp(int32_t speed);
    };

    // ================= definitions =================

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::init() {
        Guard guard;
        _left.init();
        _right.init();
        // DCMotor's forward is IN1 HIGH, IN2 LOW (positive)
        _left.setReversed(!LEFT_POSITIVE_FORWARD);
        _right.setReversed(!RIGHT_POSITIVE_FORWARD);
        setRampMs(200);
        _count = 0;
        _running = false;
        _failsafeMs = 0;
        standStill();
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::forward(uint8_t speed, uint16_t ms) {
        return push(DRIVE, speed, speed, ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::backward(uint8_t speed, uint16_t ms) {
        return push(DRIVE, (int16_t)-speed, (int16_t)-speed, ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::spin(int16_t speed, uint16_t ms) {
        return push(DRIVE, clamp(speed), clamp(-(int32_t)speed), ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::pivotLeft(int16_t speed, uint16_t ms) {
        // left braked, right drives
        return push(DRIVE, 0, clamp(speed), ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::pivotRight(int16_t speed, uint16_t ms) {
        return push(DRIVE, clamp(speed), 0, ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::arc(int16_t speed, int16_t difference, uint16_t ms) {
        return push(DRIVE, clamp((int32_t)speed + difference / 2), clamp((int32_t)speed - difference / 2), ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::wheels(int16_t left, int16_t right, uint16_t ms) {
        return push(DRIVE, clamp(left), clamp(right), ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::brake(uint16_t ms) {
        return push(BRAKE, 0, 0, ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::coast(uint16_t ms) {
        return push(COAST, 0, 0, ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::until(Condition condition) {
        Guard guard;
        if (_count > 0) {
            _queue[(uint8_t)((_head + _count - 1) % Q)].condition = condition;
            return true;
        }
        if (_running) {
            _current.condition = condition;
            return true;
        }
        return false;
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::stop() {
        Guard guard;
        _count = 0;
        _running = false;
        standStill();
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::update() {
        Guard guard;
        _left.update();
        _right.update();
        uint32_t now = LeftBridge::nowMs();
        if (_running) {
            bool ended = false;
            if (_current.ms != 0 && now - _startMs >= _current.ms) {
                ended = true;                                   // its time is up
            } else if (_current.ms == 0 && _count > 0) {
                ended = true;                                   // no time, and a next move is there
            } else if (_current.ms == 0 && _failsafeMs != 0 && now - _lastMoveMs >= _failsafeMs) {
                ended = true;                                   // no time, and no news for too long
            } else if (_current.condition != nullptr && _current.condition()) {
                ended = true;                                   // until() came true
            }
            if (!ended) {
                return;
            }
            _running = false;
            if (_count == 0) {
                standStill();
                return;
            }
        }
        if (_count > 0) {
            Move move = _queue[_head];
            _head = (uint8_t)((_head + 1) % Q);
            _count--;
            start(move);
        }
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::isBusy() const {
        Guard guard;
        return _count > 0 || (_running && _current.ms != 0);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::isRunning() const {
        Guard guard;
        return _running || _count > 0;
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::wait() {
        while (isBusy()) {
            update();
        }
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline uint8_t TwoWheelCar<Board, S, P12, P34, Q>::queued() const {
        Guard guard;
        return _count;
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline uint32_t TwoWheelCar<Board, S, P12, P34, Q>::moveMs() const {
        Guard guard;
        return _running ? LeftBridge::nowMs() - _startMs : 0;
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline int16_t TwoWheelCar<Board, S, P12, P34, Q>::leftSpeed() const {
        Guard guard;
        return _left.speed();
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline int16_t TwoWheelCar<Board, S, P12, P34, Q>::rightSpeed() const {
        Guard guard;
        return _right.speed();
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::setRampMs(uint16_t ms) {
        _left.setRampMs(ms);
        _right.setRampMs(ms);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::setMinDuty(uint8_t duty) {
        _left.setMinDuty(duty);
        _right.setMinDuty(duty);
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::setTrim(uint8_t leftPercent, uint8_t rightPercent) {
        Guard guard;
        _leftTrim = leftPercent < 1 ? 1 : leftPercent > 100 ? 100 : leftPercent;
        _rightTrim = rightPercent < 1 ? 1 : rightPercent > 100 ? 100 : rightPercent;
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::setFailsafeMs(uint16_t ms) {
        Guard guard;
        _failsafeMs = ms;
        _lastMoveMs = LeftBridge::nowMs();
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline bool TwoWheelCar<Board, S, P12, P34, Q>::push(Kind kind, int16_t left, int16_t right, uint16_t ms) {
        Guard guard;
        // time goes on even when the queue is full (a loop that only queues moves still drives the car)
        update();
        if (_count >= Q) {
            return false;
        }
        _queue[(uint8_t)((_head + _count) % Q)] = {kind, left, right, ms, nullptr};
        _count++;
        _lastMoveMs = LeftBridge::nowMs();
        update();                           // idle (or a move without a time): starts now
        return true;
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::start(const Move &move) {
        _current = move;
        _running = true;
        _startMs = LeftBridge::nowMs();
        if (move.kind == BRAKE) {
            standStill();
        } else if (move.kind == COAST) {
            _left.coast();
            _right.coast();
        } else {
            // a wheel at 0 inside a drive (pivot) is held by the brake, not left to roll
            if (move.left == 0) {
                _left.brake();
            } else {
                _left.setSpeed((int16_t)((int32_t)move.left * _leftTrim / 100));
            }
            if (move.right == 0) {
                _right.brake();
            } else {
                _right.setSpeed((int16_t)((int32_t)move.right * _rightTrim / 100));
            }
        }
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline void TwoWheelCar<Board, S, P12, P34, Q>::standStill() {
        _left.brake();
        _right.brake();
    }

    template <class Board, Side S, Turn P12, Turn P34, uint8_t Q>
    inline int16_t TwoWheelCar<Board, S, P12, P34, Q>::clamp(int32_t speed) {
        return (int16_t)(speed > 255 ? 255 : speed < -255 ? -255 : speed);
    }
}

#endif
