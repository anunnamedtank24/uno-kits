# font

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| (nothing); a font file .ttf / .otf | `font::*`: one struct per font (made by `FONT("file")` or `source/convert.py`), for `setFont<…>()` of `graphics::Text` |

Needs no interface: works on any board. The details for developers (prerequisites, how the data is made, input limits) are in [INTERFACE.md](INTERFACE.md).

Font data, `namespace font`. Each font is a struct made by `source/convert.py` from a .ttf / .otf, drawn by `graphics::Text`.

## Usage

```bash
cd lib/font
python source/convert.py assets/menlo.ttf output --height 23
python source/check.py
```

```cpp
#include "font.h"
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
screen.drawTexts(10, 10, color, "hello");
```

Complete examples: `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp` (ST7789), `src/week_4_SSD1306_switch_buzzer_audio/tests/1_oled.cpp` (OLED).

## Using the files directly (recommended)

A program writes `FONT("path", "options")` and it is converted before compiling: no need to run `convert.py` first, and the file need not be in `assets/`:

```cpp
#include "font.h"
using Title = FONT("~/Fonts/Inter.ttf", "--height 16");
screen.setFont<Title>();
```

| Item | Explanation |
|---|---|
| Path | absolute, `~/…`, relative to the .cpp that writes it; if not found, the project folder, then `lib/font/assets/` |
| Options | one string, the options of `source/convert.py`: size (`--height` / `--width` / `--size`), `--first`, `--last`, `--default`; without a size it is `--height 16` |
| Where it goes | `lib/font/output/`, the same as running `convert.py` by hand (`src/tools/assets.py` looks before every build: the same file with the same options already converted is used, only otherwise is it converted) |
| Type | `FONT(...)` is that `font::` struct, used like one converted by hand |
| Limits | both arguments must be string constants (no variables) |

The full explanation: [lib/README.md](../README.md#fonts-pictures-videos-sounds-use-the-files-directly); example: `src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp` (all four kinds). "Usage" above is the way of converting by hand, which works just as well.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| character codes | 0 ~ 255, one byte a character | multi-byte UTF-8 characters such as Chinese split into several bytes, each drawn as the default character |
| `--first` ~ `--last` | 0 ~ 255, first ≤ last, continuous | `convert.py` error |
| grid height, the width of each character | ≤ 255 px | `convert.py` error |
| points, horizontal lines, vertical lines, rectangles of a character | ≤ 255 each | `convert.py` error |
| the shape data of the whole font | ≤ 65535 bytes | `convert.py` error: make the font or the character range smaller |
| the default character `--default` | must be in the font | `convert.py` error |
| font files | .ttf, .otf | `convert.py` error |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| symbols the font does not have (e.g. `°`) | `assets/menlo.ttf` has only ASCII | take the needed characters out of a system font with fontTools first, then convert |
| anti-aliasing | none: every pixel is on or off, and there is no background color | to cover old text, draw the background yourself first |
| small text (12 px) | only 4 px wide, looks rough | use 23 px or more |
| monospaced or not | both work: every character has its own width, all share the height `gridHeight` | — |
