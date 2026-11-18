#ifndef COLOR_BW_H
#define COLOR_BW_H

#include <stdint.h>

namespace color {
    // black / white, 1 bit (SSD1306 OLED: 0 = dot off, 1 = on)
    // a color is white when its brightness (0.30 R + 0.59 G + 0.11 B, 0 ~ 255) is at least THRESHOLD,
    // so every ColorFormat function works here too: BW<>::RED is black (brightness 77), YELLOW white
    template <uint8_t THRESHOLD = 128>
    struct BW {
        static constexpr uint8_t BITS = 1;
        using Value = uint8_t;

        static inline constexpr Value color(uint8_t r, uint8_t g, uint8_t b);
        static inline constexpr uint8_t red(Value c);
        static inline constexpr uint8_t green(Value c);
        static inline constexpr uint8_t blue(Value c);
        static inline constexpr Value fromRGB888(uint32_t rgb);
        static inline constexpr uint32_t toRGB888(Value c);
        static inline constexpr Value fromRGB565(uint16_t c);

        static constexpr Value OFF = 0;
        static constexpr Value ON = 1;
        static constexpr Value BLACK   = OFF;
        static constexpr Value WHITE   = ON;
        static constexpr Value RED     = color(255, 0, 0);
        static constexpr Value GREEN   = color(0, 255, 0);
        static constexpr Value BLUE    = color(0, 0, 255);
        static constexpr Value YELLOW  = color(255, 255, 0);
        static constexpr Value CYAN    = color(0, 255, 255);
        static constexpr Value MAGENTA = color(255, 0, 255);
        static constexpr Value ORANGE  = color(255, 165, 0);
        static constexpr Value GRAY    = color(128, 128, 128);
    };

    // ================= definitions =================

    template <uint8_t THRESHOLD>
    inline constexpr typename BW<THRESHOLD>::Value BW<THRESHOLD>::color(uint8_t r, uint8_t g, uint8_t b) {
        // weights x 256: 77 + 151 + 28 = 256 (32 bits: AVR int is 16)
        return (uint32_t)77 * r + (uint32_t)151 * g + (uint32_t)28 * b >= (uint32_t)THRESHOLD * 256 ? ON : OFF;
    }

    template <uint8_t THRESHOLD>
    inline constexpr uint8_t BW<THRESHOLD>::red(Value c) {
        return c ? 255 : 0;
    }

    template <uint8_t THRESHOLD>
    inline constexpr uint8_t BW<THRESHOLD>::green(Value c) {
        return c ? 255 : 0;
    }

    template <uint8_t THRESHOLD>
    inline constexpr uint8_t BW<THRESHOLD>::blue(Value c) {
        return c ? 255 : 0;
    }

    template <uint8_t THRESHOLD>
    inline constexpr typename BW<THRESHOLD>::Value BW<THRESHOLD>::fromRGB888(uint32_t rgb) {
        return color((uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
    }

    template <uint8_t THRESHOLD>
    inline constexpr uint32_t BW<THRESHOLD>::toRGB888(Value c) {
        return c ? 0xFFFFFFUL : 0;
    }

    template <uint8_t THRESHOLD>
    inline constexpr typename BW<THRESHOLD>::Value BW<THRESHOLD>::fromRGB565(uint16_t c) {
        return color((uint8_t)(((c >> 11) & 0x1F) * 255 / 31), (uint8_t)(((c >> 5) & 0x3F) * 255 / 63),
                     (uint8_t)((c & 0x1F) * 255 / 31));
    }
}

#endif
