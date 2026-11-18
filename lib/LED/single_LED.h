#ifndef LED_SINGLE_LED_H
#define LED_SINGLE_LED_H

#include "brightness.h"

namespace LED {
    // one LED channel on a PWM pin: the core of every module (module/LED_2_pins.h, RGB_LED_4_pins.h ...)
    //
    // Pwm: a struct providing these static functions (see lib/Uno/Uno_pin.h, UnoPwmPin)
    //   void init()               pin LOW
    //   void write(uint8_t duty)  0 = always LOW, 255 = always HIGH
    //
    // ACTIVE_HIGH: true if the pin -> resistor -> LED + -> LED - -> GND (lit while HIGH);
    //              false if 5V -> LED + -> LED - -> resistor -> pin (lit while LOW)
    // true for a PWM output (Uno::UnoPwmPin, NoPwm: IS_PWM_OUTPUT)
    template <class Pin, class = void> struct IsPwmOutput { static constexpr bool value = false; };
    template <class Pin> struct IsPwmOutput<Pin, decltype((void)Pin::IS_PWM_OUTPUT)> {
        static constexpr bool value = Pin::IS_PWM_OUTPUT;
    };

    template <class Pwm, bool ACTIVE_HIGH = true>
    class SingleLED;

    // a Pwm that does nothing: for a color of a module that is not wired (e.g. the yellow of a traffic light)
    struct NoPwm {
        static constexpr bool IS_PWM_OUTPUT = true;
        static inline void init();
        static inline void write(uint8_t duty);
    };

    template <class Pwm, bool ACTIVE_HIGH>
    class SingleLED {
    public:
        static_assert(IsPwmOutput<Pwm>::value,
                      "LED: the brightness needs a PWM pin (Uno::UnoPwmPin<3, 5, 6, 9, 10 or 11>), or LED::NoPwm for a color not wired");

        // off
        inline void init();

        // 0 = off ~ 255 = full, as the eye sees it (see toDuty)
        inline void set(uint8_t brightness);

        // the duty itself, no correction
        inline void setDuty(uint8_t duty);

        inline uint8_t brightness() const;

        // the time of the Pwm (for tools such as Effect)
        static inline uint32_t nowMs() {
            return Pwm::nowMs();
        }

    private:
        uint8_t _brightness = 0;
    };

    // ================= definitions =================

    inline void NoPwm::init() {
    }

    inline void NoPwm::write(uint8_t duty) {
        (void)duty;
    }

    template <class Pwm, bool ACTIVE_HIGH>
    inline void SingleLED<Pwm, ACTIVE_HIGH>::init() {
        Pwm::init();
        set(0);
    }

    template <class Pwm, bool ACTIVE_HIGH>
    inline void SingleLED<Pwm, ACTIVE_HIGH>::set(uint8_t brightness) {
        _brightness = brightness;
        setDuty(toDuty(brightness));
    }

    template <class Pwm, bool ACTIVE_HIGH>
    inline void SingleLED<Pwm, ACTIVE_HIGH>::setDuty(uint8_t duty) {
        Pwm::write(ACTIVE_HIGH ? duty : (uint8_t)(255 - duty));
    }

    template <class Pwm, bool ACTIVE_HIGH>
    inline uint8_t SingleLED<Pwm, ACTIVE_HIGH>::brightness() const {
        return _brightness;
    }
}

#endif
