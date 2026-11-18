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
    // speed: EN switched on / off fast (PWM): duty 128 = drive half the time, coast the other half
    //
    // every type of this file provides the Bridge functions (DCMotor, Stepper use them):
    //   static void init()                 pins set, coasting
    //   static void drive(int16_t duty)    -255 ~ 255: > 0 forward (OUT1 +), < 0 backward, 0 = coast
    //   static void brake(uint8_t duty)    0 ~ 255: how hard (255 = both ends shorted all the time)
    //   static void coast()                outputs off (where the board allows: see EnableJumper)
    //   static uint32_t nowMs()  /  static uint32_t nowUs()
    //   static uint8_t stopInterrupts()  /  static void restoreInterrupts(uint8_t saved)
    //                                      (DCMotor, Stepper, TwoWheelCar use them to be safe in the background)

    // an IN wired to GND instead of to a pin: always LOW, needs no pin
    // (In1 or In2 of Bridge / InputPwmBridge, not both): the motor then turns only one way, the other way brakes
    //   In2 = Ground: only forward (IN1 - IN2 > 0); drive(< 0) brakes
    //   In1 = Ground: only backward;                drive(> 0) brakes
    // saves a pin a motor (e.g. a car that never backs up: 2 pins + 2 GND instead of 4 pins)
    struct Ground {
        static constexpr bool IS_GROUND = true;
        static inline void init() {
        }
        // write(bool) of a digital pin and write(uint8_t duty) of a PWM pin: both ignored
        template <class Level>
        static inline void write(Level) {
        }
    };

    // true for a PWM output (Uno::UnoPwmPin: IS_PWM_OUTPUT), false for an on / off pin
    template <class Pin, class = void> struct IsPwmOutput { static constexpr bool value = false; };
    template <class Pin> struct IsPwmOutput<Pin, decltype((void)Pin::IS_PWM_OUTPUT)> {
        static constexpr bool value = Pin::IS_PWM_OUTPUT;
    };

    // picks one of two types (no <type_traits> on the AVR)
    template <bool FIRST, class First, class Second> struct Pick { using Type = First; };
    template <class First, class Second> struct Pick<false, First, Second> { using Type = Second; };

    // true for Ground (a type with IS_GROUND = true), false for a real pin
    template <class Pin, class = void> struct IsGround { static constexpr bool value = false; };
    template <class Pin> struct IsGround<Pin, decltype((void)Pin::IS_GROUND)> {
        static constexpr bool value = Pin::IS_GROUND;
    };

    // the pin that gives the time: In1, or In2 if In1 is Ground
    template <class In1, class In2>
    using ClockOf = typename Pick<IsGround<In1>::value, In2, In1>::Type;

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

    // EN on a PWM pin (the board's ENA / ENB jumper removed): drive, brake, coast and speed all work
    //
    // In1, In2: a struct providing these static functions (see lib/Uno/Uno_pin.h, UnoOutputPin), or Ground
    //   void init()  /  void write(bool high)  /  uint32_t nowMs()  /  uint32_t nowUs()
    //   uint8_t stopInterrupts()  /  void restoreInterrupts(uint8_t saved)
    // Enable: a struct providing these static functions (see lib/Uno/Uno_pin.h, UnoPwmPin; or EnableJumper)
    //   void init()  /  void write(uint8_t duty)   0 = LOW, 255 = HIGH
    template <class In1, class In2, class Enable>
    struct Bridge {
        static_assert(!(IsGround<In1>::value && IsGround<In2>::value),
                      "L298N::Bridge: In1 and In2 cannot both be Ground (the motor could not turn)");
        static_assert(!IsPwmOutput<In1>::value && !IsPwmOutput<In2>::value,
                      "L298N::Bridge: IN1 / IN2 are on / off pins (Uno::UnoOutputPin); for speed by PWM on IN "
                      "(EN jumpers on) use L298N::InputPwmBridge / L298NBoardJumpers");
        static_assert(IsPwmOutput<Enable>::value,
                      "L298N::Bridge: EN must be a PWM pin (Uno::UnoPwmPin<3, 5, 6, 9, 10 or 11>) or L298N::EnableJumper");

        // false if one IN is Ground (only one way)
        static constexpr bool BOTH_WAYS = !IsGround<In1>::value && !IsGround<In2>::value;

        static inline void init();
        static inline void drive(int16_t duty);
        static inline void brake(uint8_t duty = 255);
        static inline void coast();
        static inline uint32_t nowMs();
        static inline uint32_t nowUs();
        static inline uint8_t stopInterrupts() {
            return ClockOf<In1, In2>::stopInterrupts();
        }
        static inline void restoreInterrupts(uint8_t saved) {
            ClockOf<In1, In2>::restoreInterrupts(saved);
        }
    };

    // the board's ENA / ENB jumper left on: EN is HIGH all the time, nothing to connect
    // with Bridge<In1, In2, EnableJumper>: full speed only (any duty != 0 is 255), and coast() brakes
    // (EN cannot go LOW); for speed with the jumper on, use InputPwmBridge
    struct EnableJumper {
        static constexpr bool IS_PWM_OUTPUT = true;      // fits where a PWM pin is asked for (always 255)
        static inline void init() {
        }
        static inline void write(uint8_t) {
        }
    };

    // the ENA / ENB jumper left on, speed by PWM on IN1 / IN2 (both on PWM pins)
    //   forward duty d: IN1 = PWM d, IN2 = LOW -> drive d of the time, brake the rest
    //   (drive / brake instead of drive / coast: the speed follows the duty more evenly, but it is a bit noisier)
    // coast() is not possible (EN is always HIGH): it brakes
    //
    // In1, In2: a struct providing these static functions (see lib/Uno/Uno_pin.h, UnoPwmPin), or Ground
    //   void init()  /  void write(uint8_t duty)  /  uint32_t nowMs()  /  uint32_t nowUs()
    //   uint8_t stopInterrupts()  /  void restoreInterrupts(uint8_t saved)
    template <class In1, class In2>
    struct InputPwmBridge {
        static_assert(!(IsGround<In1>::value && IsGround<In2>::value),
                      "L298N::InputPwmBridge: In1 and In2 cannot both be Ground (the motor could not turn)");
        static_assert((IsPwmOutput<In1>::value || IsGround<In1>::value) && (IsPwmOutput<In2>::value || IsGround<In2>::value),
                      "L298N::InputPwmBridge (L298NBoardJumpers): every IN must be a PWM pin "
                      "(Uno::UnoPwmPin<3, 5, 6, 9, 10 or 11>) or L298N::Ground");

        static constexpr bool BOTH_WAYS = !IsGround<In1>::value && !IsGround<In2>::value;

        static inline void init();
        static inline void drive(int16_t duty);
        static inline void brake(uint8_t duty = 255);
        static inline void coast();
        static inline uint32_t nowMs();
        static inline uint32_t nowUs();
        static inline uint8_t stopInterrupts() {
            return ClockOf<In1, In2>::stopInterrupts();
        }
        static inline void restoreInterrupts(uint8_t saved) {
            ClockOf<In1, In2>::restoreInterrupts(saved);
        }
    };

    // -255 ~ 255 -> 0 ~ 255
    inline uint8_t dutyOf(int16_t duty) {
        if (duty < 0) {
            duty = (int16_t)-duty;
        }
        return duty > 255 ? 255 : (uint8_t)duty;
    }

    // ================= definitions =================

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::init() {
        In1::init();
        In2::init();
        Enable::init();
        coast();
    }

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::drive(int16_t duty) {
        if (duty == 0) {
            coast();
            return;
        }
        // direction first, then EN: no moment of the wrong direction
        In1::write(duty > 0);
        In2::write(duty < 0);
        Enable::write(dutyOf(duty));
    }

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::brake(uint8_t duty) {
        In1::write(false);
        In2::write(false);
        Enable::write(duty);
    }

    template <class In1, class In2, class Enable>
    inline void Bridge<In1, In2, Enable>::coast() {
        // EN LOW: off; IN both LOW too, so with EnableJumper it is a brake, never a drive
        Enable::write(0);
        In1::write(false);
        In2::write(false);
    }

    template <class In1, class In2, class Enable>
    inline uint32_t Bridge<In1, In2, Enable>::nowMs() {
        return ClockOf<In1, In2>::nowMs();
    }

    template <class In1, class In2, class Enable>
    inline uint32_t Bridge<In1, In2, Enable>::nowUs() {
        return ClockOf<In1, In2>::nowUs();
    }

    template <class In1, class In2>
    inline void InputPwmBridge<In1, In2>::init() {
        In1::init();
        In2::init();
        coast();
    }

    template <class In1, class In2>
    inline void InputPwmBridge<In1, In2>::drive(int16_t duty) {
        if (duty > 0) {
            In2::write(0);
            In1::write(dutyOf(duty));
        } else if (duty < 0) {
            In1::write(0);
            In2::write(dutyOf(duty));
        } else {
            coast();
        }
    }

    template <class In1, class In2>
    inline void InputPwmBridge<In1, In2>::brake(uint8_t) {
        // both LOW with EN HIGH: always a full brake
        In1::write(0);
        In2::write(0);
    }

    template <class In1, class In2>
    inline void InputPwmBridge<In1, In2>::coast() {
        brake();
    }

    template <class In1, class In2>
    inline uint32_t InputPwmBridge<In1, In2>::nowMs() {
        return ClockOf<In1, In2>::nowMs();
    }

    template <class In1, class In2>
    inline uint32_t InputPwmBridge<In1, In2>::nowUs() {
        return ClockOf<In1, In2>::nowUs();
    }
}

#endif
