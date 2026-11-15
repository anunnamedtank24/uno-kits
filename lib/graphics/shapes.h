#ifndef GRAPHICS_SHAPES_H
#define GRAPHICS_SHAPES_H

#include <stdint.h>
#include <math.h>

namespace graphics {
    // shapes on top of any screen, no color definitions
    //
    // Screen: a class providing (see lib/ST7789/basic/window.h)
    //   uint16_t width() const
    //   uint16_t height() const
    //   void fillWindow(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)  clipped
    //
    // naming: the suffix tells the kind of parameters
    //   Size    top-left x, y + width / height (or length)
    //   Points  two (or three) points, any order, inclusive
    //   Center  center x, y + width / height (or length)
    //   Delta   start x, y + offset dx, dy to the end point
    //   Angle   start x, y + length + degrees (0 = right, 90 = up)
    //   Radius  center x, y + radius
    // draw = outline, fill = solid
    template <class Screen>
    class Shapes : public Screen {
    public:
        using Screen::Screen;

        // ---------- point ----------

        inline void drawPoint(int16_t x, int16_t y, uint16_t color);

        // ---------- horizontal line ----------

        inline void drawHLineSize(int16_t x, int16_t y, int16_t w, uint16_t color);
        inline void drawHLinePoints(int16_t x0, int16_t x1, int16_t y, uint16_t color);
        inline void drawHLineCenter(int16_t cx, int16_t y, int16_t w, uint16_t color);

        // ---------- vertical line ----------

        inline void drawVLineSize(int16_t x, int16_t y, int16_t h, uint16_t color);
        inline void drawVLinePoints(int16_t x, int16_t y0, int16_t y1, uint16_t color);
        inline void drawVLineCenter(int16_t x, int16_t cy, int16_t h, uint16_t color);

        // ---------- line (any direction) ----------

        // Bresenham
        inline void drawLinePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

        inline void drawLineDelta(int16_t x, int16_t y, int16_t dx, int16_t dy, uint16_t color);

        // uses floating point sin / cos
        inline void drawLineAngle(int16_t x, int16_t y, int16_t length, int16_t degrees, uint16_t color);

        // ---------- rectangle ----------

