#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <stdint.h>
#include <stdlib.h>

namespace video {
    // frame format (format 3, see lib/video/output/README.md), made by lib/video/source/convert.py, little endian:
    //   uint16_t delay (ms), uint8_t type (TYPE_P / TYPE_I), uint8_t window count
    //   every window: x, y, w, h (uint8_t, animation pixels before scale), then runs until w * h pixels
    //   run, by color mode:
    //     COLORS_16   1 byte: high nibble n, low nibble color index
    //                 n = 1 ~ 15: n pixels, n = 0: (next byte + 16) pixels
    //     COLORS_256  2 bytes: n, color index; n + 1 pixels (1 ~ 256)
    //     COLORS_565  3 bytes: n, RGB565 low byte, RGB565 high byte; n + 1 pixels (1 ~ 256)
    //   pixels go row by row inside the window, runs never cross a window
    // an I frame has one window covering the whole animation, a P frame only what changed
    // (the screen keeps the previous frame, nothing is kept on the board)
    constexpr uint8_t TYPE_P = 0;
    constexpr uint8_t TYPE_I = 1;

    constexpr uint8_t COLORS_16 = 0;
    constexpr uint8_t COLORS_256 = 1;
    constexpr uint8_t COLORS_565 = 2;
    constexpr uint8_t MAX_RAM_COLORS = 16;

    // who an animation was made for (Animation::target, bit flags), see lib/video/source/config.json
    constexpr uint8_t TARGET_FLASH = 1; // FlashPlayer: the frames are in the struct (flash)
    constexpr uint8_t TARGET_STREAM = 2; // StreamPlayer: the struct has the path, stream.py sends the frames

    // compile-time warnings: static_assert(video::warn::Xxx<condition>::ok, "")
    namespace warn {
        template <bool> struct NotForStream { static constexpr bool ok = true; };
        template <> struct NotForStream<true> {
            __attribute__((deprecated("this animation was made with --target flash only: it streams, "
                                      "but was not checked against the stream ranges (make it with --target flash stream)")))
            static constexpr bool ok = true;
        };
        template <bool> struct PaletteInRam { static constexpr bool ok = true; };
        template <> struct PaletteInRam<true> {
            __attribute__((deprecated("a 256-color animation streamed: the whole palette comes with the header (more "
                                      "waiting before the first frame) and is kept in RAM taken at run time (malloc, up to "
                                      "256 colors x the color's size; not in the compiler's RAM figure) until close() / the "
                                      "next open(); error() is ERROR_NO_MEMORY if it does not fit")))
            static constexpr bool ok = true;
        };
    }

    // a palette in RAM taken at run time (malloc) and given back when it goes out of scope (or release()):
    // a streamed COLORS_256 palette, whose size is only known when it arrives
    template <class Color>
    class HeapPalette {
    public:
        inline HeapPalette() = default;
        HeapPalette(const HeapPalette &) = delete;
        HeapPalette &operator=(const HeapPalette &) = delete;
        inline ~HeapPalette();

        // count colors (1 ~ 256), the old ones given back first; false (and nothing kept) if the RAM is short
        // (avr-libc's malloc keeps __malloc_margin, 128 bytes, free for the stack)
        inline bool take(uint16_t count);
        inline void release();
        inline Color *colors() const;

    private:
        Color *_colors = nullptr;
    };

    // pixels of one row kept when scaled (scaleX or scaleY >= 2)
    constexpr uint8_t MAX_SCALED_WIDTH = 120;

    // draws frames on a Screen at (x, y), every pixel as scaleX x scaleY
    //
    // Screen: provides (see lib/ST7789/basic/window.h)
    //   ColorFormat: the screen's colors (lib/color): Value, fromRGB565(c) (the colors of the file are always
    //     RGB565: turned once per palette color, and per run for COLORS_565 / a flash COLORS_256 palette)
    //   width(), height(), setWindow(x0, y0, x1, y1), writePixel(ColorFormat::Value), endWrite()
    //   BusType::readFlash16 (COLORS_256 from flash: the palette stays in flash)
    // Reader: anything with uint8_t read8() giving the next byte of the frame
    template <class Screen>
    class Decoder {
    public:
        using ColorFormat = typename Screen::ColorFormat;
        using Color = typename ColorFormat::Value;

