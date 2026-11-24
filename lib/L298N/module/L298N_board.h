#ifndef L298N_L298N_BOARD_H
#define L298N_L298N_BOARD_H

#include "../bridge.h"

namespace L298N {
    // the common red L298N board (often printed HW-095): the chip on a heat sink, a 5 V regulator (78M05),
    // 8 protection diodes, screw terminals, 2 jumpers on ENA / ENB and 1 jumper on the regulator (5V-EN)
    //
    //   terminal   what                                    to the UNO / elsewhere
    //   OUT1 OUT2  motor A (or stepper coil A)             the motor's 2 wires
    //   OUT3 OUT4  motor B (or stepper coil B)             the motor's 2 wires
    //   +12V (VS)  the motors' supply, 5 ~ 35 V            battery / adapter +   (not the UNO's 5V)
    //   GND        common ground                           battery -  AND the UNO's GND (both!)
    //   +5V        5V-EN jumper on (VS <= 12 V): 5 V out   may feed the UNO's 5V pin (~ 0.5 A at most)
    //              jumper off (VS > 12 V): 5 V in          from the UNO's 5V pin (the chip's logic needs it)
    //   ENA        channel A on / speed                    jumper on: always on; jumper off: a pin
    //   IN1 IN2    channel A direction                     pins, or wired to GND / +5V
    //   IN3 IN4    channel B direction                     pins, or wired to GND / +5V
    //   ENB        channel B on / speed                    jumper on: always on; jumper off: a pin
    //
    // every input (In1 ~ In4, Ena, Enb) is one of (see bridge.h: what each wiring can do)
    //   Uno::UnoPwmPin<PIN>     a PWM pin (3, 5, 6, 9, 10, 11): speed
    //   Uno::UnoOutputPin<PIN>  any pin: on / off
    //   L298N::Ground           wired to GND (an IN only): that motor goes one way only
    //   L298N::Supply           wired to +5V; on ENA / ENB the same as L298N::EnableJumper (the jumper on)
    //
    // the supply (both optional, checked at compile time):
    //   SUPPLY_MV  the voltage on +12V (VS), mV; 0 = not said, not checked
    //   JUMPER_5V  the 5V-EN jumper: true = on (the board makes its own 5 V, +5V is an output),
    //              false = off (+5V is an input: connect it to the UNO's 5V)
    //
    // the wirings (one row a channel; the other channel the same or different):
    //   EN        IN1, IN2          pins  can                                       type
    //   PWM pin   pin, pin          3     both ways, speed, brake, coast            L298NBoard<UnoPwmPin, UnoOutputPin, UnoOutputPin, ...>
    //   PWM pin   PWM pin, PWM pin  3     the same                                  L298NBoard<UnoPwmPin, UnoPwmPin, UnoPwmPin, ...>
    //   PWM pin   +5V, GND          1     one way, speed, coast (no brake)          L298NBoard<UnoPwmPin, Supply, Ground, ...>
    //   PWM pin   pin, GND          2     one way, speed, brake, coast              L298NBoard<UnoPwmPin, UnoOutputPin, Ground, ...>
    //   pin       pin, pin          3     both ways, full speed, brake, coast       L298NBoard<UnoOutputPin, UnoOutputPin, UnoOutputPin, ...>
    //   jumper    PWM pin, PWM pin  2     both ways, speed, brake (no coast)        L298NBoardJumpers<UnoPwmPin, UnoPwmPin, ...>
    //   jumper    PWM pin, GND      1     one way, speed, brake                     L298NBoardJumpers<UnoPwmPin, Ground, ...>
    //   jumper    pin, pin          2     both ways, full speed, brake              L298NBoardJumpers<UnoOutputPin, UnoOutputPin, ...>
    //   jumper    pin, GND          1     one way, full speed, brake                L298NBoardJumpers<UnoOutputPin, Ground, ...>
    //   both channels as one (parallel), twice the current                          L298NBoardParallel<Enable, In1, In2>
    template <class Ena, class In1, class In2, class In3, class In4, class Enb,
              uint16_t SUPPLY_MV = 0, bool JUMPER_5V = true>
    struct L298NBoard;

