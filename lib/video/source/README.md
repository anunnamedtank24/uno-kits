# video/source

English | [中文](讀我.md)

Tools that run on the computer. Needs Python 3, `pip install pillow` (streaming also needs `pyserial`); videos (.mp4, .mov, .webm…) also need `brew install ffmpeg`. All run inside `lib/video`.

## Files

| File | What it does |
|---|---|
| `config.json` | the parameter ranges and defaults of the two targets (flash, stream); the board's numbers (clock, flash, drawing cycles) are in its `board.json` (`--board`) |
| `convert.py` | animations, videos → `output/*.h`, and adds the new one to `output/selected.h` |
| `check.py` | checks every file of `output/`: the current format, an unchanged source (`convert.py` runs it after every conversion) |
| `stream_handler.py` | used by `stream.py` of [lib/stream](../../stream/README.md) to read animations, keep the time and send frames (commands `'O'`, `'N'`, `'P'`, `'S'`, `'J'`, `'V'`, `'L'`); not run on its own |

## Usage

```bash
cd lib/video
python source/convert.py assets/orbit.gif output --target flash --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8
python source/convert.py ~/Movies/clip.mp4 output --target flash --fit 64x64 --mono --start 3 --duration 2 --max-fps 10
python source/check.py
cd ../.. && python lib/stream/source/stream.py     # streaming: it serves pictures, animations and sounds
```

## convert.py

### Files supported

| Extension | Read by | Time of each frame |
|---|---|---|
| `.gif`, `.webp` (animated), `.png` / `.apng` (animated) | Pillow, with the blending and disposal rules already handled | each frame's own time in the file (under 20 ms is taken as 100 ms, as browsers do) |
| `.mp4`, `.mov`, `.m4v`, `.webm`, `.mkv`, `.avi` | ffmpeg (only the picture, the sound is dropped) | the video's fps, at most 30 (or `--max-fps`) |

- **Videos are shrunk first**: a long side over 480 px is first shrunk to 480 (the screen is only 240), so long videos fit in memory.
- **A single still picture** works too (one frame), shown all the time.
- **Measured**: the same animation saved as gif, webp, apng, mp4, mov, m4v, mkv, avi, webm, converted to 64×64 black and white, always gives 24 frames, 876 ~ 888 bytes; a 1920×1080, 60 fps video is shrunk to 480×270 and thinned to 30 fps.

### Targets

`--target` takes one or two; left out it is `flash`:

| | `--target flash` | `--target stream` | `--target flash stream` |
|---|---|---|---|
| where the frame data is | `output/x.h`, burned into Flash | in the `#if 0` of `output/x.h`, staying on the computer | `output/x.h` |
| what `output/x.h` has | frame data + path | only size, times, path | frame data + path |
| colors (`--palette`) | 16 (default), 256, 565 (warning) | 16, 256 (warning: the palette is sent first, the board takes RAM at run time), 565 (default) | 16 (default), 256, 565 (warning) |
| ranges | flash's | stream's | what both allow (the intersection) |

### Time

| Parameter | Default | Explanation |
|---|---|---|
| `--start S` | 0 | starts at second S |
| `--duration S` | to the end | takes S seconds. The UNO's Flash holds only a few seconds: videos usually need cutting |
| `--max-fps F` | — | frames less than 1/F apart are dropped, their time added to the frame before, the total length unchanged; videos are thinned straight to this fps |
| `--min-fps F` | — | the lower limit when `--max-bytes` lowers the frame rate; also a warning when the board is estimated not to reach this fps |

### Size

Every size means the size "on the screen", that is, after scaling up:

| Parameter | Explanation |
|---|---|
| (none given) | the original size |
| `--width W` or `--height H` | only one: the aspect ratio is kept |
| `--width W --height H` | both: exactly this size (stretched) |
| `--resize F` / `--resize FXxFY` | a multiple of the original size, decimals allowed, x and y may differ, e.g. `0.75` or `1.5x1` |
| `--fit WxH` | keeps the aspect ratio, as big as fits in a W×H box |
| `--fill WxH` | keeps the aspect ratio, covers W×H, cutting off the excess from the middle |
| `--scale N` / `--scale NXxNY` | the board draws each point NX×NY times, whole numbers only. The data stores only "screen size ÷ scale" |
| `--resample` | the scaling method: `nearest` (pixel art), `bilinear`, `bicubic`, `lanczos` (default) |

- For scaling by a fraction use `--resize` or `--width`; `--scale` is only for whole multiples, to save data.
- A size that is not a whole multiple of scale is rounded, with a warning giving the real size.

