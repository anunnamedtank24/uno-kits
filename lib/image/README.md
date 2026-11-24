# image

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Screen`: `writeWindow(…)` and its color format `ColorFormat` (`ST7789::M130T<Bus, color::RGB565>`, `SSD1306::OLED128x64<Bus, color::BW<>>`) | `image::FlashImage`: draws pictures burned into Flash |
| streaming: also a `Stream` (`stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`) | `image::StreamImage`: the picture stays on the computer and is sent only when drawn |
| picture files (png, jpg, webp, heic…) | `picture::*`: pictures made by `PICTURE("file")` or `source/convert.py` |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Still pictures on a screen, `namespace image` (the drawing code) and `namespace picture` (the pictures made). Like [video](../video/README.md) there are two sources, with almost the same parameters (size, `--scale`, palette or full color):

- **Compiled in**: the picture is burned into Flash (`FlashImage`).
- **From the computer**: the picture stays on the computer and comes over the USB serial (`StreamImage`).

## Usage

### FlashImage

```bash
cd lib/image
python source/convert.py assets/sunset.jpg output --target flash --fit 120x80 --scale 2 --palette 16
```

```cpp
#include "color.h"
#include "image.h"
image::FlashImage<Screen> pictures(screen);                        // Screen = graphics::Canvas<ST7789::M130T<Bus, color::RGB565>>
pictures.draw<picture::sunset_flash_120x80_palette_16>(60, 40);   // top left corner (60, 40)
```

- **Colors**: the colors in the files are always RGB565, changed into the screen's color format (`Screen::ColorFormat`) when drawn: the M130T with RGB565 takes them as they are, at no cost; RGB888 and others are converted automatically; the OLED (`color::BW<threshold>`) lights each point by its brightness. See [lib/color](../color/README.md).

### StreamImage

**The computer**: run it once, no arguments. It finds the board by itself and lets go of the port while uploading; pictures, animations and sounds all use this one program ([lib/stream](../stream/README.md)).

```bash
cd lib/image
python source/convert.py assets/sunset.heic output --target stream --fill 240x240 --palette 565
cd ../..
python lib/stream/source/stream.py
```

**The UNO**:

```cpp
#include "Uno_serial.h"
#include "stream.h"
constexpr uint32_t BAUD = 1000000;          // the same as --baud of stream.py
constexpr uint8_t STREAM_CHUNK = 64;        // bytes received at a time (RAM)
using Link = Uno::UnoSerial<BAUD>;
using StreamLink = stream::Puller<Link, STREAM_CHUNK>;
image::StreamImage<Screen, StreamLink> pictures(screen);
uint8_t result = pictures.draw<picture::sunset_stream_240x240_palette_565>(0, 0);
if (result != pictures.OK) {
    // the error codes are in INTERFACE.md (OK, ERROR_NO_ANSWER, ERROR_NO_MEMORY…)
}
```

### Drawing on the OLED

`Decoder` draws only through `writeWindow`, so the OLED ([SSD1306](../SSD1306/README.md)) works too:

```bash
python source/convert.py assets/logo.png output --target flash stream --fit 128x64 --palette 2
```

```cpp
image::FlashImage<Screen> pictures(screen);   // Screen = graphics::Canvas<SSD1306::OLED128x64<Bus, color::BW<128>>>
pictures.draw<picture::logo_flash_stream_128x64_palette_2>(0, 0);
screen.display();                                      // OLED: send it when drawn
```

- **Use `--palette 2`**: black and white is decided when converting (`--dither` possible), nothing left to decide; colored pictures draw too, with the screen's `color::BW<threshold>` lighting each point by its brightness, but without dithering the dark parts easily turn all black.
- **Photos with `--dither`**: shades of gray by the density of points; `--threshold` decides how bright counts as white, `--invert` swaps black and white.

Complete examples: `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/3_pictures.cpp` (colors), `tests[14]`, `tests[15]` of `src/week_4_SSD1306_switch_buzzer_audio/tests/1_oled.cpp` (OLED), `src/week_4_SSD1306_switch_buzzer_audio/assignments/1_photo_switch.cpp`.

## Using the files directly (recommended)

A program writes `PICTURE("path", "options")` and it is converted before compiling: no need to run `convert.py` first, and the file need not be in `assets/`:

```cpp
#include "image.h"
using Cat = PICTURE("photos/cat.jpg", "--fit 128x64 --palette 2 --dither");
pictures.draw<Cat>(0, 0);
```

| Item | Explanation |
|---|---|
| Path | absolute, `~/…`, relative to the .cpp that writes it; if not found, the project folder, then `lib/image/assets/` |
| Options | one string, the options of `source/convert.py`: `--target`, `--fit` / `--fill` / `--width`…, `--palette`, `--dither`…; without `--target` it is `flash` (as for `convert.py`) |
| Where it goes | `lib/image/output/`, the same as running `convert.py` by hand (`src/tools/assets.py` looks before every build: the same file with the same options already converted is used, only otherwise is it converted) |
| Type | `PICTURE(...)` is that `picture::` struct, used like one converted by hand |
| Limits | both arguments must be string constants (no variables) |

The full explanation: [lib/README.md](../README.md#fonts-pictures-videos-sounds-use-the-files-directly); example: `src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp` (all four kinds). "Usage" above is the way of converting by hand, which works just as well.

## Colors and sources

| `--palette` | A pixel | Good for | Flash (flash) | Streamed (stream) |
|---|---|---|---|---|
| `2` | 1 bit: black or white | OLED, line drawings; photos with `--dither` | ✅ | ✅ |
| `16` | 4 bits, a palette of up to 16 colors | flat color areas, illustrations | ✅ (default) | ✅ |
| `256` | 1 byte, a palette of up to 256 colors | photos, half the size of 565 | ✅ | ⚠️ works, with a warning (see below) |
| `565` | 2 bytes, each pixel its own color | photos, gradients | ✅ (big, with a warning) | ✅ (default) |

`convert.py` picks the smaller encoding by itself: **raw** (each pixel as it is, good for photos and dithered pictures) or **runs** (a stretch of the same color stored as one entry, good for color areas and icons).

| | `--target flash` | `--target stream` | `--target flash stream` |
|---|---|---|---|
| where the pixels are | `output/x.h`, burned into Flash | in the `#if 0` of `output/x.h`, staying on the computer | `output/x.h` |
| the classes that work | `FlashImage` (`StreamImage` too, with a warning) | `StreamImage` | both |
| good for | small, few colors, no computer needed | big, full color (240×240 in full color is about 115 KB; Flash has only 32 KB) | wanting both |

