# ST7789

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Bus` (SPI): `init(blk)`, `cmdMode()`, `dataMode()`, `write8()`, `write16()`… (`Uno::UnoST7789<DC, RES, CS, BLK>`); a `ColorFormat` (`color::RGB565`, `RGB888`, `RGB444`…) | `ST7789::M130T`: a Screen (`graphics`, `image`, `video` all draw on it), 12 / 16 / 18 bit colors, `fillWindow`, `writeWindow`, backlight |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

The driver of the ST7789 color screen controller, `namespace ST7789`. It is a Screen itself; `graphics`, `image` and `video` all draw on it.

## Usage

```cpp
#include "Uno_ST7789.h"
#include "ST7789.h"          // include only the entry: it includes basic/, tool/ and every module
#include "color.h"

constexpr uint8_t DC_PIN = 8;
constexpr uint8_t RES_PIN = 10;
constexpr uint8_t BLK_PIN = 9;
using Bus = Uno::UnoST7789<DC_PIN, RES_PIN, Uno::NONE, BLK_PIN>;   // the M130T has no CS
using ScreenColor = color::RGB565;               // 16 bit, fastest; RGB888 / RGB666 = 18 bit, RGB444 = 12 bit
ST7789::M130T<Bus, ScreenColor> screen;

void setup() {
    screen.init();                                         // Bus init + reset + initialization (COLMOD from ScreenColor)
    screen.fillWindow(0, 0, 240, 240, ScreenColor::RED);   // fill the whole screen with red
}
```

### Color modes

| `ScreenColor` | Panel mode | Sent a pixel | Speed |
|---|---|---|---|
| `color::RGB565`, `BGR565` | 16 bit (COLMOD 0x55; BGR swapped by MADCTL) | 2 bytes | fastest (about 44 cycles a pixel) |
| `color::RGB666`, `RGB888`, `BGR888`… | 18 bit (COLMOD 0x66) | 3 bytes | about 1.5 times slower; the finest colors (64 levels each) |
| `color::RGB444` | 12 bit (COLMOD 0x53) | 1.5 bytes (3 bytes for two pixels) | the least sent; the coarsest colors (16 levels each) |
| others (`RGB332`, `BW`) | none | — | compile error |

`graphics`, `font`, `image` and `video` all take the color format from the screen (`Screen::ColorFormat`); nothing else to give.

Complete example: `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp`.

## Wiring

The M130T (no CS) on the UNO:

| Module | UNO |
|---|---|
| GND | GND |
| VCC | 3.3V |
| SCL | D13 (SPI SCK, fixed) |
| SDA | D11 (SPI MOSI, fixed) |
| RES | D10 |
| DC | D8 |
| BLK | D9 (PWM, brightness can be set) |

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| x, y of `fillWindow`, `writeWindow` | any `int16_t`, may be off the screen | clipped |
| width, height | > 0 | not drawn |
| the array of `writeWindow` | at least w×h, row by row | not checked: other RAM is read |
| `setWindow(x0, y0, x1, y1)` | 0 ≤ x0 ≤ x1 < width, 0 ≤ y0 ≤ y1 < height | not checked: a broken picture |
| `init(blk)` | backlight 0 ~ 255 | — |
| `setOffset(x, y)` | the panel's place in the 240×320 memory | not checked: a broken picture |
| colors | `ColorFormat` of 12 bit (R4 G4 B4), 16 bit (R5 G6 B5, RGB or BGR), 18 / 24 bit (≤ 8 bit a color) | compile error |
| command arguments of `Datasheet` | see chapter 9 of the datasheet | not checked: sent as they are |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| `madctl()` rotating the picture (MV, MX, MY) | `width()`, `height()` and the offsets do not follow, clipping and coordinates go wrong | when rotating by 90°, swap width and height yourself and set `setOffset` again |
| calling `colmod()` or `madctl()` yourself | no longer matches the `ColorFormat`, the colors go wrong | for another color mode, change the `ColorFormat`; `madctl` must keep `MADCTL_ORDER` (0x08 for BGR565) |
| NVM commands (`nvmSet` and others) | **burn the panel's one-time memory, cannot be undone** | never call them unless you are sure (warnings next to their declarations) |
| read commands (`ramRd()`, `rddid()`…) | the UNO's Bus has no read line, they return 0 | — |
| changing large areas | tearing may show: the M130T has no TE pin, so there is no way to sync with the refresh | change only small areas at a time |
| another module | `M130T` is fixed at 240×240 | add a file in `module/`, see [module/README.md](module/README.md) |
