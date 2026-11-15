#ifndef SSD1306_FRAME_H
#define SSD1306_FRAME_H

#include "datasheet.h"

namespace SSD1306 {
    // a picture of the whole panel in the board's RAM (WIDTH x HEIGHT / 8 bytes), sent with display()
    //
    // why a copy in RAM: a byte of the display RAM holds 8 rows, so changing one dot means
    // reading its byte first, and the display RAM cannot be read over I2C
    //
    // drawing only changes the copy; display() sends the changed parts:
    // per page, the columns from the first to the last changed one
    //
    // color: 0 = dot off, anything else = dot on (e.g. color::RGB565::BLACK / WHITE)
    // fillWindow / writeWindow are clipped to the panel, so graphics::Shapes / Text work on top
    // setWindow / writeData16 / endWrite stream pixels like an ST7789 window (for video::FlashPlayer):
    // here they fill the RAM copy, pixels off the panel are dropped
    //
    // needs horizontal addressing (setAddressingMode(ADDRESSING_HORIZONTAL)): the modules' init() sets it
    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    class Frame : public Datasheet<Bus> {
    public:
        static_assert(WIDTH >= 1 && WIDTH <= 128, "SSD1306::Frame: WIDTH must be 1 ~ 128");
        static_assert(HEIGHT >= 8 && HEIGHT <= 64 && HEIGHT % 8 == 0, "SSD1306::Frame: HEIGHT must be 8 ~ 64, a multiple of 8");

        static constexpr uint8_t PAGES = HEIGHT / 8;
        static constexpr uint16_t BYTES = (uint16_t)WIDTH * PAGES;

        using Datasheet<Bus>::Datasheet;

        inline uint16_t width() const;
        inline uint16_t height() const;

        // ---------- drawing (the RAM copy only) ----------

        // solid color, clipped to the panel; w <= 0 or h <= 0 draws nothing
        inline void fillWindow(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

        // color list (w * h colors, row by row), clipped: the colors of cut parts are skipped
        inline void writeWindow(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *colors);

        // false outside the panel
        inline bool getPixel(int16_t x, int16_t y) const;

        // every dot off
        inline void clear();

        // ---------- streaming (the same calls as ST7789::Window, e.g. for video players) ----------

        // window (inclusive): the next writeData16() calls fill it row by row, from the top left
        inline void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
        inline void writeData16(uint16_t color);

        // nothing to finish: the pixels are already in the RAM copy (display() sends them)
        inline void endWrite();

        // ---------- sending ----------

        // the changed parts; false if the panel did not answer
        inline bool display();

        // everything (e.g. after the panel lost its RAM, or after scrolling)
        inline bool displayAll();

    private:
        uint8_t _buffer[BYTES] = {};

        // changed columns per page, inclusive; start > end = nothing changed
        uint8_t _dirtyStart[PAGES];
        uint8_t _dirtyEnd[PAGES];
        bool _dirtyInitialized = false;

        // the window of setWindow() and where the next pixel goes
        uint16_t _windowLeft = 0;
        uint16_t _windowRight = 0;
        uint16_t _cursorX = 0;
        uint16_t _cursorY = 0;

        inline void markAllDirty();
        inline void markDirty(uint8_t page, uint8_t start, uint8_t end);

        // the part of a rectangle on the panel, false if nothing is left
        inline bool clip(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const;
    };

    // ================= definitions =================

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline uint16_t Frame<Bus, WIDTH, HEIGHT>::width() const {
        return WIDTH;
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline uint16_t Frame<Bus, WIDTH, HEIGHT>::height() const {
        return HEIGHT;
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline void Frame<Bus, WIDTH, HEIGHT>::fillWindow(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (!clip(x, y, w, h)) {
            return;
        }
        int16_t bottom = y + h - 1;
        for (uint8_t page = (uint8_t)(y >> 3); page <= (uint8_t)(bottom >> 3); page++) {
            // the rows of this page inside y ~ bottom
            int16_t pageTop = page * 8;
            uint8_t first = y > pageTop ? (uint8_t)(y - pageTop) : 0;
            uint8_t last = bottom < pageTop + 7 ? (uint8_t)(bottom - pageTop) : 7;
            uint8_t mask = (uint8_t)((0xFF << first) & (0xFF >> (7 - last)));
            uint8_t *bytes = _buffer + (uint16_t)page * WIDTH + x;
            for (int16_t i = 0; i < w; i++) {
                if (color) {
                    bytes[i] |= mask;
                } else {
                    bytes[i] &= (uint8_t)~mask;
                }
            }
            markDirty(page, (uint8_t)x, (uint8_t)(x + w - 1));
        }
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline void Frame<Bus, WIDTH, HEIGHT>::writeWindow(int16_t x, int16_t y, int16_t w, int16_t h,
                                                       const uint16_t *colors) {
        int16_t visibleX = x;
        int16_t visibleY = y;
        int16_t visibleW = w;
        int16_t visibleH = h;
        if (!clip(visibleX, visibleY, visibleW, visibleH)) {
            return;
        }
        for (int16_t row = 0; row < visibleH; row++) {
            int16_t screenY = visibleY + row;
            const uint16_t *line = colors + (uint32_t)(screenY - y) * w + (visibleX - x);
            uint8_t bit = (uint8_t)(1 << (screenY & 7));
            uint8_t *bytes = _buffer + (uint16_t)(screenY >> 3) * WIDTH + visibleX;
            for (int16_t col = 0; col < visibleW; col++) {
                if (line[col]) {
                    bytes[col] |= bit;
                } else {
                    bytes[col] &= (uint8_t)~bit;
                }
            }
            markDirty((uint8_t)(screenY >> 3), (uint8_t)visibleX, (uint8_t)(visibleX + visibleW - 1));
        }
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline bool Frame<Bus, WIDTH, HEIGHT>::getPixel(int16_t x, int16_t y) const {
        if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) {
            return false;
        }
        return (_buffer[(uint16_t)(y >> 3) * WIDTH + x] >> (y & 7)) & 0x01;
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline void Frame<Bus, WIDTH, HEIGHT>::clear() {
        for (uint16_t i = 0; i < BYTES; i++) {
            _buffer[i] = 0;
        }
        markAllDirty();
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline void Frame<Bus, WIDTH, HEIGHT>::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
        _windowLeft = x0;
        _windowRight = x1;
        _cursorX = x0;
        _cursorY = y0;
        // the part on the panel will change
        if (x0 < WIDTH && y0 < HEIGHT && x1 >= x0 && y1 >= y0) {
            uint8_t right = (uint8_t)(x1 < WIDTH ? x1 : WIDTH - 1);
            uint8_t bottom = (uint8_t)(y1 < HEIGHT ? y1 : HEIGHT - 1);
            for (uint8_t page = (uint8_t)(y0 >> 3); page <= (uint8_t)(bottom >> 3); page++) {
                markDirty(page, (uint8_t)x0, right);
            }
        }
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline void Frame<Bus, WIDTH, HEIGHT>::writeData16(uint16_t color) {
        if (_cursorX < WIDTH && _cursorY < HEIGHT) {
            uint8_t *byte = _buffer + (uint16_t)(_cursorY >> 3) * WIDTH + _cursorX;
            uint8_t bit = (uint8_t)(1 << (_cursorY & 7));
            if (color) {
                *byte |= bit;
            } else {
                *byte &= (uint8_t)~bit;
            }
        }
        // next pixel: right, then the start of the next row
        if (_cursorX >= _windowRight) {
            _cursorX = _windowLeft;
            _cursorY++;
        } else {
            _cursorX++;
        }
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline void Frame<Bus, WIDTH, HEIGHT>::endWrite() {
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline bool Frame<Bus, WIDTH, HEIGHT>::display() {
        if (!_dirtyInitialized) {
            markAllDirty();
        }
        bool ok = true;
        for (uint8_t page = 0; page < PAGES; page++) {
            uint8_t start = _dirtyStart[page];
            uint8_t end = _dirtyEnd[page];
            if (start > end) {
                continue;
            }
            // the window: these columns of this page (one command stream)
            const uint8_t window[6] = { SET_COLUMN_ADDRESS, start, end, SET_PAGE_ADDRESS, page, page };
            this->sendCommand(window, 6);
            ok = ok && this->ok();
            this->beginData();
            const uint8_t *bytes = _buffer + (uint16_t)page * WIDTH;
            for (uint8_t col = start; col <= end; col++) {
                this->writeByte(bytes[col]);
            }
            this->end();
            ok = ok && this->ok();
            _dirtyStart[page] = WIDTH;
            _dirtyEnd[page] = 0;
        }
        return ok;
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline bool Frame<Bus, WIDTH, HEIGHT>::displayAll() {
        markAllDirty();
        return display();
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline void Frame<Bus, WIDTH, HEIGHT>::markAllDirty() {
        for (uint8_t page = 0; page < PAGES; page++) {
            _dirtyStart[page] = 0;
            _dirtyEnd[page] = WIDTH - 1;
        }
        _dirtyInitialized = true;
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline void Frame<Bus, WIDTH, HEIGHT>::markDirty(uint8_t page, uint8_t start, uint8_t end) {
        if (!_dirtyInitialized) {
            // the first display() sends everything anyway
            return;
        }
        if (start < _dirtyStart[page]) {
            _dirtyStart[page] = start;
        }
        // nothing changed yet = start WIDTH, end 0: min / max still give start ~ end
        if (end > _dirtyEnd[page]) {
            _dirtyEnd[page] = end;
        }
    }

    template <class Bus, uint8_t WIDTH, uint8_t HEIGHT>
    inline bool Frame<Bus, WIDTH, HEIGHT>::clip(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const {
        if (w <= 0 || h <= 0) {
            return false;
        }
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 0) {
            h += y;
            y = 0;
        }
        if ((int32_t)x + w > WIDTH) {
            w = WIDTH - x;
        }
        if ((int32_t)y + h > HEIGHT) {
            h = HEIGHT - y;
        }
        return w > 0 && h > 0;
    }
}

#endif