### Colors

| Parameter | Explanation |
|---|---|
| `--palette 16` | a shared palette of up to 16 colors, 1 byte a run. Good for flat color areas, pixel art |
| `--palette 256` | a shared palette of up to 256 colors, 2 bytes a run. flash: the palette is read straight from Flash; stream: it comes first with the header and the board keeps it in RAM taken at run time (warning) |
| `--palette 565` | each pixel its own RGB565 color, 3 bytes a run. Good for photos, gradients |
| `--colors N` | the palette uses only N colors (16 / 256) |
| `--dither` | dithering: smoother gradients, but shorter runs and more data |
| `--background` | the color transparent parts are filled with, default 000000 |
| `--mono` | black and white only (16-color mode, palette 0x0000, 0xFFFF), for the OLED. The file name gets an extra `_mono` |
| `--threshold N` | `--mono` without `--dither`: gray ≥ N is white (default 128) |
| `--invert` | `--mono`: black and white swapped |

### Data size

| Parameter | Explanation |
|---|---|
| `--max-bytes N` | when the data is over N it is lowered automatically until it fits; an error if it cannot get there |
| `--reduce fps` / `size` | whether `--max-bytes` lowers the frame rate or the size first (default size); when one side reaches its limit, it switches to the other |
| `--keyframe N` | flash: an I-frame every N frames (ignored for stream only: the computer sends differences directly) |
| `--tile N` | the size of the small squares used to find changed areas, default 8 |
| `--name` | your own file and struct name |
| `--board FILE` | the board's numbers: flash size and how constants are kept there (UNO: `PROGMEM`, `avr/pgmspace.h`), clock, drawing cycles…; by default `custom_board` of `platformio.ini` (UNO: `lib/Uno/board.json`). Not counted in `// options:`, recorded in `// board:` |

