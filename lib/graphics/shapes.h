#ifndef GRAPHICS_SHAPES_H
#define GRAPHICS_SHAPES_H

#include <stdint.h>
#include <math.h>

namespace graphics {
    // shapes on top of any screen, no color definitions
    //
    // Screen: a class providing (see lib/ST7789/basic/window.h)
    //   ColorFormat: the type of its colors (lib/color: color::RGB565, color::BW<> ...), ColorFormat::Value
    //   uint16_t width() const
    //   uint16_t height() const
    //   void fillWindow(int16_t x, int16_t y, int16_t w, int16_t h, ColorFormat::Value color)  clipped to the screen:
    //   every shape draws through it, so shapes are clipped without checks of their own
    //
    // naming: the suffix tells the kind of parameters
    //   Size    top-left x, y + width / height (or length)
    //   Points  two (or three) points, any order, inclusive
    //   Center  center x, y + width / height (or length)
    //   Delta   start x, y + offset dx, dy to the end point
    //   Angle   start x, y + length + degrees (0 = right, 90 = up)
    //   Radius  center x, y + radius
    // Hollow = outline (border width, growing inward), Filled = solid
    template <class Screen>
    class Shapes : public Screen {
    public:
        using Screen::Screen;

        // the screen's color: Screen::ColorFormat::Value (uint16_t for RGB565, uint8_t for BW ...)
        using ColorFormat = typename Screen::ColorFormat;
        using Color = typename ColorFormat::Value;

        // ---------- point ----------

        inline void drawPoint(int16_t x, int16_t y, Color color);

        // ---------- horizontal line ----------

        inline void drawHLineSize(int16_t x, int16_t y, int16_t w, Color color);
        inline void drawHLinePoints(int16_t x0, int16_t x1, int16_t y, Color color);
        inline void drawHLineCenter(int16_t cx, int16_t y, int16_t w, Color color);

        // ---------- vertical line ----------

        inline void drawVLineSize(int16_t x, int16_t y, int16_t h, Color color);
        inline void drawVLinePoints(int16_t x, int16_t y0, int16_t y1, Color color);
        inline void drawVLineCenter(int16_t x, int16_t cy, int16_t h, Color color);

        // ---------- line (any direction) ----------

        // Bresenham
        inline void drawLinePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color);

        inline void drawLineDelta(int16_t x, int16_t y, int16_t dx, int16_t dy, Color color);

        // uses floating point sin / cos
        inline void drawLineAngle(int16_t x, int16_t y, int16_t length, int16_t degrees, Color color);

        // ---------- rectangle ----------
        // Hollow: (x, y, w, h) is the outer edge, the border grows inward;
        //         a border of half the size or more fills the rectangle

