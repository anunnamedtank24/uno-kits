#ifndef IMAGE_DECODER_H
#define IMAGE_DECODER_H

#include <stdint.h>

namespace image {
    // pixel data (format 1, see lib/image/output/README.md), made by lib/image/source/convert.py:
    // width x height pixels, row by row, every pixel drawn as scaleX x scaleY on the screen
    //   ENCODING_RAW: every pixel as it is, the bits run on across rows
    //     COLORS_2    1 bit, first pixel in bit 7         COLORS_16   4 bits, first pixel in the high nibble
    //     COLORS_256  1 byte (palette index)               COLORS_565  2 bytes, little endian
    //   ENCODING_RUNS: runs of one color, a run may go on into the next row
    //     COLORS_2    1 byte: bit 7 color index, bits 0 ~ 6 = n - 1 (1 ~ 128 pixels)
    //     COLORS_16   1 byte: high nibble n, low nibble color index; n = 0: (next byte + 16) pixels
    //     COLORS_256  2 bytes: n - 1, color index (1 ~ 256 pixels)
    //     COLORS_565  3 bytes: n - 1, RGB565 low byte, high byte (1 ~ 256 pixels)
    constexpr uint8_t COLORS_2 = 0;
    constexpr uint8_t COLORS_16 = 1;
    constexpr uint8_t COLORS_256 = 2;
    constexpr uint8_t COLORS_565 = 3;
    constexpr uint8_t MAX_RAM_COLORS = 16;

    constexpr uint8_t ENCODING_RAW = 0;
    constexpr uint8_t ENCODING_RUNS = 1;

    // who a picture was made for (Picture::target, bit flags), see lib/image/source/config.json
    constexpr uint8_t TARGET_UNO = 1; // FlashImage: the pixels are in the struct (flash)
    constexpr uint8_t TARGET_MAC = 2; // StreamImage: the struct has the path, stream.py sends the pixels

    // compile-time warnings: static_assert(image::warn::Xxx<condition>::ok, "")
    namespace warn {
        template <bool> struct NotForMac { static constexpr bool ok = true; };
        template <> struct NotForMac<true> {
            __attribute__((deprecated("this picture was made with --target uno only: it streams, "
                                      "but was not checked against the mac ranges (make it with --target uno mac)")))
            static constexpr bool ok = true;
        };
    }

    // draws a picture on a Screen at (x, y), every stored pixel as scaleX x scaleY
    // a row is cut into pieces of SEGMENT screen pixels, each sent with writeWindow():
    // clipped (any x, y works, the part off the screen is skipped), and any Screen with
    // writeWindow works: ST7789 (lib/ST7789/basic/window.h) or the RAM picture of an OLED
    // (lib/SSD1306/basic/frame.h, then display())
    //
    // Screen: provides writeWindow(x, y, w, h, colors) clipped, BusType::readFlash16 (COLORS_256)
    // Reader: anything with uint8_t read8() giving the next byte of the pixel data
    template <class Screen, uint8_t SEGMENT = 32>
    class Decoder {
    public:
        static_assert(SEGMENT >= 1, "image::Decoder: SEGMENT must be 1 ~ 255");

        inline explicit Decoder(Screen &screen);

        inline void setArea(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t scaleX, uint8_t scaleY);

        // COLORS_2 / COLORS_16: colors set one by one into RAM
        // COLORS_256: palette in flash (PROGMEM)
        // COLORS_565: no palette
        inline void setFormat(uint8_t colorMode, uint8_t encoding);
        inline void setColor(uint8_t index, uint16_t color);
        inline void setFlashPalette(const uint16_t *palette);

        // reads all width * height pixels and draws them
        template <class Reader>
        inline void draw(Reader &reader);

    private:
        using Bus = typename Screen::BusType;

        Screen &_screen;
        int16_t _x = 0;
        int16_t _y = 0;
        uint8_t _width = 0;
        uint8_t _height = 0;
        uint8_t _scaleX = 1;
        uint8_t _scaleY = 1;
        uint8_t _colorMode = COLORS_565;
        uint8_t _encoding = ENCODING_RAW;
        uint16_t _palette[MAX_RAM_COLORS] = {};
        const uint16_t *_flashPalette = nullptr;
        uint16_t _segment[SEGMENT] = {};

        // the run being read (ENCODING_RUNS), may go on into the next row
        uint16_t _runLeft = 0;
        uint16_t _runColor = 0;

        // the byte being read bit by bit (ENCODING_RAW, COLORS_2 / COLORS_16)
        uint8_t _bits = 0;
        uint8_t _bitsLeft = 0;

        inline uint16_t indexColor(uint8_t index) const;

        template <class Reader>
        inline uint16_t nextPixel(Reader &reader);

        template <class Reader>
        inline void nextRun(Reader &reader);

        // the segment: count screen pixels from screen x, every row of the stored row
        inline void flush(int16_t x, int16_t top, uint8_t count);
    };

    // ================= definitions =================