- **File names**: `{file name}_{flash|stream|flash_stream}_{width}x{height}_palette_{16|256|565}[_mono].h` (`_mono` with `--mono`), the struct named the same. Files outside `assets/` get their folders in front; equal names (e.g. differing only by `--keyframe`) get `_2`, `_3`, never replacing another (see [output/README.md](../output/README.md#names-and-sources)).

### Steps

1. **Reading**: animated pictures give every frame through Pillow; videos are thinned by fps with ffmpeg and shrunk first. Then cut by `--start`, `--duration`.
2. **Lowering the frame rate** (`--max-fps`): frames too close together are dropped, their time added to the frame before.
3. **Size**:
   - computes the size on the screen; `--fill` first cuts the excess off the middle.
   - divided by `--scale` gives the size to store, scaled with `--resample`, transparent parts filled with the background color.
4. **Colors**:
   - 565: each pixel straight to RGB565.
   - 16 / 256: one shared palette for all frames together by the octree method; `--dither` uses Floyd–Steinberg dithering, otherwise the nearest color.
5. **Encoding each frame**: frame 0 and the frames `--keyframe` picks are I-frames; the other frames compare three ways of drawing and take the cheapest:
   - **Changed windows**: the picture is cut into small squares of `--tile`, the changed squares are merged into rectangles, then shrunk to the area that really changed.
   - **One window**: the smallest rectangle around all changes.
   - **Redrawing the whole picture**: marked as an I-frame, so `FlashPlayer` can jump here when behind.
6. **Cost**: drawing time + data size × the cost of a byte, the values all in the board's `board.json` (the UNO's below):
   - about 480 cycles a window, about 44 cycles a screen pixel, about 30 cycles a run.
   - a byte: about 6 cycles reading Flash for flash; about 176 cycles over the serial for stream (1 Mbaud). So streaming leans towards ways with less data.
7. **`--max-bytes`**: when the data is too big, the frame rate or the size is multiplied by 0.85 according to `--reduce` and it is done again. The frame rate never goes below `--min-fps`, the size never below 4 px.

The format of the files made: see [output/README.md](../output/README.md).

### Errors and warnings

| Situation | Handling |
|---|---|
| the file does not exist, the extension is not supported, it cannot be read | error, no file made |
| a video without ffmpeg / ffprobe | error: `brew install ffmpeg` |
| no frames in the cut range | error |
| over 3000 frames | error: cut it shorter with `--start` / `--duration`, or lower `--max-fps` |
| over the board's flash (`flash.bytes`, UNO: 32256 bytes) with `--target flash` | error: `--max-bytes`, cut it shorter, a lower frame rate or size, or `--target stream` |
| beyond the hard limits of the format or the board (at most 240×240 on the screen; with scaling, a stored width ≤ 120; frame data offsets beyond 64 KB) | error, with a hint on which parameter to change |
| beyond the recommended ranges of `config.json` (data size as a share of the board's flash: `recommendedMaxFlashShare`) | warning |
| the board estimated not to reach the animation's fps, or below `--min-fps` | warning |

## check.py

| Kind | Item |
|---|---|
| error | `selected.h` missing or without an include guard |
| error | an included file missing, not in `output/`, including itself, included twice |
| error | the file's `// format:` is not the current `FORMAT_VERSION` |
| error | no `struct animation::{file name}` in the file |
| error | the source file differs from when it was made (sha256 compared) |
| error | no frame data in the file (those of `--target stream` are in `#if 0`) |
| error | the `// board:` of a file is not the board of `custom_board` in `platformio.ini` (or its numbers changed): `python src/clean.py --rebuild` |
| warning | the source file not found |
| warning | a .h in `output/` that is not selected |

The exit code is 1 when the check fails. `convert.py` runs it once after making a file.

## stream_handler.py

`stream.py` of [lib/stream](../../stream/README.md) (finds the board, lets go of the port while uploading, the caveat that opening the port resets the board, sends piece by piece with `'D'` / `'R'`) hands it the animation commands:

- **Safe**: opens only `.h` files inside `lib/video`; files of `--target stream` (frames in `#if 0`) and `--target flash` both work; the palette of 256 colors is in the header (color count 0 = 256).
- **`--tile`**: an option of `stream.py`, the grid size for finding changed areas (default 8).
- **Prints every 5 seconds**: how many frames a second were drawn, how many were skipped to keep up.

### The commands and answers of animations

The board decides which file to play and how; the computer keeps the time and sends the frames. How things are passed (`'D'` to notify, `'R'` to pull a piece, never more than 1 byte unasked from the computer): see [the protocol of lib/stream](../../stream/INTERFACE.md#協定).

**Board → computer: commands**

| Command | Arguments | The computer's answer |
|---|---|---|
| `'O'` open | CHUNK, flags (bit 0 play, bit 1 loop), speed (uint16 %), start position (uint32 ms), path length, path | `'G'` or `'E'` |
| `'N'` next frame please | — | `'F'` (when there is a new frame) or `'K'` (no new frame) |
| `'P'` play / `'S'` pause | — | `'A'` |
| `'J'` seek | position (uint32 ms) | `'A'` |
| `'V'` speed | percent (uint16, 1 ~ 1000) | `'A'` |
| `'L'` loop | 0 / 1 | `'A'` |

The first byte of an answer is its kind:

| Answer | Content |
|---|---|
| `'G'` header | state, width, height, scaleX, scaleY, color mode, color count, frame count (uint16), total length (uint32 ms), palette (16-color mode only) |
| `'F'` frame | state + one frame (format: see [output/README.md](../output/README.md#format-format-3)) |
| `'K'` no new frame | state. When paused, finished, or the next frame is still far off, at most once a second; sent at once when the state changes (e.g. finished). The board sends `'N'` again after it |
| `'A'` command done | state |
| `'E'` error | code (1 not found, 2 unreadable, 3 not streamable), message length, message |

**State** (7 bytes): flags (bit 0 playing, bit 1 finished), position (uint32 ms), frame number (uint16).

### Timing (the computer)

- **Position** = the start + (now − the time started) × speed. Paused, the position stays. Looping, it starts over past the total length; not looping, it stops at the end and the state becomes finished.
- **Which frame to send**: after an `'N'`, whenever "the frame due at the current position" differs from the one on the screen, it sends the difference between that frame and the one on the screen. Frames fallen behind are simply skipped, no I-frame needed.
- **Seeking, changing the speed**: only changes the clock; the next `'N'` naturally gets the frame of the new position.

### Error handling (the board)

- **Reconnecting**: in these cases the board sends `'O'` again, with the current position, play state, speed and loop setting, so it goes on where it was:
  - no answer at all within 3 seconds.
  - a piece of data not complete within 200 ms.
  - a command not answered within 1 second.
  - an answer of the wrong kind.
  - a window beyond the animation's area.
- **`'E'`**: remembers the error code and tries again after 3 seconds, as the file may be made a little later.
- **Races**: while the board sends a control command, the computer may just be sending a frame. The board finishes drawing that frame first, asks for the next frame, then keeps waiting for the command's `'A'`; the computer also waits until that frame has been pulled before answering the command.
