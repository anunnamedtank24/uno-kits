# video

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Screen`: `setWindow(…)`, `writePixel(color)`… and its color format `ColorFormat` (`ST7789::M130T<Bus, color::RGB565>`, `SSD1306::OLED128x64<Bus, color::BW<>>`) | `video::FlashPlayer`: plays animations burned into Flash |
| streaming: also a `Stream` (`stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`) | `video::StreamPlayer`: the frames stay on the computer and are sent while playing (play, pause, seek, speed, loop) |
| animated pictures or videos (gif, webp, mp4, mov…) | `animation::*`: animations made by `ANIMATION("file")` or `source/convert.py` |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Animations and videos on a screen, `namespace video` (the players) and `namespace animation` (the animations made). The source can be an animated picture (.gif, .webp, .apng) or a video (.mp4, .mov, .webm…), turned into the same frame format by `source/convert.py`.

Two sources:

- **Compiled in**: the frames are burned into Flash (`FlashPlayer`).
- **From the computer**: the frames stay on the computer and come over the USB serial (`StreamPlayer`).

## Usage

### FlashPlayer

```bash
cd lib/video
python source/convert.py assets/orbit.gif output --target flash --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8
python source/convert.py clip.mp4 output --target flash --fit 64x64 --mono --start 3 --duration 2 --max-fps 10
```

```cpp
#include "color.h"
#include "video.h"
video::FlashPlayer<Screen> player(screen);           // colors follow Screen::ColorFormat, see lib/color
if (player.start<animation::orbit_flash_120x120_palette_16>(60, 40)) {   // false if it does not fit the screen
    while (true) {
        player.update();   // draws the next frame only when it is time
    }
}
```

### StreamPlayer

**The computer**: run it once, no arguments. It finds the board by itself and reconnects after unplugging, replugging or a board reset; pictures, animations and sounds all use this one program ([lib/stream](../stream/README.md)).

```bash
cd lib/video
python source/convert.py assets/orbit.gif output --target stream --fit 120x120 --palette 565
cd ../..
python lib/stream/source/stream.py
```

**The UNO**:

```cpp
#include "Uno_serial.h"
#include "stream.h"
constexpr uint32_t BAUD = 1000000;                                  // the same as --baud of stream.py
constexpr uint8_t STREAM_CHUNK = 64;                                // bytes received at a time (RAM), 16 ~ 255
using Link = Uno::UnoSerial<BAUD>;
using StreamLink = stream::Puller<Link, STREAM_CHUNK>;
video::StreamPlayer<Screen, StreamLink> player(screen);
player.open<animation::orbit_stream_120x120_palette_565>(60, 40);     // (x, y, play = true, loop = true, speed = 100)
while (true) {
    player.update();
}
```

### Playing on the OLED

`SSD1306::Frame` ([SSD1306](../SSD1306/README.md)) also has `setWindow`, `writePixel`, `endWrite`, so `FlashPlayer` works directly:

```bash
python source/convert.py assets/orbit.gif output --target flash --fit 64x64 --mono --keyframe 8
```

```cpp
SSD1306::OLED128x64<Bus, color::BW<>> screen;
video::FlashPlayer<SSD1306::OLED128x64<Bus, color::BW<>>> player(screen);   // points with brightness ≥ 128 light up
player.start<animation::orbit_flash_64x64_palette_16_mono>(32, 0);
while (true) {
    if (player.update()) {
        screen.display();    // drawn into the screen in RAM, then the changed parts sent
    }
}
```

- `--mono`: turned into black and white when converting (0x0000 / 0xFFFF), so `color::BW<>` has nothing left to decide; colored animations play too, with `color::BW<threshold>` lighting each point by its brightness.
- Measured 16.5 ~ 17 fps (the animation itself is 16.7 fps), no skipped frames.

Complete examples: `tests[18]`, `tests[19]` of `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp` (ST7789), `src/week_4_SSD1306_switch_buzzer_audio/assignments/2_animation.cpp` (OLED).

## Using the files directly (recommended)

A program writes `ANIMATION("path", "options")` and it is converted before compiling: no need to run `convert.py` first, and the file need not be in `assets/`:

```cpp
#include "video.h"
using Orbit = ANIMATION("clips/orbit.gif", "--width 64 --mono");
player.start<Orbit>(32, 0);
```

| Item | Explanation |
|---|---|
| Path | absolute, `~/…`, relative to the .cpp that writes it; if not found, the project folder, then `lib/video/assets/` |
| Options | one string, the options of `source/convert.py`: `--target`, `--width` / `--fit`…, `--palette`, `--mono`, `--max-fps`…; without `--target` it is `flash` (as for `convert.py`) |
| Where it goes | `lib/video/output/`, the same as running `convert.py` by hand (`src/tools/assets.py` looks before every build: the same file with the same options already converted is used, only otherwise is it converted) |
| Type | `ANIMATION(...)` is that `animation::` struct, used like one converted by hand |
| Limits | both arguments must be string constants (no variables) |

The full explanation: [lib/README.md](../README.md#fonts-pictures-videos-sounds-use-the-files-directly); example: `src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp` (all four kinds). "Usage" above is the way of converting by hand, which works just as well.

## The three targets

| | `--target flash` | `--target stream` | `--target flash stream` |
|---|---|---|---|
| where the frame data is | `output/x.h`, burned into Flash | in the `#if 0` of `output/x.h`, staying on the computer | `output/x.h` |
| the players that work | `FlashPlayer` (`StreamPlayer` too, with a warning) | `StreamPlayer` | both |
| good for | small, few colors, short, no computer needed | big, full color, long | wanting both |