        inline void drawRectSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        inline void drawRectPoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
        inline void drawRectCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color);
        inline void fillRectSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        inline void fillRectPoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
        inline void fillRectCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color);

        // whole screen
        inline void fillScreen(uint16_t color);

        // ---------- triangle ----------
        // Size / Center: isosceles, apex at top middle, base at the bottom

        inline void drawTrianglePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                       int16_t x2, int16_t y2, uint16_t color);
        inline void drawTriangleSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        inline void drawTriangleCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color);

        // horizontal scanlines
        inline void fillTrianglePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                       int16_t x2, int16_t y2, uint16_t color);
        inline void fillTriangleSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        inline void fillTriangleCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color);

        // ---------- circle ----------
        // Points: center + one point on the circle

        // midpoint
        inline void drawCircleRadius(int16_t cx, int16_t cy, int16_t r, uint16_t color);

        inline void drawCirclePoints(int16_t cx, int16_t cy, int16_t px, int16_t py, uint16_t color);

        // vertical lines, each column drawn once
        inline void fillCircleRadius(int16_t cx, int16_t cy, int16_t r, uint16_t color);

        inline void fillCirclePoints(int16_t cx, int16_t cy, int16_t px, int16_t py, uint16_t color);

    private:
        static inline void swap(int16_t &a, int16_t &b);
        static inline int16_t abs16(int16_t v);

        // (x0, y0) becomes top-left, (x1, y1) bottom-right
        static inline void sortPoints(int16_t &x0, int16_t &y0, int16_t &x1, int16_t &y1);

        // rounded integer distance, no floating point
        static inline int16_t distance(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    };

    // ================= definitions =================

    // ---------- point ----------

    template <class Screen>
    inline void Shapes<Screen>::drawPoint(int16_t x, int16_t y, uint16_t color) {
        this->fillWindow(x, y, 1, 1, color);
    }

    // ---------- horizontal line ----------

    template <class Screen>
    inline void Shapes<Screen>::drawHLineSize(int16_t x, int16_t y, int16_t w, uint16_t color) {
        this->fillWindow(x, y, w, 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHLinePoints(int16_t x0, int16_t x1, int16_t y, uint16_t color) {
        if (x0 > x1) {
            swap(x0, x1);
        }
        drawHLineSize(x0, y, x1 - x0 + 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHLineCenter(int16_t cx, int16_t y, int16_t w, uint16_t color) {
        drawHLineSize(cx - w / 2, y, w, color);
    }

    // ---------- vertical line ----------

    template <class Screen>
    inline void Shapes<Screen>::drawVLineSize(int16_t x, int16_t y, int16_t h, uint16_t color) {
        this->fillWindow(x, y, 1, h, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawVLinePoints(int16_t x, int16_t y0, int16_t y1, uint16_t color) {
        if (y0 > y1) {
            swap(y0, y1);
        }
        drawVLineSize(x, y0, y1 - y0 + 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawVLineCenter(int16_t x, int16_t cy, int16_t h, uint16_t color) {
        drawVLineSize(x, cy - h / 2, h, color);
    }

    // ---------- line (any direction) ----------

    template <class Screen>
    inline void Shapes<Screen>::drawLinePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        if (y0 == y1) {
            drawHLinePoints(x0, x1, y0, color);
            return;
        }
        if (x0 == x1) {
            drawVLinePoints(x0, y0, y1, color);
            return;
        }

        bool steep = abs16(y1 - y0) > abs16(x1 - x0);
        if (steep) {
            swap(x0, y0);
            swap(x1, y1);
        }
        if (x0 > x1) {
            swap(x0, x1);
            swap(y0, y1);
        }

        int16_t dx = x1 - x0;
        int16_t dy = abs16(y1 - y0);
        int16_t err = dx / 2;
        int16_t yStep = (y0 < y1) ? 1 : -1;

        for (; x0 <= x1; x0++) {
            if (steep) {
                drawPoint(y0, x0, color);
            } else {
                drawPoint(x0, y0, color);
            }
            err -= dy;
            if (err < 0) {
                y0 += yStep;
                err += dx;
            }
        }
    }

    template <class Screen>
    inline void Shapes<Screen>::drawLineDelta(int16_t x, int16_t y, int16_t dx, int16_t dy, uint16_t color) {
        drawLinePoints(x, y, x + dx, y + dy, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawLineAngle(int16_t x, int16_t y, int16_t length, int16_t degrees, uint16_t color) {
        float rad = degrees * (float)M_PI / 180.0f;
        int16_t dx = (int16_t)lroundf(length * cosf(rad));
        int16_t dy = (int16_t)-lroundf(length * sinf(rad)); // screen y grows down
        drawLineDelta(x, y, dx, dy, color);
    }

    // ---------- rectangle ----------

    template <class Screen>
    inline void Shapes<Screen>::drawRectSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (w <= 0 || h <= 0) {
            return;
        }
        drawHLineSize(x, y, w, color);
        drawHLineSize(x, y + h - 1, w, color);
        drawVLineSize(x, y + 1, h - 2, color);
        drawVLineSize(x + w - 1, y + 1, h - 2, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawRectPoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        sortPoints(x0, y0, x1, y1);
        drawRectSize(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawRectCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color) {
        drawRectSize(cx - w / 2, cy - h / 2, w, h, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::fillRectSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        this->fillWindow(x, y, w, h, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::fillRectPoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        sortPoints(x0, y0, x1, y1);
        fillRectSize(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::fillRectCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color) {
        fillRectSize(cx - w / 2, cy - h / 2, w, h, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::fillScreen(uint16_t color) {
        this->fillWindow(0, 0, this->width(), this->height(), color);
    }

    // ---------- triangle ----------

    template <class Screen>
    inline void Shapes<Screen>::drawTrianglePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                                   int16_t x2, int16_t y2, uint16_t color) {
        drawLinePoints(x0, y0, x1, y1, color);
        drawLinePoints(x1, y1, x2, y2, color);
        drawLinePoints(x2, y2, x0, y0, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawTriangleSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (w <= 0 || h <= 0) {
            return;
        }
        drawTrianglePoints(x + (w - 1) / 2, y, x, y + h - 1, x + w - 1, y + h - 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawTriangleCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color) {
        drawTriangleSize(cx - w / 2, cy - h / 2, w, h, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::fillTrianglePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                                   int16_t x2, int16_t y2, uint16_t color) {
        // sort by y: y0 <= y1 <= y2
        if (y0 > y1) {
            swap(y0, y1);
            swap(x0, x1);
        }
        if (y1 > y2) {
            swap(y1, y2);
            swap(x1, x2);
        }
        if (y0 > y1) {
            swap(y0, y1);
            swap(x0, x1);
        }

        // all on one line
        if (y0 == y2) {
            int16_t a = x0;
            int16_t b = x0;
            if (x1 < a) {
                a = x1;
            }
            if (x1 > b) {
                b = x1;
            }
            if (x2 < a) {
                a = x2;
            }
            if (x2 > b) {
                b = x2;
            }
            drawHLinePoints(a, b, y0, color);
            return;
        }

        for (int16_t y = y0; y <= y2; y++) {
            // long edge: 0 -> 2
            int16_t a = x0 + (int32_t)(x2 - x0) * (y - y0) / (y2 - y0);
            // short edges: 0 -> 1, then 1 -> 2
            int16_t b;
            if (y < y1) {
                b = x0 + (int32_t)(x1 - x0) * (y - y0) / (y1 - y0);
            } else if (y1 == y2) {
                b = x1;
            } else {
                b = x1 + (int32_t)(x2 - x1) * (y - y1) / (y2 - y1);
            }
            drawHLinePoints(a, b, y, color);
        }

        // scanlines miss edge pixels on shallow edges: cover the outline too
        drawTrianglePoints(x0, y0, x1, y1, x2, y2, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::fillTriangleSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (w <= 0 || h <= 0) {
            return;
        }
        fillTrianglePoints(x + (w - 1) / 2, y, x, y + h - 1, x + w - 1, y + h - 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::fillTriangleCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color) {
        fillTriangleSize(cx - w / 2, cy - h / 2, w, h, color);
    }

    // ---------- circle ----------

    template <class Screen>
    inline void Shapes<Screen>::drawCircleRadius(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
        if (r < 0) {
            return;
        }
        int16_t f = 1 - r;
        int16_t ddx = 1;
        int16_t ddy = -2 * r;
        int16_t x = 0;
        int16_t y = r;

        drawPoint(cx, cy + r, color);
        drawPoint(cx, cy - r, color);
        drawPoint(cx + r, cy, color);
        drawPoint(cx - r, cy, color);

        while (x < y) {
            if (f >= 0) {
                y--;
                ddy += 2;
                f += ddy;
            }
            x++;
            ddx += 2;
            f += ddx;

            drawPoint(cx + x, cy + y, color);
            drawPoint(cx - x, cy + y, color);
            drawPoint(cx + x, cy - y, color);
            drawPoint(cx - x, cy - y, color);
            drawPoint(cx + y, cy + x, color);
            drawPoint(cx - y, cy + x, color);
            drawPoint(cx + y, cy - x, color);
            drawPoint(cx - y, cy - x, color);
        }
    }

    template <class Screen>
    inline void Shapes<Screen>::drawCirclePoints(int16_t cx, int16_t cy, int16_t px, int16_t py, uint16_t color) {
        drawCircleRadius(cx, cy, distance(cx, cy, px, py), color);
    }

    template <class Screen>
    inline void Shapes<Screen>::fillCircleRadius(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
        if (r < 0) {
            return;
        }
        int16_t f = 1 - r;
        int16_t ddx = 1;
        int16_t ddy = -2 * r;
        int16_t x = 0;
        int16_t y = r;

        drawVLineSize(cx, cy - r, 2 * r + 1, color);

        while (x < y) {
            if (f >= 0) {
                // columns at +-y are final before y moves: height 2x + 1
                drawVLineSize(cx + y, cy - x, 2 * x + 1, color);
                drawVLineSize(cx - y, cy - x, 2 * x + 1, color);
                y--;
                ddy += 2;
                f += ddy;
            }
            x++;
            ddx += 2;
            f += ddx;

            drawVLineSize(cx + x, cy - y, 2 * y + 1, color);
            drawVLineSize(cx - x, cy - y, 2 * y + 1, color);
        }
    }

    template <class Screen>
    inline void Shapes<Screen>::fillCirclePoints(int16_t cx, int16_t cy, int16_t px, int16_t py, uint16_t color) {
        fillCircleRadius(cx, cy, distance(cx, cy, px, py), color);
    }

    template <class Screen>
    inline void Shapes<Screen>::swap(int16_t &a, int16_t &b) {
        int16_t t = a;
        a = b;
        b = t;
    }

    template <class Screen>
    inline int16_t Shapes<Screen>::abs16(int16_t v) {
        return v < 0 ? -v : v;
    }

    template <class Screen>
    inline void Shapes<Screen>::sortPoints(int16_t &x0, int16_t &y0, int16_t &x1, int16_t &y1) {
        if (x0 > x1) {
            swap(x0, x1);
        }
        if (y0 > y1) {
            swap(y0, y1);
        }
    }

    template <class Screen>
    inline int16_t Shapes<Screen>::distance(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
        int32_t dx = x1 - x0;
        int32_t dy = y1 - y0;
        uint32_t n = dx * dx + dy * dy;

        // integer square root (bit by bit)
        uint32_t root = 0;
        uint32_t bit = 1UL << 30;
        while (bit > n) {
            bit >>= 2;
        }
        while (bit != 0) {
            if (n >= root + bit) {
                n -= root + bit;
                root = (root >> 1) + bit;
            } else {
                root >>= 1;
            }
            bit >>= 2;
        }
        // n is now the remainder: round up if remainder > root
        if (n > root) {
            root++;
        }
        return (int16_t)root;
    }
}

#endif
