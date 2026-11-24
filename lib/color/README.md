# color

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| (nothing) | color formats: `color::RGB565`, `BGR565`, `RGB666`, `RGB888`, `BGR888`, `GBR888`, `RGB444`, `RGB332`, `BW<threshold>`, and `color::Packed<R, G, B, order>` with any bits and any order |
| (nothing) | every format has the same things: common colors (`RED`, `BLACK`…), `color(r, g, b)`, `red(c)` / `green(c)` / `blue(c)`, conversions between them |
| (nothing) | every format can be a screen's `ColorFormat`: `ST7789::M130T<Bus, color::RGB565>`, and graphics, font, image, video all draw with it |

Needs no interface: works on any board. The details for developers (every function, how to write a new format, prerequisites) are in [INTERFACE.md](INTERFACE.md).

## Usage

```cpp
#include "color.h"

using ScreenColor = color::RGB565;                                   // the screen's color format
using Screen = graphics::Canvas<ST7789::M130T<Bus, ScreenColor>>;
Screen screen;

screen.drawFilledScreen(ScreenColor::BLUE);
screen.drawTexts(0, 0, ScreenColor::color(255, 128, 0), "orange");   // red, green, blue 0 ~ 255
image::FlashImage<Screen> pictures(screen);                          // pictures use ScreenColor too, no need to give it again
```

Complete examples: `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp` (RGB565), `src/week_4_SSD1306_switch_buzzer_audio/tests/1_oled.cpp` (`BW`).

- **The color format is written on the screen, in one place**: graphics (shapes), font (text), image (pictures) and video (animations) all take it from the screen, so it cannot be given wrong.
- **Files of pictures and animations**: the colors `convert.py` makes are always stored as RGB565 (streams too), and turned into the screen's format when drawn.

## The formats and where they are used

| Format | Each color | Used for |
|---|---|---|
| `color::RGB565` | 16 bit (red 5, green 6, blue 5) | the fastest mode of the ST7789; also the format of picture and animation files |
| `color::BGR565` | 16 bit, red and blue swapped | ST7789 panels that show red as blue |
| `color::RGB666` | 18 bit (6 each) | the ST7789's 18-bit mode: finer colors, drawing about 1.5 times slower |
| `color::RGB888` | 24 bit (8 each, `0xRRGGBB`) | the usual way on computers; the ST7789 sends it in 18-bit mode (only the top 6 bits of each) |
| `color::RGB444` | 12 bit (4 each) | the ST7789's 12-bit mode: the least data, the coarsest colors |
| `color::BGR888`, `GBR888` | 24 bit, other orders | data in other orders; the ST7789 takes them too (18-bit mode, one color at a time) |
| `color::RGB332` | 8 bit (red 3, green 3, blue 2) | data with 1 byte a color (palettes, LED matrices); **the ST7789 has no such mode**: a compile error on the ST7789 |
| `color::BW<threshold>` | 1 bit: black or white | the SSD1306 OLED: points with brightness ≥ threshold (0 ~ 255, default 128) are lit |
| `color::Packed<R, G, B, order>` | your own: 1 ~ 8 bits each, order RGB / RBG / GRB / GBR / BRG / BGR | formats not above |

## Conversions

```cpp
uint32_t web = color::RGB888::toRGB888(color::RGB888::ORANGE);          // 0xFFA500
uint16_t c = color::convert<color::RGB565, color::RGB888>(0x336699);     // RGB888 → RGB565
uint8_t gray = color::RGB565::green(color::RGB565::GRAY);                // 0 ~ 255
```

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| `color(r, g, b)` | 0 ~ 255 each | — (the low bits are dropped: e.g. RGB565 keeps 5 bits of red and blue, 6 of green) |
| `red/green/blue(c)`, `fromRGB565(c)` | any value | — (extra high bits are ignored) |
| bits of each color of `Packed` | 1 ~ 8 | compile error |
| threshold of `BW` | 0 ~ 255; 0 = all lit | compile error (`uint8_t`) |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| giving the ST7789 a format it does not have (RGB332, BW) | a compile error listing the ones that work | use RGB444 / RGB565 / RGB666 / RGB888 |
| giving the SSD1306 a color format | compile error | use `color::BW<threshold>` |
| the dark parts of a photo on the OLED all black | `BW` uses only the threshold, no dithering | lower the threshold, or convert with `--palette 2 --dither` |
| 18-bit screens (RGB666, RGB888) | drawing about 1.5 times slower; the buffers of pictures and animations take 4 bytes a color | use RGB565 when such fine colors are not needed |
| old code writing `namespace RGB565 = color::RGB565;` | it is a type now: compile error | change it to `using RGB565 = color::RGB565;` |
