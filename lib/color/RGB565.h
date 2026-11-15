#ifndef COLOR_RGB565_H
#define COLOR_RGB565_H

#include <stdint.h>

namespace color {
    // 16-bit color: RRRRR GGGGGG BBBBB
    namespace RGB565 {
        constexpr uint16_t BLACK   = 0x0000;
        constexpr uint16_t WHITE   = 0xFFFF;
        constexpr uint16_t RED     = 0xF800;
        constexpr uint16_t GREEN   = 0x07E0;
        constexpr uint16_t BLUE    = 0x001F;
        constexpr uint16_t YELLOW  = 0xFFE0;
        constexpr uint16_t CYAN    = 0x07FF;
        constexpr uint16_t MAGENTA = 0xF81F;
        constexpr uint16_t ORANGE  = 0xFD20;
        constexpr uint16_t GRAY    = 0x8410;

        // from 8-bit channels (0 ~ 255)
        inline constexpr uint16_t color(uint8_t r, uint8_t g, uint8_t b) {
            return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
        }

        // to 8-bit channels (0 ~ 255)
        inline constexpr uint8_t red(uint16_t c) {
            return (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
        }

        inline constexpr uint8_t green(uint16_t c) {
            return (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
        }

        inline constexpr uint8_t blue(uint16_t c) {
            return (uint8_t)((c & 0x1F) * 255 / 31);
        }
    }
}

#endif