        inline explicit Decoder(Screen &screen);

        // false (and nothing is drawn later) if it does not fit the screen
        inline bool setArea(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t scaleX, uint8_t scaleY);

        // COLORS_16: colors set one by one into RAM
        // COLORS_256: palette in flash (RGB565, PROGMEM), or already in the screen's colors in RAM
        //   (setRamPalette, e.g. a HeapPalette: it must live while frames are drawn)
        // COLORS_565: no palette
        inline void setColorMode(uint8_t colorMode);
        inline void setColor(uint8_t index, uint16_t color);
        inline void setFlashPalette(const uint16_t *palette);
        inline void setRamPalette(const Color *palette);

        // false on broken data (a window outside the animation), the rest of the frame is not read
        template <class Reader>
        inline bool drawFrame(Reader &reader);

        inline uint16_t lastDelay() const;
        inline uint8_t lastType() const;

    private:
        using Bus = typename Screen::BusType;

        Screen &_screen;
        int16_t _x = 0;
        int16_t _y = 0;
        uint8_t _width = 0;
        uint8_t _height = 0;
        uint8_t _scaleX = 1;
        uint8_t _scaleY = 1;
        bool _fits = false;
        uint8_t _colorMode = COLORS_16;
        Color _palette[MAX_RAM_COLORS] = {};
        const uint16_t *_flashPalette = nullptr;
        const Color *_ramPalette = nullptr;
        Color _row[MAX_SCALED_WIDTH] = {};
        uint16_t _lastDelay = 0;
        uint8_t _lastType = TYPE_P;

        // the run being drawn, may go on into the next row
        uint16_t _runLeft = 0;
        Color _runColor = 0;

        template <class Reader>
        inline void nextRun(Reader &reader);

