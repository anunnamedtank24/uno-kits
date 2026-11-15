#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <stdint.h>

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
    constexpr uint8_t TARGET_UNO = 1; // FlashPlayer: the frames are in the struct (flash)
    constexpr uint8_t TARGET_MAC = 2; // StreamPlayer: the struct has the path, stream.py sends the frames

    // compile-time warnings: static_assert(video::warn::Xxx<condition>::ok, "")
    namespace warn {
        template <bool> struct NotForMac { static constexpr bool ok = true; };
        template <> struct NotForMac<true> {
            __attribute__((deprecated("this animation was made with --target uno only: it streams, "
                                      "but was not checked against the mac ranges (make it with --target uno mac)")))
            static constexpr bool ok = true;
        };
    }

    // pixels of one row kept when scaled (scaleX or scaleY >= 2)
    constexpr uint8_t MAX_SCALED_WIDTH = 120;

    // draws frames on a Screen at (x, y), every pixel as scaleX x scaleY
    //
    // Screen: provides (see lib/ST7789/basic/window.h)
    //   width(), height(), setWindow(x0, y0, x1, y1), writeData16(color), endWrite()
    //   BusType::readFlash16 (COLORS_256: the palette stays in flash)
    //
    // Reader: anything with uint8_t read8() giving the next byte of the frame
    template <class Screen>
    class Decoder {
    public:
        inline explicit Decoder(Screen &screen);

        // false (and nothing is drawn later) if it does not fit the screen
        inline bool setArea(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t scaleX, uint8_t scaleY);

        // COLORS_16: colors set one by one into RAM
        // COLORS_256: palette in flash (PROGMEM), 256 colors at most
        // COLORS_565: no palette
        inline void setColorMode(uint8_t colorMode);
        inline void setColor(uint8_t index, uint16_t color);
        inline void setFlashPalette(const uint16_t *palette);

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
        uint16_t _palette[MAX_RAM_COLORS] = {};
        const uint16_t *_flashPalette = nullptr;
        uint16_t _row[MAX_SCALED_WIDTH] = {};
        uint16_t _lastDelay = 0;
        uint8_t _lastType = TYPE_P;

        // the run being drawn, may go on into the next row
        uint16_t _runLeft = 0;
        uint16_t _runColor = 0;

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
    inline void Decoder<Screen>::setColor(uint8_t index, uint16_t color) {
        if (index < MAX_RAM_COLORS) {
            _palette[index] = color;
        }
    }

    template <class Screen>
    inline void Decoder<Screen>::setFlashPalette(const uint16_t *palette) {
        _flashPalette = palette;
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
            _runColor = _flashPalette ? Bus::readFlash16(_flashPalette + index) : 0;
        } else {
            _runLeft = (uint16_t)reader.read8() + 1;
            _runColor = reader.read8();
            _runColor |= (uint16_t)reader.read8() << 8;
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
                _screen.writeData16(_runColor);
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
                        _screen.writeData16(_row[col]);
                    }
                }
            }
        }
        _runLeft = 0;
        if (_fits) {
            _screen.endWrite();
        }
    }
}

#endif