    // both EN jumpers on (ENA, ENB not connected): the speed by PWM on the INs, coast brakes (see InputPwmBridge)
    template <class In1, class In2, class In3, class In4, uint16_t SUPPLY_MV = 0, bool JUMPER_5V = true>
    using L298NBoardJumpers = L298NBoard<EnableJumper, In1, In2, In3, In4, EnableJumper, SUPPLY_MV, JUMPER_5V>;

    // compile-time warnings of the supply: static_assert(L298N::warn::Xxx<condition>::ok, "")
    namespace warn {
        template <bool> struct RegulatorHot { static constexpr bool ok = true; };
        template <> struct RegulatorHot<true> {
            __attribute__((deprecated("L298N: VS above 12 V with the 5V-EN jumper on: the regulator gets too hot "
                                      "(remove the jumper and feed +5V from the UNO's 5V)")))
            static constexpr bool ok = true;
        };

        template <bool> struct RegulatorLow { static constexpr bool ok = true; };
        template <> struct RegulatorLow<true> {
            __attribute__((deprecated("L298N: VS below 7 V with the 5V-EN jumper on: the regulator cannot make 5 V "
                                      "for the chip's logic (remove the jumper and feed +5V from the UNO's 5V)")))
            static constexpr bool ok = true;
        };

        template <bool> struct SupplyLow { static constexpr bool ok = true; };
        template <> struct SupplyLow<true> {
            __attribute__((deprecated("L298N: VS below 5 V: after the ~ 2 V drop the motors get almost nothing")))
            static constexpr bool ok = true;
        };
    }

    // the checks of the supply, shared by the board types
    template <uint16_t SUPPLY_MV, bool JUMPER_5V>
    struct SupplyCheck {
        static_assert(SUPPLY_MV <= 35000,
                      "L298N::L298NBoard: SUPPLY_MV above 35 V: the board's capacitors are 35 V parts");
        static_assert(warn::RegulatorHot<(JUMPER_5V && SUPPLY_MV > 12000)>::ok, "");
        static_assert(warn::RegulatorLow<(JUMPER_5V && SUPPLY_MV != 0 && SUPPLY_MV < 7000)>::ok, "");
        static_assert(warn::SupplyLow<(SUPPLY_MV != 0 && SUPPLY_MV < 5000)>::ok, "");

        // the voltage on +12V (0 = not said)
        static constexpr uint16_t SUPPLY_MILLIVOLTS = SUPPLY_MV;
        // the 5V-EN jumper on: +5V is a 5 V output (may feed the UNO); off: +5V needs 5 V from the UNO
        static constexpr bool FIVE_V_IS_OUTPUT = JUMPER_5V;
    };

    template <class Ena, class In1, class In2, class In3, class In4, class Enb, uint16_t SUPPLY_MV, bool JUMPER_5V>
    struct L298NBoard : SupplyCheck<SUPPLY_MV, JUMPER_5V> {
        using A = Bridge<In1, In2, Ena>;     // OUT1, OUT2
        using B = Bridge<In3, In4, Enb>;     // OUT3, OUT4
    };

    // one motor on both channels in parallel, for up to ~ 3 A (one channel: 2 A); wire it as the L298N
    // datasheet says (half-bridge 1 with 4, 2 with 3, so the two halves switch together):
    //   OUT1 + OUT4 -> one motor wire      OUT2 + OUT3 -> the other
    //   IN1 + IN4   -> the In1 pin         IN2 + IN3   -> the In2 pin
    //   ENA + ENB   -> the Enable pin (or both jumpers on: EnableJumper)
    // Motor (= A) is the one bridge
    template <class Enable, class In1, class In2, uint16_t SUPPLY_MV = 0, bool JUMPER_5V = true>
    struct L298NBoardParallel : SupplyCheck<SUPPLY_MV, JUMPER_5V> {
        using Motor = Bridge<In1, In2, Enable>;
        using A = Motor;
    };
}

#endif
