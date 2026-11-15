#ifndef ST7789_WINDOW_H
#define ST7789_WINDOW_H

#include "datasheet.h"

namespace ST7789 {
    // window = the area set by CASET + RASET, written with RAMWR
    // coordinates are screen coordinates (offset applied)
    // fillWindow / writeWindow are clipped to the screen: every shape and text is drawn through them,
    // so clipping here is enough for all of graphics (setWindow itself is not clipped)
    template <class Bus>
    class Window : public Datasheet<Bus> {
    public:
        using Datasheet<Bus>::Datasheet;

        // window (inclusive) then RAMWR: stream pixels with writeData16(), then endWrite()
        inline void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

        // solid color, clipped to the screen; w <= 0 or h <= 0 draws nothing
        inline void fillWindow(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

        // color list (w * h colors, row by row), clipped to the screen: the colors of cut parts are skipped
        inline void writeWindow(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *colors);

    private:
        // the part of a rectangle on the screen, false if nothing is left
        inline bool clip(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const;
    };

    // ================= definitions =================

    template <class Bus>
    inline void Window<Bus>::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
        this->caset(x0 + this->xOffset(), x1 + this->xOffset());
        this->raset(y0 + this->yOffset(), y1 + this->yOffset());
        this->ramWr();
    }

    template <class Bus>
    inline void Window<Bus>::fillWindow(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (!clip(x, y, w, h)) {
            return;
        }
        setWindow(x, y, x + w - 1, y + h - 1);
        uint32_t n = (uint32_t)w * h;
        while (n--) {
            this->writeData16(color);
        }
        this->endWrite();
    }

    template <class Bus>
    inline void Window<Bus>::writeWindow(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *colors) {
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
            const uint16_t *line = colors + (uint32_t)(visibleY - y + row) * w + (visibleX - x);
            for (int16_t col = 0; col < visibleW; col++) {
                this->writeData16(line[col]);
            }
        }
        this->endWrite();
    }

    template <class Bus>
    inline bool Window<Bus>::clip(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const {
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