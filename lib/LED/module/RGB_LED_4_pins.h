#ifndef LED_RGB_LED_4_PINS_H
#define LED_RGB_LED_4_PINS_H

#include "../single_LED.h"

namespace LED {
    // an RGB LED, 4 legs: R, G, B and the longest one, the common leg; three LEDs in one
    // each color through its own 220 Ω to its own PWM pin (a module with resistors on it: KY-016 ...)
    // (three separate LEDs, each + through 220 Ω to a pin and every - to GND, work the same: common cathode)
    //
    // COMMON_ANODE: false = common cathode (the long leg to GND, a color is lit while its pin is HIGH)
    //               true  = common anode   (the long leg to 5V, a color is lit while its pin is LOW)
    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE = false>
    class RGBLED4Pins {
    public:
        // off
        inline void init();

        // each 0 = off ~ 255 = full, as the eye sees it
        inline void set(uint8_t red, uint8_t green, uint8_t blue);

        inline void setRed(uint8_t brightness);
        inline void setGreen(uint8_t brightness);
        inline void setBlue(uint8_t brightness);

        inline uint8_t red() const;
        inline uint8_t green() const;
        inline uint8_t blue() const;

    private:
        SingleLED<RedPwm, !COMMON_ANODE> _red;
        SingleLED<GreenPwm, !COMMON_ANODE> _green;
        SingleLED<BluePwm, !COMMON_ANODE> _blue;
    };

    // ================= definitions =================

    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE>
    inline void RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>::init() {
        _red.init();
        _green.init();
        _blue.init();
    }

    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE>
    inline void RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>::set(uint8_t red, uint8_t green, uint8_t blue) {
        _red.set(red);
        _green.set(green);
        _blue.set(blue);
    }

    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE>
    inline void RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>::setRed(uint8_t brightness) {
        _red.set(brightness);
    }

    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE>
    inline void RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>::setGreen(uint8_t brightness) {
        _green.set(brightness);
    }

    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE>
    inline void RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>::setBlue(uint8_t brightness) {
        _blue.set(brightness);
    }

    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE>
    inline uint8_t RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>::red() const {
        return _red.brightness();
    }

    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE>
    inline uint8_t RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>::green() const {
        return _green.brightness();
    }

    template <class RedPwm, class GreenPwm, class BluePwm, bool COMMON_ANODE>
    inline uint8_t RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>::blue() const {
        return _blue.brightness();
    }
}

#endif
