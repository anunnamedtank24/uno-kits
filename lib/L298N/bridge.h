#ifndef L298N_BRIDGE_H
#define L298N_BRIDGE_H

#include <stdint.h>

namespace L298N {
    // one of the two H-bridges of the L298N (channel A: IN1, IN2, ENA -> OUT1, OUT2; channel B: IN3, IN4, ENB -> OUT3, OUT4)
    //
    // an H-bridge is 4 switches around the motor: closing them crosswise sends the current one way or the other
    //   EN    IN1   IN2   OUT1  OUT2   the motor
    //   HIGH  HIGH  LOW   +     -      forward (drive)
    //   HIGH  LOW   HIGH  -     +      backward (drive)
    //   HIGH  LOW   LOW   -     -      brake: both ends on GND, the motor's own voltage stops it fast
    //   HIGH  HIGH  HIGH  +     +      brake: the same, both ends on the supply
    //   LOW   any   any   off   off    coast: nothing connected, the motor rolls out
    //
    // Bridge<In1, In2, Enable> takes every way the three inputs can be wired; each one is
    //   a PWM pin    (Uno::UnoPwmPin<PIN>, IS_PWM_OUTPUT): on, off and any duty
    //   a pin        (Uno::UnoOutputPin<PIN>): on and off
    //   Ground       wired to GND: always LOW, no pin
    //   Supply       wired to +5V: always HIGH, no pin; EnableJumper is the same (the board's ENA / ENB jumper)
    // and what the channel can do follows from it at compile time (the constants CAN_FORWARD ... below):
    //   forward      IN1 can be HIGH and IN2 LOW         backward: IN1 LOW and IN2 HIGH
    //   brake        IN1 and IN2 can be the same         coast:    EN can be LOW (not Supply / EnableJumper)
    //   speed (PWM)  on EN: drive / coast               (the smoothest, and coasting works)
    //                else on the IN that goes HIGH: drive / brake
    //                else on the IN that goes LOW, inverted: drive / brake
    //                else none: full speed only (any duty != 0 is 255)
    //   what cannot be done is replaced: a direction that cannot be driven brakes (or coasts when braking is
    //   not possible either), a coast that cannot be done brakes
    //
    // every Bridge provides the Bridge functions (DCMotor, Stepper use them):
    //   static void init()                 pins set, coasting
    //   static void drive(int16_t duty)    -255 ~ 255: > 0 forward (OUT1 +), < 0 backward, 0 = coast
    //   static void brake(uint8_t duty)    0 ~ 255: how hard (255 = both ends shorted all the time;
    //                                      less only with EN on a PWM pin: brake, coast, brake ...)
    //   static void coast()                outputs off (where EN can be LOW)
    //   static uint32_t nowMs()  /  static uint32_t nowUs()
    //   static uint8_t stopInterrupts()  /  static void restoreInterrupts(uint8_t saved)
    //                                      (DCMotor, Stepper, TwoWheelCar use them to be safe in the background)
    //
    // In1, In2, Enable: a struct providing these static functions (see lib/Uno/Uno_pin.h), or Ground / Supply
    //   void init()
    //   void write(bool high)              a pin (Uno::UnoOutputPin)
    //   void write(uint8_t duty)           a PWM pin (Uno::UnoPwmPin, with IS_PWM_OUTPUT = true): 0 = LOW, 255 = HIGH
    //   at least one of the three is a real pin, and it gives the time and the interrupts:
    //   uint32_t nowMs()  /  uint32_t nowUs()  /  uint8_t stopInterrupts()  /  void restoreInterrupts(uint8_t saved)

    // an input wired to GND instead of to a pin: always LOW, needs no pin
    //   In2 = Ground: only forward (IN1 - IN2 > 0); drive(< 0) brakes
    //   In1 = Ground: only backward;                drive(> 0) brakes
    // saves a pin a motor (e.g. a car that never backs up: 2 pins + 2 GND instead of 4 pins)
    struct Ground {
        static constexpr bool IS_GROUND = true;
        static inline void init() {
        }
        // write(bool) of a pin and write(uint8_t duty) of a PWM pin: both ignored
        template <class Level>
        static inline void write(Level) {
        }
    };

    // an input wired to +5V instead of to a pin: always HIGH, needs no pin
    //   Enable = Supply: EN always on (the same as the jumper: EnableJumper); coast() brakes
    //   In1 = Supply, In2 = Ground: always forward, speed and coast by a PWM pin on EN (1 pin a motor)
    //   In1 = Supply, In2 = a pin: forward (IN2 LOW) and brake (IN2 HIGH), not backward
    struct Supply {
        static constexpr bool IS_SUPPLY = true;
        static inline void init() {
        }
        template <class Level>
        static inline void write(Level) {
        }
    };

    // the board's ENA / ENB jumper left on: EN is on +5V all the time, nothing to connect (see Supply)
    using EnableJumper = Supply;

