#ifndef ST7789_WINDOW_H
#define ST7789_WINDOW_H

#include "datasheet.h"

namespace ST7789 {
    // window = the area set by CASET + RASET, written with RAMWR
    // coordinates are screen coordinates (offset applied)
    // fillWindow / writeWindow are clipped to the screen: every shape and text is drawn through them,
    // so clipping here is enough for all of graphics (setWindow itself is not clipped)
    //
    // ColorFormat: how a pixel is stored and sent (lib/color, or any struct with the same members):
    //   16 bit (COLMOD 0x55): R5 G6 B5 in RGB or BGR order (color::RGB565, color::BGR565), 2 bytes a pixel, the fastest
    //   18 bit (COLMOD 0x66): 6 or 8 bits a channel (color::RGB666, color::RGB888 ...), 3 bytes a pixel
    //   12 bit (COLMOD 0x53): R4 G4 B4 (color::RGB444), 2 pixels in 3 bytes
    //   it needs Value, BITS, R_BITS, G_BITS, B_BITS, ORDER, redField / greenField / blueField
    //   anything else (color::RGB332, color::BW): a compile error
    template <class Bus, class ColorFormat_>
    class Window : public Datasheet<Bus> {
    public:
        using ColorFormat = ColorFormat_;
        using Color = typename ColorFormat::Value;

        static_assert(ColorFormat::BITS == 12 || ColorFormat::BITS == 16 || ColorFormat::BITS == 18
                      || ColorFormat::BITS == 24,
                      "ST7789::Window: the ColorFormat must be 12 bit (RGB444), 16 bit (RGB565 / BGR565) or "
                      "18 / 24 bit (RGB666 / RGB888 ...): the panel has no other mode");

        static_assert(ColorFormat::BITS != 16 || (ColorFormat::R_BITS == 5 && ColorFormat::G_BITS == 6
                                                   && (ColorFormat::ORDER == 0 || ColorFormat::ORDER == 5)),
                      "ST7789::Window: 16 bit is R5 G6 B5 in RGB or BGR order only (color::RGB565 / BGR565)");
        static_assert(ColorFormat::BITS != 12 || (ColorFormat::R_BITS == 4 && ColorFormat::G_BITS == 4),
                      "ST7789::Window: 12 bit is R4 G4 B4 only (color::RGB444)");

        // the COLMOD and the MADCTL color order bit (ORDER 5 = color::BGR) for this format (init() sends them)
        static constexpr uint8_t COLMOD_VALUE = ColorFormat::BITS == 12 ? COLMOD_12BIT
                                              : ColorFormat::BITS == 16 ? COLMOD_16BIT : COLMOD_18BIT;
        static constexpr uint8_t MADCTL_ORDER = ColorFormat::BITS == 16 && ColorFormat::ORDER == 5 ? 0x08 : 0x00;

        using Datasheet<Bus>::Datasheet;

        // window (inclusive) then RAMWR: stream pixels with writePixel(), then endWrite()
        inline void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

        // one pixel of the window set by setWindow
        inline void writePixel(Color color);

        // ends a run of writePixel() (12 bit: sends the half pixel left)
        inline void endWrite();

        // solid color, clipped to the screen; w <= 0 or h <= 0 draws nothing
        inline void fillWindow(int16_t x, int16_t y, int16_t w, int16_t h, Color color);

        // color list (w * h colors, row by row), clipped to the screen: the colors of cut parts are skipped
        inline void writeWindow(int16_t x, int16_t y, int16_t w, int16_t h, const Color *colors);

    private:
        // 12 bit: the blue of a pixel waits for the red of the next one
        uint8_t _pending = 0;
        bool _hasPending = false;

        // the part of a rectangle on the screen, false if nothing is left
        inline bool clip(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const;
    };

    // ================= definitions =================

    template <class Bus, class ColorFormat_>
    inline void Window<Bus, ColorFormat_>::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
        this->caset(x0 + this->xOffset(), x1 + this->xOffset());
        this->raset(y0 + this->yOffset(), y1 + this->yOffset());
        this->ramWr();
        _hasPending = false;
    }

    template <class Bus, class ColorFormat_>
    inline void Window<Bus, ColorFormat_>::writePixel(Color color) {
        if (ColorFormat::BITS == 16) {
            // R5 G6 B5 as it is (BGR: MADCTL swaps on the panel)
            this->writeData16((uint16_t)color);
        } else if (ColorFormat::BITS == 12) {
            uint8_t r = ColorFormat::redField(color);
            uint8_t g = ColorFormat::greenField(color);
            uint8_t b = ColorFormat::blueField(color);
            if (!_hasPending) {
                this->writeData8((uint8_t)((r << 4) | g));
                _pending = b;
                _hasPending = true;
            } else {
                this->writeData8((uint8_t)((_pending << 4) | r));
                this->writeData8((uint8_t)((g << 4) | b));
                _hasPending = false;
            }
        } else {
            // 18 bit: a byte a channel, the top 6 bits count
            this->writeData8((uint8_t)(ColorFormat::redField(color) << (8 - ColorFormat::R_BITS)));
            this->writeData8((uint8_t)(ColorFormat::greenField(color) << (8 - ColorFormat::G_BITS)));
            this->writeData8((uint8_t)(ColorFormat::blueField(color) << (8 - ColorFormat::B_BITS)));
        }
    }

    template <class Bus, class ColorFormat_>
    inline void Window<Bus, ColorFormat_>::endWrite() {
        if (ColorFormat::BITS == 12 && _hasPending) {
            // the half pixel is dropped by the panel when the next command comes
            this->writeData8((uint8_t)(_pending << 4));
            _hasPending = false;
        }
        Datasheet<Bus>::endWrite();
    }

    template <class Bus, class ColorFormat_>
    inline void Window<Bus, ColorFormat_>::fillWindow(int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
        if (!clip(x, y, w, h)) {
            return;
        }
        setWindow(x, y, x + w - 1, y + h - 1);
        uint32_t n = (uint32_t)w * h;
        while (n--) {
            writePixel(color);
        }
        endWrite();
    }

    template <class Bus, class ColorFormat_>
    inline void Window<Bus, ColorFormat_>::writeWindow(int16_t x, int16_t y, int16_t w, int16_t h, const Color *colors) {
        // visible part
        int16_t visibleX = x;
        int16_t visibleY = y;
        int16_t visibleW = w;
        int16_t visibleH = h;
        if (!clip(visibleX, visibleY, visibleW, visibleH)) {
            return;
        }

        setWindow(visibleX, visibleY, visibleX + visibleW - 1, visibleY + visibleH - 1);
        for (int16_t row = 0; row < visibleH; row++) {
            // the visible columns of this row in the color list
            const Color *line = colors + (uint32_t)(visibleY - y + row) * w + (visibleX - x);
            for (int16_t col = 0; col < visibleW; col++) {
                writePixel(line[col]);
            }
        }
        endWrite();
    }

    template <class Bus, class ColorFormat_>
    inline bool Window<Bus, ColorFormat_>::clip(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const {
        if (w <= 0 || h <= 0) {
            return false;
        }
        // cut the left / top part: move the start to 0, shorten by the cut
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 0) {
            h += y;
            y = 0;
        }
        // cut the right / bottom part
        if ((int32_t)x + w > this->width()) {
            w = this->width() - x;
        }
        if ((int32_t)y + h > this->height()) {
            h = this->height() - y;
        }
        return w > 0 && h > 0;
    }
}

#endif