| Used wrong | Result | Why |
|---|---|---|
| `FlashPlayer` given a stream-only file | compile error | the file has no frame data |
| `StreamPlayer` given a flash-only file | compile warning | it plays, but was not checked against the stream's ranges |
| `StreamPlayer` given a 256-color file | compile warning | the palette comes first with the header and is kept in RAM taken at run time, freed only at `close()` or the next open (how it works: see [image's explanation](../image/README.md#streaming-256-colors-the-palette-first-in-ram-taken-at-run-time)) |

## Timing

| | FlashPlayer | StreamPlayer |
|---|---|---|
| who keeps the time | the board (`nowMs`) | the computer; the board only draws where the computer says |
| the first frame | frame 0 (an I-frame) is drawn at the first `update()` | the whole picture is sent first after connecting |
| afterwards | each frame is due at "the sum of the delays of all earlier frames" | the computer sends the difference between "the frame due now" and the one on the screen |
| behind by more than a frame | jumps to the last I-frame already due (skipped ones count in `skippedFrames()`); with no I-frame in between it draws in order and restarts the timing from now (slower, but never drawn wrong) | skips directly, no I-frame needed |

- `--keyframe N`: an I-frame every N frames. The shorter, the faster it catches up when behind, but the bigger the data.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| x, y of `start` / `open` | the whole animation (scaled up) on the screen: x ≥ 0, x + width ≤ screen width, the same for y | not drawn: `start` returns false, `error()` = `ERROR_TOO_BIG` (streamed straight into one window, without clipping) |
| `Animation` | a struct in `output/selected.h` | compile error |
| a row after scaling | with scaling, the stored width ≤ 120 (the board's row buffer) | not drawn (ensured by `convert.py`) |
| `seek(ms)` | 0 ~ the total length; beyond it, the remainder when looping, the end when not | — |
| `setSpeed(percent)` | 1 ~ 1000 | clamped (on the computer) |
| `CHUNK` of `Stream` | 16 ~ 255 | compile error |
| time resolution | 1 ms (`millis`) | — |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| not calling `update()` all the time | no interrupts: drawing happens only when `update()` is called; `FlashPlayer` skips frames to catch up, `StreamPlayer` has the computer skip frames | do nothing long in between |
| drawing something else inside the animation's area | P-frames draw only what changed: with `FlashPlayer` the covered part waits for the next I-frame; with `StreamPlayer` it stays wrong until those pixels change again | do not cover the animation; for `StreamPlayer`, `open` again to redraw |
| part of the animation off the screen | not drawn (no clipping) | move it, or change the size when converting |
| Arduino's `Serial` at the same time | `UnoSerial` uses D0 / D1 too | they cannot be used together; close the serial monitor first too |
| uploading with `stream.py` running | it lets go of the port while uploading, so it can stay running | start `stream.py` first, then upload (opening the port resets the board, see [lib/stream](../stream/README.md#streampy)) |
| large changes on the ST7789 | the M130T has no TE pin, so tearing may show | — |
