# graphics

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Screen`: `width()`, `height()`, `fillWindow(x, y, w, h, color)` and its color format `ColorFormat` (`ST7789::M130T<Bus, color::RGB565>`, `SSD1306::OLED128x64<Bus, color::BW<>>`) | `graphics::Canvas<Screen>`: points, lines, rectangles, triangles, circles (outlined, filled), text and numbers (`drawTexts`) |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Shapes and text, `namespace graphics`. It uses only the Screen's `fillWindow`, so clipping is left to the Screen too. No color constants: the type and constants of colors follow the screen's color format (`Screen::ColorFormat`, see [color](../color/README.md)); fonts are in [font](../font/README.md).

## Usage

```cpp
#include "graphics.h"
#include "font.h"

using Screen = graphics::Canvas<ST7789::M130T<Bus, color::RGB565>>;
Screen screen;
auto color = Screen::ColorFormat::ORANGE;                      // the screen's color format: orange in RGB565

screen.drawFilledRectSize(10, 10, 60, 40, color);
screen.drawHollowCircleRadius(120, 120, 50, color, 3);
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
screen.drawTexts(10, 200, color, "x = ", 12, ' ', 3.14159);
```

Complete examples: `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp` (ST7789), `src/week_4_SSD1306_switch_buzzer_audio/tests/1_oled.cpp` (OLED).

## Outlined and filled

```cpp
screen.drawFilledRectSize(10, 10, 60, 40, color);
screen.drawHollowRectSize(10, 10, 60, 40, color);        // border 1
screen.drawHollowRectSize(10, 10, 60, 40, color, 5);     // border 5
```

- **The last argument of `Hollow` is the border width**, 1 by default.
- **Coordinates are the outer edge**: x, y, w, h of a rectangle, the three corners of a triangle, the radius of a circle are all the outermost line; the border grows inward. An outlined and a filled shape with the same coordinates have exactly the same outer edge.
- **A border too wide**: over half, it is the same as filled.
- **Which pixels belong to a shape**:
  - circle: the pixel's center is nearer than r + 0.5 to the center. Outlined = the circle of radius r minus the circle of radius r − border, with no holes at any width.
  - triangle: the pixel's center is no more than 0.5 outside every edge. Outlined also removes the pixels "more than border − 0.5 inside every edge".

## What each value draws

| Type | Drawn as | Example |
|---|---|---|
| `char` | that character | `'A'` → `A` |
| `int8_t` (`signed char`), `uint8_t` | a number | `(int8_t)-5` → `-5`, `(uint8_t)200` → `200` |
| `int`, `long`, `unsigned`… | a decimal number, `-` for negatives | the minimum is right too: `-2147483648` |
| `long long`, `unsigned long long` | the same (about 0.7 KB more Flash) | — |
| `bool` | `true` / `false` | — |
| `float`, `double`, `long double` | `setDecimals()` decimals, the last one rounded | `2.345f` → `2.35` (a float has only about 7 significant digits) |
| ↳ special values | `nan`, `inf`, `-inf`, `ovf` (> 4294967040 after rounding) | tiny negatives are drawn as `-0.00` |
| `const char *`, `char[]` | a string, up to `'\0'` | `nullptr`: not drawn |
| `F("...")` | a string in Flash | takes no RAM |
| objects with `c_str()` | a string | Arduino `String` (brings in malloc, about 1.2 KB) |
| other arrays `T[N]` | each element, `setSeparator` between them | `int[]{1, 2, 3}` → `1, 2, 3` (with the separator `", "`); only real arrays, not pointers |
| other types | — | compile error |

| Situation | Result |
|---|---|
| `'\n'`, without `setAutoLineBreak` | drawn as the default character |
| `'\n'`, with `setAutoLineBreak` | a new line: x back to the starting x, y plus `fontHeight() + setLineSpacing()` |
| a character not fitting the screen's width, with `setAutoLineBreak` | a new line first, then drawn; already at the start of a line it is drawn anyway (clipped), never breaking lines forever |
| return value | the x after the last character (on the last line); y is not returned, compute it with `fontHeight()` |
| no value given, no font set | nothing drawn, returns the original x |

## Input limits

**What is off the screen is clipped**: every shape and text ends in the Screen's `fillWindow`, which draws the intersection of the window and the screen (see [ST7789](../ST7789/INTERFACE.md#裁切)). The cost: the part off the screen is still computed, only not sent.

| Item | Range | Beyond it |
|---|---|---|
| x, y, corners, centers, end points | −32768 ~ 32767, may be off the screen | clipped |
| the three corners of a triangle | x and y differences between them < 2048 (e.g. all within −900 ~ 1100) | not checked: the edge computation overflows, drawn wrong |
| the two ends of a line | x and y differences < 32768 | not checked: overflow, drawn wrong |
| w, h (`Size`, `Center`) | > 0 | not drawn |
| radius r | ≥ 0; 0 = one point | not drawn |
| `border` | ≥ 1; over half = filled | not drawn |
| angle of `drawLineAngle` | anything, 0 = right, 90 = up | — |
| length of `drawLineAngle` | ≥ 0 | clipped |
| colors | the screen's `ColorFormat::Value` (`uint16_t` for RGB565, `uint8_t` for BW, `uint32_t` for RGB888) | — |
| x, y of text | the top left corner, may be off the screen | clipped |
| text width | without `setAutoLineBreak`: no line breaks | clipped |
| text height | line breaks do not check the bottom | clipped |
| characters | first ~ last of the font | drawn as the default character |
| `float` / `double` | absolute value ≤ 4294967040 | drawn as `ovf`; nan and inf are drawn too |
| `setDecimals` | 0 ~ 9 is practical (a float has only about 7 significant digits) | — |
| `setSeparator` | only the pointer is kept: the string must stay | not checked |
| font | `setFont<…>()` must come first | not drawn |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| text over old text | only the letters are drawn, the background stays as it was | draw the background yourself first to cover the old text |
| drawing on the OLED | only changes the screen in the board's RAM | call `display()` after drawing |
| drawing an Arduino `String` | brings in malloc, about 1.2 KB of Flash | use `const char *` or `F("...")` |
