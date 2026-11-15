#ifndef LED_BRIGHTNESS_H
#define LED_BRIGHTNESS_H

#include <stdint.h>

namespace LED {
    // the eye sees brightness about as the square root of the light (gamma ~2):
    // duty 128 looks ~70 % bright, not 50 %; this turns "how bright it looks" into a duty
    //   brightness 0 ~ 255 -> duty = brightness^2 / 255 (0 -> 0, 128 -> 64, 255 -> 255)
    inline constexpr uint8_t toDuty(uint8_t brightness);

    // level 0 ~ levels -> brightness 0 ~ 255 (rounded), e.g. 11 steps for levels = 10; levels = 0 gives 0
    inline constexpr uint8_t levelToBrightness(uint8_t level, uint8_t levels);

    // ================= definitions =================

    inline constexpr uint8_t toDuty(uint8_t brightness) {
        return (uint8_t)(((uint16_t)brightness * brightness + 127) / 255);
    }

    inline constexpr uint8_t levelToBrightness(uint8_t level, uint8_t levels) {
        return levels == 0 ? 0
             : level >= levels ? 255
             : (uint8_t)(((uint16_t)level * 255 + levels / 2) / levels);
    }
}

#endif