        inline void drawHollowRectSize(int16_t x, int16_t y, int16_t w, int16_t h, Color color, int16_t border = 1);
        inline void drawHollowRectPoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color, int16_t border = 1);
        inline void drawHollowRectCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, Color color, int16_t border = 1);
        inline void drawFilledRectSize(int16_t x, int16_t y, int16_t w, int16_t h, Color color);
        inline void drawFilledRectPoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color);
        inline void drawFilledRectCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, Color color);

        // whole screen
        inline void drawFilledScreen(Color color);

        // ---------- triangle ----------
        // Size / Center: isosceles, apex at top middle, base at the bottom
        // every pixel of the corners' bounding box on the screen is tested (small code, slow: about 100 cycles per box pixel)
        // corners: their x and y differences must stay below 2048 (e.g. all in -900 ~ 1100), or the edge math overflows
        // Filled: pixels whose center is at most 0.5 outside every edge (edges about 1 pixel thick)
        // Hollow: minus the pixels more than border - 0.5 inside every edge,
        //         so the corners are the outer edge and the border grows inward

        inline void drawHollowTrianglePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                             int16_t x2, int16_t y2, Color color, int16_t border = 1);
        inline void drawHollowTriangleSize(int16_t x, int16_t y, int16_t w, int16_t h, Color color, int16_t border = 1);
        inline void drawHollowTriangleCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, Color color, int16_t border = 1);
        inline void drawFilledTrianglePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                             int16_t x2, int16_t y2, Color color);
        inline void drawFilledTriangleSize(int16_t x, int16_t y, int16_t w, int16_t h, Color color);
        inline void drawFilledTriangleCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, Color color);

        // ---------- circle ----------
        // Points: center + one point on the circle
        // pixel (x, y) is in the circle of radius r when (x - cx)^2 + (y - cy)^2 <= r^2 + r,
        // i.e. its center is closer than r + 0.5
        // Filled: drawn column by column, each column once
        // Hollow: the circle of radius r minus the circle of radius r - border,
        //         so r is the outer edge and the border grows inward, without holes for any border

        inline void drawHollowCircleRadius(int16_t cx, int16_t cy, int16_t r, Color color, int16_t border = 1);
        inline void drawHollowCirclePoints(int16_t cx, int16_t cy, int16_t px, int16_t py, Color color, int16_t border = 1);
        inline void drawFilledCircleRadius(int16_t cx, int16_t cy, int16_t r, Color color);
        inline void drawFilledCirclePoints(int16_t cx, int16_t cy, int16_t px, int16_t py, Color color);

    private:
        static inline void swap(int16_t &a, int16_t &b);
        static inline int16_t abs16(int16_t v);

        // (x0, y0) becomes top-left, (x1, y1) bottom-right
        static inline void sortPoints(int16_t &x0, int16_t &y0, int16_t &x1, int16_t &y1);

        // rounded integer distance, no floating point
        static inline int16_t distance(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

        // border <= 0: filled
        inline void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                 int16_t x2, int16_t y2, Color color, int16_t border);

        // pixels whose center is closer than outer + 0.5, minus the ones closer than inner + 0.5
        // inner < 0: filled
        inline void drawRing(int16_t cx, int16_t cy, int16_t outer, int16_t inner, Color color);
    };

    // ================= definitions =================

    // ---------- point ----------

    template <class Screen>
    inline void Shapes<Screen>::drawPoint(int16_t x, int16_t y, Color color) {
        this->fillWindow(x, y, 1, 1, color);
    }

    // ---------- horizontal line ----------

    template <class Screen>
    inline void Shapes<Screen>::drawHLineSize(int16_t x, int16_t y, int16_t w, Color color) {
        this->fillWindow(x, y, w, 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHLinePoints(int16_t x0, int16_t x1, int16_t y, Color color) {
        if (x0 > x1) {
            swap(x0, x1);
        }
        drawHLineSize(x0, y, x1 - x0 + 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHLineCenter(int16_t cx, int16_t y, int16_t w, Color color) {
        drawHLineSize(cx - w / 2, y, w, color);
    }

    // ---------- vertical line ----------

    template <class Screen>
    inline void Shapes<Screen>::drawVLineSize(int16_t x, int16_t y, int16_t h, Color color) {
        this->fillWindow(x, y, 1, h, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawVLinePoints(int16_t x, int16_t y0, int16_t y1, Color color) {
        if (y0 > y1) {
            swap(y0, y1);
        }
        drawVLineSize(x, y0, y1 - y0 + 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawVLineCenter(int16_t x, int16_t cy, int16_t h, Color color) {
        drawVLineSize(x, cy - h / 2, h, color);
    }

    // ---------- line (any direction) ----------

    template <class Screen>
    inline void Shapes<Screen>::drawLinePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color) {
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
    inline void Shapes<Screen>::drawLineDelta(int16_t x, int16_t y, int16_t dx, int16_t dy, Color color) {
        drawLinePoints(x, y, x + dx, y + dy, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawLineAngle(int16_t x, int16_t y, int16_t length, int16_t degrees, Color color) {
        float rad = degrees * (float)M_PI / 180.0f;
        int16_t dx = (int16_t)lroundf(length * cosf(rad));
        int16_t dy = (int16_t)-lroundf(length * sinf(rad)); // screen y grows down
        drawLineDelta(x, y, dx, dy, color);
    }

    // ---------- rectangle ----------

    template <class Screen>
    inline void Shapes<Screen>::drawHollowRectSize(int16_t x, int16_t y, int16_t w, int16_t h, Color color, int16_t border) {
        if (w <= 0 || h <= 0 || border <= 0) {
            return;
        }
        if (2 * (int32_t)border >= w || 2 * (int32_t)border >= h) {
            drawFilledRectSize(x, y, w, h, color);
            return;
        }
        this->fillWindow(x, y, w, border, color);
        this->fillWindow(x, y + h - border, w, border, color);
        this->fillWindow(x, y + border, border, h - 2 * border, color);
        this->fillWindow(x + w - border, y + border, border, h - 2 * border, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHollowRectPoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color, int16_t border) {
        sortPoints(x0, y0, x1, y1);
        drawHollowRectSize(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, border);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHollowRectCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, Color color, int16_t border) {
        drawHollowRectSize(cx - w / 2, cy - h / 2, w, h, color, border);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledRectSize(int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
        this->fillWindow(x, y, w, h, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledRectPoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color) {
        sortPoints(x0, y0, x1, y1);
        drawFilledRectSize(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledRectCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, Color color) {
        drawFilledRectSize(cx - w / 2, cy - h / 2, w, h, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledScreen(Color color) {
        this->fillWindow(0, 0, this->width(), this->height(), color);
    }

    // ---------- triangle ----------

    template <class Screen>
    inline void Shapes<Screen>::drawHollowTrianglePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                                         int16_t x2, int16_t y2, Color color, int16_t border) {
        if (border <= 0) {
            return;
        }
        drawTriangle(x0, y0, x1, y1, x2, y2, color, border);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHollowTriangleSize(int16_t x, int16_t y, int16_t w, int16_t h, Color color, int16_t border) {
        if (w <= 0 || h <= 0) {
            return;
        }
        drawHollowTrianglePoints(x + (w - 1) / 2, y, x, y + h - 1, x + w - 1, y + h - 1, color, border);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHollowTriangleCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, Color color, int16_t border) {
        drawHollowTriangleSize(cx - w / 2, cy - h / 2, w, h, color, border);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledTrianglePoints(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                                         int16_t x2, int16_t y2, Color color) {
        drawTriangle(x0, y0, x1, y1, x2, y2, color, 0);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledTriangleSize(int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
        if (w <= 0 || h <= 0) {
            return;
        }
        drawFilledTrianglePoints(x + (w - 1) / 2, y, x, y + h - 1, x + w - 1, y + h - 1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledTriangleCenter(int16_t cx, int16_t cy, int16_t w, int16_t h, Color color) {
        drawFilledTriangleSize(cx - w / 2, cy - h / 2, w, h, color);
    }

    // ---------- circle ----------

    template <class Screen>
    inline void Shapes<Screen>::drawHollowCircleRadius(int16_t cx, int16_t cy, int16_t r, Color color, int16_t border) {
        if (r < 0 || border <= 0) {
            return;
        }
        drawRing(cx, cy, r, r - border, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawHollowCirclePoints(int16_t cx, int16_t cy, int16_t px, int16_t py, Color color, int16_t border) {
        drawHollowCircleRadius(cx, cy, distance(cx, cy, px, py), color, border);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledCircleRadius(int16_t cx, int16_t cy, int16_t r, Color color) {
        if (r < 0) {
            return;
        }
        drawRing(cx, cy, r, -1, color);
    }

    template <class Screen>
    inline void Shapes<Screen>::drawFilledCirclePoints(int16_t cx, int16_t cy, int16_t px, int16_t py, Color color) {
        drawFilledCircleRadius(cx, cy, distance(cx, cy, px, py), color);
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

    template <class Screen>
    inline void Shapes<Screen>::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                             int16_t x2, int16_t y2, Color color, int16_t border) {
        // corners counterclockwise on screen (y down): every edge function is positive inside
        if ((int32_t)(x2 - x0) * (y1 - y0) - (int32_t)(y2 - y0) * (x1 - x0) < 0) {
            swap(x1, x2);
            swap(y1, y2);
        }
        const int16_t corners[8] = { x0, y0, x1, y1, x2, y2, x0, y0 };

        // bounding box: keeps the 0.5 pixel tolerance from growing spikes at sharp corners
        int16_t left = x0;
        int16_t right = x0;
        int16_t top = y0;
        int16_t bottom = y0;
        for (uint8_t i = 2; i < 6; i += 2) {
            left = corners[i] < left ? corners[i] : left;
            right = corners[i] > right ? corners[i] : right;
            top = corners[i + 1] < top ? corners[i + 1] : top;
            bottom = corners[i + 1] > bottom ? corners[i + 1] : bottom;
        }
        // only the part on the screen is tested (corners far away would make the box huge)
        left = left < 0 ? 0 : left;
        top = top < 0 ? 0 : top;
        right = right > (int16_t)this->width() - 1 ? (int16_t)this->width() - 1 : right;
        bottom = bottom > (int16_t)this->height() - 1 ? (int16_t)this->height() - 1 : bottom;

        // edge a -> b: E(x, y) = (x - ax) * (by - ay) - (y - ay) * (bx - ax) = distance * length
        // kept as 32 * E, the length as 16 * length, so 0.5 pixel = 1 length unit
        int32_t value[3];      // 32 * E at (left, y)
        int32_t stepX[3];      // + 32 * (by - ay) per pixel to the right
        int32_t stepY[3];      // - 32 * (bx - ax) per row down
        int32_t outerLimit[3]; // drawn: distance >= -0.5
        int32_t holeLimit[3];  // hole: distance > border - 0.5
        for (uint8_t i = 0; i < 3; i++) {
            const int16_t *a = corners + 2 * i;
            const int16_t *b = a + 2;
            int16_t dx = b[0] - a[0];
            int16_t dy = b[1] - a[1];
            int32_t length = distance(0, 0, dx * 16, dy * 16);
            stepX[i] = (int32_t)dy * 32;
            stepY[i] = -(int32_t)dx * 32;
            value[i] = (int32_t)(left - a[0]) * stepX[i] + (int32_t)(top - a[1]) * stepY[i];
            outerLimit[i] = -length;
            holeLimit[i] = border > 0 ? (2 * (int32_t)border - 1) * length : 0x7FFFFFFF;
        }

        for (int16_t y = top; y <= bottom; y++) {
            int32_t e[3] = { value[0], value[1], value[2] };
            int16_t runStart = -1;
            // one column past the box closes the last run
            for (int16_t x = left; x <= right + 1; x++) {
                bool drawn = x <= right;
                bool hole = true;
                for (uint8_t i = 0; i < 3; i++) {
                    if (e[i] < outerLimit[i]) {
                        drawn = false;
                    }
                    if (e[i] <= holeLimit[i]) {
                        hole = false;
                    }
                    e[i] += stepX[i];
                }
                drawn = drawn && !hole;
                if (drawn && runStart < 0) {
                    runStart = x;
                } else if (!drawn && runStart >= 0) {
                    drawHLineSize(runStart, y, x - runStart, color);
                    runStart = -1;
                }
            }
            for (uint8_t i = 0; i < 3; i++) {
                value[i] += stepY[i];
            }
        }
    }

    template <class Screen>
    inline void Shapes<Screen>::drawRing(int16_t cx, int16_t cy, int16_t outer, int16_t inner, Color color) {
        int32_t outerLimit = (int32_t)outer * outer + outer;
        int32_t innerLimit = inner >= 0 ? (int32_t)inner * inner + inner : -1;
        // half heights of the column, they only shrink while dx grows
        int16_t outerHalf = outer;
        int16_t innerHalf = inner;

        for (int16_t dx = 0; dx <= outer; dx++) {
            int32_t dxSquare = (int32_t)dx * dx;
            while ((int32_t)outerHalf * outerHalf > outerLimit - dxSquare) {
                outerHalf--;
            }
            // the column: one part, or a top part and a bottom part around the hole
            bool hasHole = dxSquare <= innerLimit;
            int16_t top = cy - outerHalf;
            int16_t h = 2 * outerHalf + 1;
            int16_t bottom = 0;
            if (hasHole) {
                while ((int32_t)innerHalf * innerHalf > innerLimit - dxSquare) {
                    innerHalf--;
                }
                h = outerHalf - innerHalf;
                bottom = cy + innerHalf + 1;
            }
            // parts: right top, right bottom, left top, left bottom; one call site so drawVLineSize may be inlined
            int16_t x = cx + dx;
            for (uint8_t part = 0; part < 4; part++) {
                if (part == 2) {
                    if (dx == 0) {
                        break;
                    }
                    x = cx - dx;
                }
                bool isBottom = part & 1;
                if (isBottom && !hasHole) {
                    continue;
                }
                drawVLineSize(x, isBottom ? bottom : top, h, color);
            }
        }
    }
}

#endif
