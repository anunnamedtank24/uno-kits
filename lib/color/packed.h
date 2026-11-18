#ifndef COLOR_PACKED_H
#define COLOR_PACKED_H

#include <stdint.h>

namespace color {
    // the order of the three channels in a packed value, first = highest bits
    enum Order : uint8_t { RGB, RBG, GRB, GBR, BRG, BGR };

    // the smallest unsigned type that holds BITS bits
    template <uint8_t BITS>
    struct ValueOf {
        using Type = typename ValueOf<(BITS <= 8 ? 8 : BITS <= 16 ? 16 : 32)>::Type;
    };
    template <> struct ValueOf<8> { using Type = uint8_t; };
    template <> struct ValueOf<16> { using Type = uint16_t; };
    template <> struct ValueOf<32> { using Type = uint32_t; };

    // a color format: three channels of R_BITS, G_BITS, B_BITS bits (1 ~ 8) packed into one value, in ORDER
    //   color::RGB565 = Packed<5, 6, 5, RGB>   RRRRR GGGGGG BBBBB
    //   color::BGR565 = Packed<5, 6, 5, BGR>   BBBBB GGGGGG RRRRR
    //   color::RGB888 = Packed<8, 8, 8, RGB>   0x00RRGGBB (uint32_t)
    //
    // a ColorFormat (what a Screen declares, and what graphics, font, image, video draw with):
    //   Value, BITS, the constants (BLACK ... GRAY), color(r, g, b), red / green / blue(c) (0 ~ 255),
    //   fromRGB565(c) (the colors of convert.py's files), fromRGB888(rgb), toRGB888(c),
    //   and for screens that send channels one by one: R_BITS / G_BITS / B_BITS, ORDER, redField / greenField / blueField
    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_ = RGB>
    struct Packed {
        static_assert(R >= 1 && R <= 8 && G >= 1 && G <= 8 && B >= 1 && B <= 8,
                      "color::Packed: every channel has 1 ~ 8 bits");

        static constexpr uint8_t R_BITS = R;
        static constexpr uint8_t G_BITS = G;
        static constexpr uint8_t B_BITS = B;
        static constexpr uint8_t BITS = R + G + B;
        static constexpr Order ORDER = ORDER_;
        using Value = typename ValueOf<BITS>::Type;

        // from 8-bit channels (0 ~ 255): the low bits are dropped
        static inline constexpr Value color(uint8_t r, uint8_t g, uint8_t b);

        // to 8-bit channels (0 ~ 255)
        static inline constexpr uint8_t red(Value c);
        static inline constexpr uint8_t green(Value c);
        static inline constexpr uint8_t blue(Value c);

        // the raw bits of a channel (0 ~ 2^bits - 1)
        static inline constexpr uint8_t redField(Value c);
        static inline constexpr uint8_t greenField(Value c);
        static inline constexpr uint8_t blueField(Value c);

        // 0xRRGGBB <-> this format
        static inline constexpr Value fromRGB888(uint32_t rgb);
        static inline constexpr uint32_t toRGB888(Value c);

        // a color of a file of convert.py (always RGB565) -> this format
        static inline constexpr Value fromRGB565(uint16_t c);

        static constexpr Value BLACK   = color(0, 0, 0);
        static constexpr Value WHITE   = color(255, 255, 255);
        static constexpr Value RED     = color(255, 0, 0);
        static constexpr Value GREEN   = color(0, 255, 0);
        static constexpr Value BLUE    = color(0, 0, 255);
        static constexpr Value YELLOW  = color(255, 255, 0);
        static constexpr Value CYAN    = color(0, 255, 255);
        static constexpr Value MAGENTA = color(255, 0, 255);
        static constexpr Value ORANGE  = color(255, 165, 0);
        static constexpr Value GRAY    = color(128, 128, 128);

    private:
        // where each channel starts (bit 0 = lowest): the first of ORDER is in the highest bits
        static constexpr uint8_t FIRST = ORDER == RGB || ORDER == RBG ? 0 : ORDER == GRB || ORDER == GBR ? 1 : 2;
        static constexpr uint8_t SECOND = ORDER == GRB || ORDER == BRG ? 0 : ORDER == RGB || ORDER == BGR ? 1 : 2;
        static constexpr uint8_t bitsOf(uint8_t channel) { return channel == 0 ? R : channel == 1 ? G : B; }
        static constexpr uint8_t shiftOf(uint8_t channel) {
            // third: 0; second: bits of the third; first: bits of the second and the third
            return channel == FIRST ? (uint8_t)(BITS - bitsOf(FIRST))
                 : channel == SECOND ? (uint8_t)(BITS - bitsOf(FIRST) - bitsOf(SECOND)) : 0;
        }
        static constexpr uint8_t R_SHIFT = shiftOf(0);
        static constexpr uint8_t G_SHIFT = shiftOf(1);
        static constexpr uint8_t B_SHIFT = shiftOf(2);

