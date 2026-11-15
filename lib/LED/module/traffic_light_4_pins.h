#ifndef LED_TRAFFIC_LIGHT_4_PINS_H
#define LED_TRAFFIC_LIGHT_4_PINS_H

#include "../single_LED.h"

namespace LED {
    // a traffic light module, 4 pins: R, Y, G, GND (GND may be marked "-")
    // three LEDs with their resistors on the board: each color pin straight to a PWM pin, no resistor;
    // lit while its pin is HIGH
    //
    // a color not used: leave its pin unconnected and give it LED::NoPwm,
    // e.g. TrafficLight4Pins<Uno::UnoPwmPin<3>, LED::NoPwm, Uno::UnoPwmPin<5>> for red and green only
    template <class RedPwm, class YellowPwm, class GreenPwm>
    class TrafficLight4Pins {
    public:
        // off
        inline void init();

        // each 0 = off ~ 255 = full, as the eye sees it
        inline void set(uint8_t red, uint8_t yellow, uint8_t green);

        inline void setRed(uint8_t brightness);
        inline void setYellow(uint8_t brightness);
        inline void setGreen(uint8_t brightness);

        inline uint8_t red() const;
        inline uint8_t yellow() const;
        inline uint8_t green() const;

    private:
        SingleLED<RedPwm, true> _red;
        SingleLED<YellowPwm, true> _yellow;
        SingleLED<GreenPwm, true> _green;
    };

    // ================= definitions =================

    template <class RedPwm, class YellowPwm, class GreenPwm>
    inline void TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>::init() {
        _red.init();
        _yellow.init();
        _green.init();
    }

    template <class RedPwm, class YellowPwm, class GreenPwm>
    inline void TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>::set(uint8_t red, uint8_t yellow, uint8_t green) {
        _red.set(red);
        _yellow.set(yellow);
        _green.set(green);
    }

    template <class RedPwm, class YellowPwm, class GreenPwm>
    inline void TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>::setRed(uint8_t brightness) {
        _red.set(brightness);
    }

    template <class RedPwm, class YellowPwm, class GreenPwm>
    inline void TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>::setYellow(uint8_t brightness) {
        _yellow.set(brightness);
    }

    template <class RedPwm, class YellowPwm, class GreenPwm>
    inline void TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>::setGreen(uint8_t brightness) {
        _green.set(brightness);
    }

    template <class RedPwm, class YellowPwm, class GreenPwm>
    inline uint8_t TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>::red() const {
        return _red.brightness();
    }

    template <class RedPwm, class YellowPwm, class GreenPwm>
    inline uint8_t TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>::yellow() const {
        return _yellow.brightness();
    }

    template <class RedPwm, class YellowPwm, class GreenPwm>
    inline uint8_t TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>::green() const {
        return _green.brightness();
    }
}

#endif