| Used wrong | Result | Why |
|---|---|---|
| `FlashImage` given a stream-only file | compile error | the file has no pixels |
| `StreamImage` given a flash-only file | compile warning | it draws, but was not checked against the stream's ranges |
| `StreamImage` given a 256-color file | compile warning | the palette is sent first and the RAM taken at run time (see below) |

### Streaming 256 colors: the palette first, in RAM taken at run time

Each pixel of a 256-color picture stores "which color number" (0 ~ 255); drawing looks it up in the palette to know the real color. The palette has up to 256 colors:

| | Where the palette is | RAM the board needs |
|---|---|---|
| Flash | burned into Flash with the picture; each pixel looks it up in Flash directly (`readFlash16`) | 0 |
| stream | sent from the computer; Flash cannot be written once burned, so it can only go into RAM | colors × the size of a screen color: at most 512 bytes for RGB565, 1 KB for RGB888 |

What the board does when streaming:

1. **The palette first**: after the header the whole palette comes first (at most 512 bytes), so the first pixel waits that much longer.
2. **RAM taken at run time** (`malloc`): the number of colors is known only from the header, so the RAM the compiler counts (`RAM: used …`) **does not include it**.
3. **Making sure it fits**: avr-libc's `malloc` keeps 128 bytes for the stack; if it does not fit, `ERROR_NO_MEMORY` is returned and nothing is drawn.
4. **Given back when done**: always freed before `draw()` returns (`HeapPalette` `free`s it when leaving the function), never leaked.

So there is a compile **warning** (`PaletteInRam`), and `convert.py --target stream --palette 256` warns too. The UNO has only 2048 bytes of RAM (the OLED's screen alone takes 1 KB), so OLED programs usually do not have room; for an easy life use 2 or 16 colors (the palette is fixed in the object, at most 16 colors) or 565, which has no palette.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| x, y of `draw` | anything | clipped |
| `Picture` | a struct in `output/selected.h` | compile error |
| stored width, height | 1 ~ 240 | `convert.py` error |
| scale | 1 ~ 8 | `convert.py` error |
| colors of the palette | 2: 2 colors; 16: ≤ 16; 256: ≤ 256 | ensured by `convert.py`; a compile error in the Flash version if over |
| `SEGMENT`, `CHUNK` of `Stream` | 1 ~ 255 | 0: compile error |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| `StreamImage::draw` takes long | it returns only when the whole picture is drawn | for big pictures, less data with `--scale` or a palette |
| `pio device monitor` open at the same time | both need the same serial port | close the monitor while streaming; `stream.py` itself handles pictures, animations and sounds together |
| Arduino's `Serial` at the same time | `UnoSerial` uses D0 / D1 too | they cannot be used together |
| wanting animations | this is only one picture | use [video](../video/README.md) |