        static inline constexpr uint8_t expand(uint8_t field, uint8_t bits);
    };

    using RGB565 = Packed<5, 6, 5, RGB>;    // ST7789 16 bit, and the files of convert.py
    using BGR565 = Packed<5, 6, 5, BGR>;    // ST7789 16 bit, red and blue swapped (MADCTL BGR)
    using RGB666 = Packed<6, 6, 6, RGB>;    // ST7789 18 bit
    using RGB888 = Packed<8, 8, 8, RGB>;    // ST7789 18 bit (the low 2 bits of each channel are ignored), PC colors
    using BGR888 = Packed<8, 8, 8, BGR>;
    using GBR888 = Packed<8, 8, 8, GBR>;
    using RGB444 = Packed<4, 4, 4, RGB>;    // ST7789 12 bit
    using RGB332 = Packed<3, 3, 2, RGB>;    // 1 byte a color (palettes, LED matrices); no ST7789 mode

    // one format to another, through 8-bit channels: convert<color::RGB332>(color::RGB565::ORANGE)
    template <class To, class From>
    inline constexpr typename To::Value convert(typename From::Value c) {
        return To::color(From::red(c), From::green(c), From::blue(c));
    }

    // ================= definitions =================

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr typename Packed<R, G, B, ORDER_>::Value Packed<R, G, B, ORDER_>::color(uint8_t r, uint8_t g, uint8_t b) {
        return (Value)(((uint32_t)(r >> (8 - R)) << R_SHIFT) | ((uint32_t)(g >> (8 - G)) << G_SHIFT)
                       | ((uint32_t)(b >> (8 - B)) << B_SHIFT));
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr uint8_t Packed<R, G, B, ORDER_>::redField(Value c) {
        return (uint8_t)(((uint32_t)c >> R_SHIFT) & ((1u << R) - 1));
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr uint8_t Packed<R, G, B, ORDER_>::greenField(Value c) {
        return (uint8_t)(((uint32_t)c >> G_SHIFT) & ((1u << G) - 1));
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr uint8_t Packed<R, G, B, ORDER_>::blueField(Value c) {
        return (uint8_t)(((uint32_t)c >> B_SHIFT) & ((1u << B) - 1));
    }

    // n bits -> 8 bits, the full range: 31 -> 255, 0 -> 0
    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr uint8_t Packed<R, G, B, ORDER_>::expand(uint8_t field, uint8_t bits) {
        return bits == 8 ? field : (uint8_t)((uint16_t)field * 255 / ((1u << bits) - 1));
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr uint8_t Packed<R, G, B, ORDER_>::red(Value c) {
        return expand(redField(c), R);
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr uint8_t Packed<R, G, B, ORDER_>::green(Value c) {
        return expand(greenField(c), G);
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr uint8_t Packed<R, G, B, ORDER_>::blue(Value c) {
        return expand(blueField(c), B);
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr typename Packed<R, G, B, ORDER_>::Value Packed<R, G, B, ORDER_>::fromRGB888(uint32_t rgb) {
        return color((uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr uint32_t Packed<R, G, B, ORDER_>::toRGB888(Value c) {
        return ((uint32_t)red(c) << 16) | ((uint32_t)green(c) << 8) | blue(c);
    }

    template <uint8_t R, uint8_t G, uint8_t B, Order ORDER_>
    inline constexpr typename Packed<R, G, B, ORDER_>::Value Packed<R, G, B, ORDER_>::fromRGB565(uint16_t c) {
        // RGB565 itself: as it is (nothing to compute)
        return (R == 5 && G == 6 && B == 5 && ORDER_ == RGB) ? (Value)c
             : color((uint8_t)(((c >> 11) & 0x1F) * 255 / 31), (uint8_t)(((c >> 5) & 0x3F) * 255 / 63),
                     (uint8_t)((c & 0x1F) * 255 / 31));
    }
}

#endif