        template <class Reader>
        inline void drawWindow(Reader &reader, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
        template <class Reader>
        inline void drawWindowScaled(Reader &reader, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    };

    // ================= definitions =================

    template <class Screen>
    inline Decoder<Screen>::Decoder(Screen &screen) : _screen(screen) {
    }

    template <class Screen>
    inline bool Decoder<Screen>::setArea(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t scaleX, uint8_t scaleY) {
        _x = x;
        _y = y;
        _width = width;
        _height = height;
        _scaleX = scaleX;
        _scaleY = scaleY;
        // frames are streamed into one window (not through fillWindow): not clipped, so the whole animation
        // must be on the screen; a scaled row also has to fit the row buffer
        int32_t right = (int32_t)x + (int32_t)width * scaleX;
        int32_t bottom = (int32_t)y + (int32_t)height * scaleY;
        bool isScaled = scaleX != 1 || scaleY != 1;
        _fits = scaleX >= 1 && scaleY >= 1 && x >= 0 && y >= 0
             && right <= (int32_t)_screen.width() && bottom <= (int32_t)_screen.height()
             && (!isScaled || width <= MAX_SCALED_WIDTH);
        return _fits;
    }

    template <class Screen>
    inline void Decoder<Screen>::setColorMode(uint8_t colorMode) {
        _colorMode = colorMode;
    }

    template <class Screen>
    inline void Decoder<Screen>::setRamPalette(const Color *palette) {
        _ramPalette = palette;
        _flashPalette = nullptr;
    }

    template <class Screen>
    inline void Decoder<Screen>::setColor(uint8_t index, uint16_t color) {
        if (index < MAX_RAM_COLORS) {
            _palette[index] = ColorFormat::fromRGB565(color);
        }
    }

    template <class Screen>
    inline void Decoder<Screen>::setFlashPalette(const uint16_t *palette) {
        _flashPalette = palette;
        _ramPalette = nullptr;
    }

    template <class Screen>
    template <class Reader>
    inline bool Decoder<Screen>::drawFrame(Reader &reader) {
        _lastDelay = reader.read8();
        _lastDelay |= (uint16_t)reader.read8() << 8;
        _lastType = reader.read8();
        uint8_t windowCount = reader.read8();

        bool isScaled = _scaleX != 1 || _scaleY != 1;
        for (uint8_t i = 0; i < windowCount; i++) {
            uint8_t x = reader.read8();
            uint8_t y = reader.read8();
            uint8_t w = reader.read8();
            uint8_t h = reader.read8();
            if (w == 0 || h == 0 || (uint16_t)x + w > _width || (uint16_t)y + h > _height) {
                return false;
            }
            if (isScaled && w > MAX_SCALED_WIDTH) {
                return false;
            }
            if (isScaled) {
                drawWindowScaled(reader, x, y, w, h);
            } else {
                drawWindow(reader, x, y, w, h);
            }
        }
        return true;
    }

    template <class Screen>
    inline uint16_t Decoder<Screen>::lastDelay() const {
        return _lastDelay;
    }

    template <class Screen>
    inline uint8_t Decoder<Screen>::lastType() const {
        return _lastType;
    }

    template <class Screen>
    template <class Reader>
    inline void Decoder<Screen>::nextRun(Reader &reader) {
        if (_colorMode == COLORS_16) {
            uint8_t run = reader.read8();
            _runColor = _palette[run & 0x0F];
            _runLeft = run >> 4;
            if (_runLeft == 0) {
                _runLeft = (uint16_t)reader.read8() + 16;
            }
        } else if (_colorMode == COLORS_256) {
            _runLeft = (uint16_t)reader.read8() + 1;
            uint8_t index = reader.read8();
            _runColor = _ramPalette ? _ramPalette[index]
                      : _flashPalette ? ColorFormat::fromRGB565(Bus::readFlash16(_flashPalette + index)) : 0;
        } else {
            _runLeft = (uint16_t)reader.read8() + 1;
            uint16_t color = reader.read8();
            _runColor = ColorFormat::fromRGB565((uint16_t)(color | ((uint16_t)reader.read8() << 8)));
        }
    }

    // not scaled: runs go straight to the screen
    template <class Screen>
    template <class Reader>
    inline void Decoder<Screen>::drawWindow(Reader &reader, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
        if (_fits) {
            _screen.setWindow(_x + x, _y + y, _x + x + w - 1, _y + y + h - 1);
        }
        uint16_t pixelsLeft = (uint16_t)w * h;
        while (pixelsLeft > 0) {
            nextRun(reader);
            // broken data must not write outside the window
            if (_runLeft > pixelsLeft) {
                _runLeft = pixelsLeft;
            }
            pixelsLeft -= _runLeft;
            if (!_fits) {
                continue;
            }
            for (uint16_t i = 0; i < _runLeft; i++) {
                _screen.writePixel(_runColor);
            }
        }
        _runLeft = 0;
        if (_fits) {
            _screen.endWrite();
        }
    }

    // scaled: one row of colors is kept, every pixel is sent scaleX times and every row scaleY times
    template <class Screen>
    template <class Reader>
    inline void Decoder<Screen>::drawWindowScaled(Reader &reader, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
        if (_fits) {
            int16_t left = _x + (int16_t)x * _scaleX;
            int16_t top = _y + (int16_t)y * _scaleY;
            _screen.setWindow(left, top, left + (int16_t)w * _scaleX - 1, top + (int16_t)h * _scaleY - 1);
        }
        _runLeft = 0;
        for (uint8_t row = 0; row < h; row++) {
            for (uint8_t col = 0; col < w; col++) {
                if (_runLeft == 0) {
                    nextRun(reader);
                }
                _row[col] = _runColor;
                _runLeft--;
            }
            if (!_fits) {
                continue;
            }
            for (uint8_t repeat = 0; repeat < _scaleY; repeat++) {
                for (uint8_t col = 0; col < w; col++) {
                    for (uint8_t i = 0; i < _scaleX; i++) {
                        _screen.writePixel(_row[col]);
                    }
                }
            }
        }
        _runLeft = 0;
        if (_fits) {
            _screen.endWrite();
        }
    }

    template <class Color>
    inline HeapPalette<Color>::~HeapPalette() {
        release();
    }

    template <class Color>
    inline bool HeapPalette<Color>::take(uint16_t count) {
        release();
        _colors = (Color *)malloc((size_t)count * sizeof(Color));
        return _colors != nullptr;
    }

    template <class Color>
    inline void HeapPalette<Color>::release() {
        free(_colors);
        _colors = nullptr;
    }

    template <class Color>
    inline Color *HeapPalette<Color>::colors() const {
        return _colors;
    }
}

#endif