    // true for a PWM output (Uno::UnoPwmPin: IS_PWM_OUTPUT), false for an on / off pin
    template <class Pin, class = void> struct IsPwmOutput { static constexpr bool value = false; };
    template <class Pin> struct IsPwmOutput<Pin, decltype((void)Pin::IS_PWM_OUTPUT)> {
        static constexpr bool value = Pin::IS_PWM_OUTPUT;
    };

    // true for Ground (a type with IS_GROUND = true), false for a real pin
    template <class Pin, class = void> struct IsGround { static constexpr bool value = false; };
    template <class Pin> struct IsGround<Pin, decltype((void)Pin::IS_GROUND)> {
        static constexpr bool value = Pin::IS_GROUND;
    };

    // true for Supply / EnableJumper (a type with IS_SUPPLY = true), false for a real pin
    template <class Pin, class = void> struct IsSupply { static constexpr bool value = false; };
    template <class Pin> struct IsSupply<Pin, decltype((void)Pin::IS_SUPPLY)> {
        static constexpr bool value = Pin::IS_SUPPLY;
    };

    // a real pin: not wired to GND or +5V
    template <class Pin>
    struct IsPin {
        static constexpr bool value = !IsGround<Pin>::value && !IsSupply<Pin>::value;
    };

    // picks one of two types (no <type_traits> on the AVR)
    template <bool FIRST, class First, class Second> struct Pick { using Type = First; };
    template <class First, class Second> struct Pick<false, First, Second> { using Type = Second; };

    // the pin that gives the time and the interrupts: In1, else In2, else Enable (the first real pin)
    template <class In1, class In2, class Enable = Ground>
    using ClockOf = typename Pick<IsPin<In1>::value, In1,
                                  typename Pick<IsPin<In2>::value, In2, Enable>::Type>::Type;

    // interrupts off while it lives: { InterruptGuard<Bridge> guard; ... } (safe with Uno::UnoBackground)
    template <class Clock>
    struct InterruptGuard {
        uint8_t saved;
        inline InterruptGuard() : saved(Clock::stopInterrupts()) {
        }
        inline ~InterruptGuard() {
            Clock::restoreInterrupts(saved);
        }
    };

    // -255 ~ 255 -> 0 ~ 255
    inline uint8_t dutyOf(int16_t duty) {
        if (duty < 0) {
            duty = (int16_t)-duty;
        }
        return duty > 255 ? 255 : (uint8_t)duty;
    }

    // HIGH, LOW, a duty on any input (nothing on Ground / Supply)
    template <class Pin>
    inline void setHigh() {
        if constexpr (!IsPin<Pin>::value) {
        } else if constexpr (IsPwmOutput<Pin>::value) {
            Pin::write((uint8_t)255);
        } else {
            Pin::write(true);
        }
    }

    template <class Pin>
    inline void setLow() {
        if constexpr (!IsPin<Pin>::value) {
        } else if constexpr (IsPwmOutput<Pin>::value) {
            Pin::write((uint8_t)0);
        } else {
            Pin::write(false);
        }
    }

    template <class Pin>
    inline void setDuty(uint8_t duty) {
        if constexpr (!IsPin<Pin>::value) {
        } else if constexpr (IsPwmOutput<Pin>::value) {
            Pin::write(duty);
        } else {
            Pin::write(duty != 0);
        }
    }

    // one H-bridge, any wiring of IN1, IN2, EN (see the top of this file)
    template <class In1, class In2, class Enable>
    struct Bridge {
        static_assert(!IsGround<Enable>::value,
                      "L298N::Bridge: EN on GND keeps the channel off (connect EN to a pin, or put the jumper on)");
        static_assert(IsPin<In1>::value || IsPin<In2>::value || IsPin<Enable>::value,
                      "L298N::Bridge: IN1, IN2 and EN are all wired to GND / +5V: nothing to control "
                      "(connect at least one to a pin)");

    private:
        static constexpr bool IN1_HIGH = !IsGround<In1>::value;      // IN1 can be HIGH
        static constexpr bool IN1_LOW = !IsSupply<In1>::value;
        static constexpr bool IN2_HIGH = !IsGround<In2>::value;
        static constexpr bool IN2_LOW = !IsSupply<In2>::value;

    public:
        // what this wiring can do (compile time)
        static constexpr bool CAN_FORWARD = IN1_HIGH && IN2_LOW;
        static constexpr bool CAN_BACKWARD = IN1_LOW && IN2_HIGH;
        static constexpr bool BOTH_WAYS = CAN_FORWARD && CAN_BACKWARD;
        static constexpr bool CAN_BRAKE = (IN1_LOW && IN2_LOW) || (IN1_HIGH && IN2_HIGH);
        static constexpr bool CAN_COAST = !IsSupply<Enable>::value;
        // the speed can be set (PWM somewhere): else full speed only
        static constexpr bool HAS_SPEED =
            IsPwmOutput<Enable>::value || IsPwmOutput<In1>::value || IsPwmOutput<In2>::value;
        // brake(duty) with less than 255 (EN on a PWM pin)
        static constexpr bool HAS_BRAKE_DUTY = IsPwmOutput<Enable>::value;
        // the pins it takes on the board (0 ~ 3)
        static constexpr uint8_t PIN_COUNT =
            (uint8_t)(IsPin<In1>::value + IsPin<In2>::value + IsPin<Enable>::value);