    template <class Screen, uint8_t SEGMENT>
    inline Decoder<Screen, SEGMENT>::Decoder(Screen &screen) : _screen(screen) {
    }

    template <class Screen, uint8_t SEGMENT>
    inline void Decoder<Screen, SEGMENT>::setArea(int16_t x, int16_t y, uint8_t width, uint8_t height,
                                                  uint8_t scaleX, uint8_t scaleY) {
        _x = x;
        _y = y;
        _width = width;
        _height = height;
        _scaleX = scaleX < 1 ? 1 : scaleX;
        _scaleY = scaleY < 1 ? 1 : scaleY;
    }

    template <class Screen, uint8_t SEGMENT>
    inline void Decoder<Screen, SEGMENT>::setFormat(uint8_t colorMode, uint8_t encoding) {
        _colorMode = colorMode;
        _encoding = encoding;
    }

    template <class Screen, uint8_t SEGMENT>
    inline void Decoder<Screen, SEGMENT>::setColor(uint8_t index, uint16_t color) {
        if (index < MAX_RAM_COLORS) {
            _palette[index] = color;
        }
    }

    template <class Screen, uint8_t SEGMENT>
    inline void Decoder<Screen, SEGMENT>::setFlashPalette(const uint16_t *palette) {
        _flashPalette = palette;
    }

    template <class Screen, uint8_t SEGMENT>
    template <class Reader>
    inline void Decoder<Screen, SEGMENT>::draw(Reader &reader) {
        _runLeft = 0;
        _bitsLeft = 0;
        for (uint8_t row = 0; row < _height; row++) {
            int16_t top = _y + (int16_t)row * _scaleY;
            int16_t segmentX = _x;
            uint8_t count = 0;
            for (uint8_t col = 0; col < _width; col++) {
                uint16_t color = nextPixel(reader);
                for (uint8_t i = 0; i < _scaleX; i++) {
                    _segment[count++] = color;
                    if (count == SEGMENT) {
                        flush(segmentX, top, count);
                        segmentX += count;
                        count = 0;
                    }
                }
            }
            if (count > 0) {
                flush(segmentX, top, count);
            }
            // raw 2 / 16 colors: the bits run on across rows, nothing to reset here
        }
    }

    template <class Screen, uint8_t SEGMENT>
    inline uint16_t Decoder<Screen, SEGMENT>::indexColor(uint8_t index) const {
        if (_colorMode == COLORS_256) {
            return _flashPalette ? Bus::readFlash16(_flashPalette + index) : 0;
        }
        return _palette[index & (MAX_RAM_COLORS - 1)];
    }

    template <class Screen, uint8_t SEGMENT>
    template <class Reader>
    inline uint16_t Decoder<Screen, SEGMENT>::nextPixel(Reader &reader) {
        if (_encoding == ENCODING_RUNS) {
            if (_runLeft == 0) {
                nextRun(reader);
            }
            _runLeft--;
            return _runColor;
        }
        if (_colorMode == COLORS_565) {
            uint16_t color = reader.read8();
            return color | ((uint16_t)reader.read8() << 8);
        }
        if (_colorMode == COLORS_256) {
            return indexColor(reader.read8());
        }
        // 1 or 4 bits from the byte being read, first pixel in the high bits
        uint8_t bitCount = _colorMode == COLORS_2 ? 1 : 4;
        if (_bitsLeft == 0) {
            _bits = reader.read8();
            _bitsLeft = 8;
        }
        _bitsLeft -= bitCount;
        uint8_t index = (uint8_t)(_bits >> _bitsLeft) & (uint8_t)((1 << bitCount) - 1);
        return indexColor(index);
    }

    template <class Screen, uint8_t SEGMENT>
    template <class Reader>
    inline void Decoder<Screen, SEGMENT>::nextRun(Reader &reader) {
        if (_colorMode == COLORS_2) {
            uint8_t run = reader.read8();
            _runColor = indexColor(run >> 7);
            _runLeft = (uint16_t)(run & 0x7F) + 1;
        } else if (_colorMode == COLORS_16) {
            uint8_t run = reader.read8();
            _runColor = indexColor(run & 0x0F);
            _runLeft = run >> 4;
            if (_runLeft == 0) {
                _runLeft = (uint16_t)reader.read8() + 16;
            }
        } else if (_colorMode == COLORS_256) {
            _runLeft = (uint16_t)reader.read8() + 1;
            _runColor = indexColor(reader.read8());
        } else {
            _runLeft = (uint16_t)reader.read8() + 1;
            _runColor = reader.read8();
            _runColor |= (uint16_t)reader.read8() << 8;
        }
    }

    template <class Screen, uint8_t SEGMENT>
    inline void Decoder<Screen, SEGMENT>::flush(int16_t x, int16_t top, uint8_t count) {
        for (uint8_t repeat = 0; repeat < _scaleY; repeat++) {
            _screen.writeWindow(x, top + repeat, count, 1, _segment);
        }
    }
}

#endif
