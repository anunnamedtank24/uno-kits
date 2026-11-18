#ifndef UNO_PIN_H
#define UNO_PIN_H

#include "Uno.h"

namespace Uno {
    // the pins, one family: every kind inherits UnoBasicPin, which inherits UnoBoard
    //
    //   UnoBoard                       time, delays, flash, interrupts on / off (Uno.h)
    //   └── UnoBasicPin<PIN>           the pin number and what it can do (compile-time constants)
    //       ├── UnoInputPin<PIN, PULLUP>    Pin (input):  init, read                    switch, TCRT5000, ultrasonic Echo ...
    //       │   ├── UnoInterruptPin<...>    InterruptPin: init(onChange), read          NEC (Uno_interrupt_pin.h)
    //       │   └── UnoAnalogPin<...>       AnalogPin:    init, read, readAnalog ...    analog (Uno_analog_pin.h)
    //       ├── UnoOutputPin<PIN>           Pin (output): init, write, toggle, isHigh   active buzzer, L298N IN, Trig ...
    //       └── UnoPwmPin<PIN>              Pwm:          init, write(duty), duty       LED, L298N EN ...
    //
    // a library asks for the smallest one it needs (e.g. Pin: init + read), so any of the bigger ones fits too:
    // an UnoInterruptPin or an UnoAnalogPin also works where an UnoInputPin is asked for

    // the base of every pin: its number and what it can do
    template <uint8_t PIN>
    struct UnoBasicPin : UnoBoard {
        static_assert(exists(PIN), "Uno::UnoInputPin: PIN must be 0 ~ 19 (D0 ~ D13 = 0 ~ 13, A0 ~ A5 = 14 ~ 19)");
        static_assert(warn::SerialPin<(PIN == 0 || PIN == 1)>::ok, "");

        // the Arduino pin number
        static constexpr uint8_t NUMBER = PIN;

        // what the pin can do (see Uno.h, PIN_FUNCTIONS)
        static constexpr bool CAN_PWM = has(PIN, function::PWM);
        static constexpr bool CAN_ANALOG = has(PIN, function::ANALOG_IN);
        static constexpr bool IS_SERIAL = has(PIN, function::UART);
        static constexpr bool IS_SPI = has(PIN, function::SPI);
        static constexpr bool IS_LED = has(PIN, function::LED);
    };

    // one digital input pin, e.g. the OUT of a switch module (switches::Switch3Pins)
    // INTERNAL_PULLUP: the ~35 kΩ internal pull-up, for a bare button or switch to GND
    // (modules with GND / VCC / OUT drive OUT themselves: leave it off)
    template <uint8_t PIN, bool INTERNAL_PULLUP = false>
    struct UnoInputPin : UnoBasicPin<PIN> {
        // ---------- Pin (input) functions ----------

        static inline void init();

        // true = HIGH
        static inline bool read();
    };

    // the old name of UnoInputPin (every program and README uses it)
    template <uint8_t PIN, bool INTERNAL_PULLUP = false>
    using UnoPin = UnoInputPin<PIN, INTERNAL_PULLUP>;

    // one digital output pin, e.g. an active buzzer (buzzer::ActiveBuzzer2Pins): LOW after init()
    // a pin gives at most 20 mA (40 mA absolute): drive anything hungrier through a transistor or a driver
    template <uint8_t PIN>
    struct UnoOutputPin : UnoBasicPin<PIN> {
        // ---------- Pin (output) functions ----------

        // LOW first, then output (no HIGH glitch)
        static inline void init();

        // true = HIGH
        static inline void write(bool high);

        // HIGH <-> LOW
        static inline void toggle();

        // the level it is driving now
        static inline bool isHigh();
    };

    // one PWM output pin (3, 5, 6, 9, 10, 11), e.g. an LED (LED::SingleLED), the EN of an L298N: LOW after init()
    // ~976 Hz, duty 0 ~ 255 (see Uno::pwmSet): D9 / D10 use Timer1 (not with UnoTone / UnoPcmOut),
    // D3 / D11 Timer2 (not with UnoPcmOut), D5 / D6 Timer0 (shared with millis: only the output is used)
    template <uint8_t PIN>
    struct UnoPwmPin : UnoBasicPin<PIN> {
        static_assert(has(PIN, function::PWM), "Uno::UnoPwmPin: PIN must be a PWM pin (3, 5, 6, 9, 10, 11)");

        // ---------- Pwm functions ----------

        // marks a PWM output (a library can check that it got one: see L298N, LED)
        static constexpr bool IS_PWM_OUTPUT = true;

        static inline void init();

        // 0 = LOW all the time, 255 = HIGH all the time
        static inline void write(uint8_t duty);

        // the duty written last
        static inline uint8_t duty();

    private:
        static inline uint8_t _duty = 0;
    };

    // ================= definitions =================

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline void UnoInputPin<PIN, INTERNAL_PULLUP>::init() {
        pinInput<PIN>(INTERNAL_PULLUP);
    }

    template <uint8_t PIN, bool INTERNAL_PULLUP>
    inline bool UnoInputPin<PIN, INTERNAL_PULLUP>::read() {
        return pinRead<PIN>();
    }

    template <uint8_t PIN>
    inline void UnoOutputPin<PIN>::init() {
        pinLow<PIN>();
        pinOutput<PIN>();
    }

    template <uint8_t PIN>
    inline void UnoOutputPin<PIN>::write(bool high) {
        if (high) {
            pinHigh<PIN>();
        } else {
            pinLow<PIN>();
        }
    }

    template <uint8_t PIN>
    inline void UnoOutputPin<PIN>::toggle() {
        write(!isHigh());
    }

    template <uint8_t PIN>
    inline bool UnoOutputPin<PIN>::isHigh() {
        // the PORT bit is what an output drives
        if (PIN < 8) {
            return (PORTD & mask(PIN)) != 0;
        }
        if (PIN < 14) {
            return (PORTB & mask(PIN)) != 0;
        }
        return (PORTC & mask(PIN)) != 0;
    }

    template <uint8_t PIN>
    inline void UnoPwmPin<PIN>::init() {
        write(0);
    }

    template <uint8_t PIN>
    inline void UnoPwmPin<PIN>::write(uint8_t duty) {
        _duty = duty;
        pwmSet<PIN>(duty);
    }

    template <uint8_t PIN>
    inline uint8_t UnoPwmPin<PIN>::duty() {
        return _duty;
    }
}

#endif