        static_assert(CAN_FORWARD || CAN_BACKWARD,
                      "L298N::Bridge: IN1 and IN2 are both on GND or both on +5V: the motor can never turn");

        static inline void init();
        static inline void drive(int16_t duty);
        static inline void brake(uint8_t duty = 255);
        static inline void coast();
        static inline uint32_t nowMs();
        static inline uint32_t nowUs();
        static inline uint8_t stopInterrupts() {
            return ClockOf<In1, In2, Enable>::stopInterrupts();
        }
        static inline void restoreInterrupts(uint8_t saved) {
            ClockOf<In1, In2, Enable>::restoreInterrupts(saved);
        }

    private:
        // drives with High HIGH and Low LOW (forward: In1, In2; backward: In2, In1)
        template <class High, class Low>
        static inline void driveWay(uint8_t duty);

        // stands still the best way possible: brake, else coast
        static inline void hold();
    };

    // the ENA / ENB jumper left on, speed by PWM on IN1 / IN2 (both on PWM pins, or one on Ground):
    //   forward duty d: IN1 = PWM d, IN2 = LOW -> drive d of the time, brake the rest
    //   (drive / brake instead of drive / coast: the speed follows the duty more evenly, but it is a bit noisier)
    // coast() is not possible (EN is always HIGH): it brakes
    template <class In1, class In2>
    using InputPwmBridge = Bridge<In1, In2, EnableJumper>;

    // ================= definitions =================

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::init() {
        In1::init();
        In2::init();
        Enable::init();
        coast();
    }

    template <class In1, class In2, class Enable>
    template <class High, class Low>
    inline void Bridge<In1, In2, Enable>::driveWay(uint8_t duty) {
        // the LOW side first: switching direction passes through a brake, never through the wrong way
        if constexpr (IsPwmOutput<Enable>::value) {
            setLow<Low>();
            setHigh<High>();
            Enable::write(duty);                // drive / coast
        } else if constexpr (IsPwmOutput<High>::value) {
            setLow<Low>();
            High::write(duty);                  // drive / brake (both LOW)
            setHigh<Enable>();
        } else if constexpr (IsPwmOutput<Low>::value) {
            setHigh<High>();
            Low::write((uint8_t)(255 - duty));  // drive / brake (both HIGH)
            setHigh<Enable>();
        } else {
            setLow<Low>();
            setHigh<High>();
            setHigh<Enable>();                  // full speed only
        }
    }

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::drive(int16_t duty) {
        if (duty == 0) {
            coast();
        } else if (duty > 0) {
            if constexpr (CAN_FORWARD) {
                driveWay<In1, In2>(dutyOf(duty));
            } else {
                hold();
            }
        } else {
            if constexpr (CAN_BACKWARD) {
                driveWay<In2, In1>(dutyOf(duty));
            } else {
                hold();
            }
        }
    }

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::brake(uint8_t duty) {
        (void)duty;                             // used only with EN on a PWM pin
        if constexpr (!CAN_BRAKE) {
            coast();
        } else {
            // both LOW where possible, else both HIGH (an IN on +5V)
            if constexpr (IN1_LOW && IN2_LOW) {
                setLow<In1>();
                setLow<In2>();
            } else {
                setHigh<In1>();
                setHigh<In2>();
            }
            if constexpr (IsPwmOutput<Enable>::value) {
                Enable::write(duty);            // brake duty of the time, coast the rest
            } else {
                setHigh<Enable>();
            }
        }
    }

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::coast() {
        if constexpr (!CAN_COAST) {
            brake();
        } else {
            // EN LOW: off; the INs LOW too where they can be (never a drive when EN comes back)
            setLow<Enable>();
            if constexpr (IN1_LOW && IN2_LOW) {
                setLow<In1>();
                setLow<In2>();
            }
        }
    }

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::hold() {
        if constexpr (CAN_BRAKE) {
            brake();
        } else {
            coast();
        }
    }

    template <class In1, class In2, class Enable>
    inline uint32_t Bridge<In1, In2, Enable>::nowMs() {
        return ClockOf<In1, In2, Enable>::nowMs();
    }

    template <class In1, class In2, class Enable>
    inline uint32_t Bridge<In1, In2, Enable>::nowUs() {
        return ClockOf<In1, In2, Enable>::nowUs();
    }
}

#endif
