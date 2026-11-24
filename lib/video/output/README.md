# video/output

English | [中文](讀我.md)

The animations made by `source/convert.py` and by macros in programs, `namespace animation`. Never edit the files here by hand: to change one, convert it again.

## Files

```
output/
├── selected.h    written automatically: lists every .h here, video.h includes it
├── registry.h    written automatically: the types of ANIMATION("path", "options") in the current program
└── *.h           the converted animations: made by convert.py by hand or by ANIMATION(...), all here, in the same format
```

## selected.h, registry.h

- Both are written by `src/tools/assets.py` before every build (`convert.py` also adds a new one to `selected.h`); nothing to edit by hand.
- `selected.h` lists **every** one: the unused ones take no Flash (the data is in arrays inside `inline` functions, with the board's flash attribute (`board.json`; UNO: `PROGMEM`); never called, never generated; measured: the same Flash).
- `registry.h` has only the `ANIMATION(...)` of the current program.
- `python source/check.py` checks that every file has the current format and an unchanged source; `convert.py` checks once after every conversion.

## The current animations

All converted from `assets/orbit.gif` (120×120, 24 frames, 1.44 seconds, 16.7 fps):

| File | Command | Data | A frame (estimated) |
|---|---|---|---|
| `orbit_flash_120x120_palette_16.h` | `--target flash --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8` | 1992 B | 1.4 ~ 40.2 ms |
| `orbit_flash_64x64_palette_16_mono.h` | `--target flash --fit 64x64 --mono --keyframe 8` (OLED) | 882 B | 0.4 ~ 11.4 ms |
| `orbit_flash_stream_120x120_palette_16.h` | `--target stream flash --width 120 --scale 2` | 2352 B | 1.8 ~ 40.4 ms |
| `orbit_stream_120x120_palette_565.h` | `--target stream --fit 120x120 --palette 565` | 4658 B (in `#if 0`) | 2.4 ~ 54.0 ms (with sending) |
| `orbit_flash_64x64_palette_16_mono_2.h` | `ANIMATION("orbit.gif", "--width 64 --mono")` (`src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp`); differs from the mono one above only by `--keyframe`, so `_2` is added | 753 B | 0.4 ~ 11.4 ms |

## The generated struct

```cpp
namespace animation {
    struct orbit_flash_120x120_palette_16 {
        static constexpr uint8_t target;       // bit flags: video::TARGET_FLASH = 1, TARGET_STREAM = 2, 3 for both
        static constexpr uint8_t width, height; // the stored size (before scaling)
        static constexpr uint8_t scaleX, scaleY;
        static constexpr uint8_t colorMode;    // video::COLORS_16 = 0 / COLORS_256 = 1 / COLORS_565 = 2
        static constexpr uint16_t colorCount;  // the palette's colors, 0 for 565
        static constexpr uint16_t frameCount;
        static constexpr uint32_t durationMs;  // the total length of one round
        static inline const char *getPath();   // the path of the frame data file (relative to lib/video), which StreamPlayer passes to stream.py

        // compiled only in files with frame data (--target flash, flash stream); for --target stream they are in #if 0:
        static inline const uint16_t *getPalette();   // colorCount RGB565 colors
        static inline const uint16_t *getOffsets();   // where each frame starts in getFrames() (uint32_t for streams over 64 KB)
        static inline const uint8_t *getFrames();     // all frames, format below
    };
}
```

- **`--target stream`** also has just one file `output/x.h`: first the upper part the board compiles (size, `getPath()`), then the frame data in `#if 0` (too big to burn into Flash, so never compiled), which `stream.py` reads from the same file. The `stream` in the file name and the `target` of the struct both say it is for streaming.
- **The start of a file**:
  - `// options: …`: the arguments when converted (`src/tools/assets.py` finds converted ones by it and `source:`).
  - `// format: 3`: the format version. When the format changes, raise `FORMAT_VERSION` in `check.py` so that old files are caught.
  - `// board: Arduino UNO (ATmega328P, 16 MHz) sha256 …`: the board it was converted for (`custom_board` of `platformio.ini`; when the hash of the numbers differs, `check.py` errors and `src/tools/assets.py` converts it again before compiling).
  - `// source: … sha256 …`: the source file and its hash.
  - `// settings:`: every setting, including the original command, the cut, the real size, colors, frame rate, data size and the estimated fps, so nothing has to be guessed from the file name.

## Format (format 3)

`decoder.h` decodes, `convert.py` encodes; `convert.py` also has a Python decoder for `stream.py`. Flash and streaming share this format. Every multi-byte value is little endian.

```
frame:   [delay ms low][delay ms high][type][window count]
         type: 0 = P-frame, 1 = I-frame
window:  [x][y][w][h]   animation coordinates (before scaling), then runs until w×h pixels are filled
run:     depends on the color mode
  COLORS_16   1 byte: high 4 bits = n, low 4 bits = the palette number
              n = 1~15: n pixels; n = 0: the next byte + 16 pixels (16~271)
  COLORS_256  2 bytes: [n][palette number], n + 1 pixels (1~256)
  COLORS_565  3 bytes: [n][RGB565 low][RGB565 high], n + 1 pixels (1~256)
```

- The pixels of a window go row by row. A run may cross rows, but never windows.
- **I-frame**: one window covering the whole animation, independent of the frame before.
- **P-frame**: only the changed windows; the rest of the screen keeps the frame before.
- **Empty frame**: delay 0, type P, 0 windows.
- **Scaling**: each point is drawn as scaleX×scaleY. With scaling, the board first decodes a row into colors (at most 120), then sends it repeatedly.

## Names and sources

The start of every .h says which file and which options it was converted from:

```
// options: --fit 120x80 --palette 16
// source: assets/sunset.jpg sha256 1413…
```

`ANIMATION("path", "options")` in a program finds a converted file by these two lines: the same file with the same options (the order and the form `--fit=…` do not matter) is used as it is, only otherwise is it converted. So converted by hand or by a macro, each is converted only once.

| Rule | Explanation |
|---|---|
| Names | the rule of `convert.py`: `{file name}_{settings}`, the struct named the same (`animation::name`) |
| Files outside `assets/` | the folders go in front of the file name (relative to the project; outside the project, to the home folder), e.g. `~/Music/Game/title.mp3` → `music_game_title_…`, so files with the same name in different folders never mix |
| Characters that are not letters or digits (e.g. Japanese) | left out, with a 6-digit hash added instead, e.g. `16 戦い：ウォロ.aiff` → `…_16_7b5dc7_…` |
| Equal names (the name does not hold every option, or another source) | `_2`, `_3`… added, never replacing another file |
| The source changed | a warning when compiling; `python src/tools/assets.py --force` converts it again (replacing the one with the same name) |
| The source moved or deleted | it still compiles as long as the output is there (the old path in the program still finds it) |
| Unwanted ones | `python src/tools/assets.py --unused` lists those no program (`.bak` included) uses; delete them yourself |
