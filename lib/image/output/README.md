# image/output

English | [中文](讀我.md)

The pictures made by `source/convert.py` and by macros in programs, `namespace picture`. Never edit the files here by hand: to change one, convert it again.

## Files

```
output/
├── selected.h    written automatically: lists every .h here, image.h includes it
├── registry.h    written automatically: the types of PICTURE("path", "options") in the current program
└── *.h           the converted pictures: made by convert.py by hand or by PICTURE(...), all here, in the same format
```

## selected.h, registry.h

- Both are written by `src/tools/assets.py` before every build (`convert.py` also adds a new one to `selected.h`); nothing to edit by hand.
- `selected.h` lists **every** one: the unused ones take no Flash (the data is in arrays inside `inline` functions, with the board's flash attribute (`board.json`; UNO: `PROGMEM`); never called, never generated; measured: the same Flash).
- `registry.h` has only the `PICTURE(...)` of the current program.
- `python source/check.py` checks that every file has the current format and an unchanged source; `convert.py` checks once after every conversion.

## The current pictures

| File | Source | On the screen | Colors | Encoding | Data |
|---|---|---|---|---|---|
| `sunset_flash_120x80_palette_16.h` | sunset.jpg | 120×80 (stored 60×40, scale 2) | 16 (15 used) | runs | 319 B |
| `sunset_flash_80x53_palette_256.h` | sunset.webp | 80×53 | 256 | raw | 4744 B |
| `sunset_flash_60x40_palette_565.h` | sunset.png | 60×40 | 565 | runs | 1629 B |
| `sunset_stream_240x240_palette_565.h` | sunset.heic | 240×240 (`--fill`) | 565 | runs | 19137 B (in `#if 0`) |
| `logo_flash_stream_128x64_palette_2.h` | logo.png (transparent) | 128×64 | 2 | runs | 357 B |
| `sunset_flash_stream_96x64_palette_2.h` | sunset.jpg (`--dither`) | 96×64 | 2 | raw | 772 B |
| `sunset_flash_128x64_palette_2.h` | sunset.heic (`--fill`, `--dither`) | 128×64 | 2 | raw | 1028 B |
| `moon_flash_128x64_palette_2.h` | moon.jpg (`--fill`, `--dither`) | 128×64 | 2 | raw | 1028 B |
| `city_flash_128x64_palette_2.h` | city.png (`--fill`, `--threshold 100`) | 128×64 | 2 | raw | 1028 B |
| `treble_clef_flash_15x40_palette_2.h` | treble_clef.png | 15×40 | 2 | raw | 75 B |
| `moon_flash_64x32_palette_2.h` | moon.jpg (`PICTURE(...)`, `src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp`) | 64×32 | 2, dither | raw | 260 B |

The pictures of `assets/` were drawn with Pillow: a gradient sky, a sun, three layers of mountains (sunset), a starry sky and the moon (moon), buildings with lit windows (city), and an icon on a transparent background (logo). The last three 128×64 ones are the photos of assignments 1 and 3 of `src/week_4_SSD1306_switch_buzzer_audio`. treble_clef is the treble clef: 𝄞 (U+1D11E) of the macOS font Apple Symbols drawn by Pillow as a black-and-white picture 40 px high, for the staff of `src/week_5_NEC/assignments/3_staff` (the center of the curl is on row 25, lined up with the G4 line). sunset is also saved as jpg and webp, and turned into heic with `sips`, so all four formats are tested.

## The generated struct

```cpp
namespace picture {
    struct sunset_flash_120x80_palette_16 {
        static constexpr uint8_t target;        // bit flags: image::TARGET_FLASH = 1, TARGET_STREAM = 2, 3 for both
        static constexpr uint8_t width, height; // the stored size (before scaling), 1 ~ 240
        static constexpr uint8_t scaleX, scaleY;
        static constexpr uint8_t colorMode;     // image::COLORS_2 = 0 / COLORS_16 = 1 / COLORS_256 = 2 / COLORS_565 = 3
        static constexpr uint8_t encoding;      // image::ENCODING_RAW = 0 / ENCODING_RUNS = 1
        static constexpr uint16_t colorCount;   // the palette's colors, 0 for 565
        static constexpr uint32_t dataSize;     // bytes of pixel data
        static inline const char *getPath();    // the path of the pixel file (relative to lib/image), which StreamImage passes to stream.py

        // compiled only in files with pixels (--target flash, flash stream); for --target stream they are in #if 0:
        static inline const uint16_t *getPalette();   // colorCount RGB565 colors
        static inline const uint8_t *getPixels();     // pixels, format below
    };
}
```

- **`--target stream`** also has just one file `output/x.h`: first the upper part the board compiles (size, `getPath()`), then the pixels in `#if 0` (too big to burn into Flash, so never compiled), which `stream.py` reads from the same file. The `stream` in the file name and the `target` of the struct both say it is for streaming.
- **The start of a file**:
  - `// options: …`: the arguments when converted (`src/tools/assets.py` finds converted ones by it and `source:`).
  - `// format: 1`: the format version.
  - `// board: Arduino UNO (ATmega328P, 16 MHz) sha256 …`: the board it was converted for (`custom_board` of `platformio.ini`; when the hash of the numbers differs, `check.py` errors and `src/tools/assets.py` converts it again before compiling).
  - `// source: … sha256 …`: the source picture and its hash.
  - `// settings:`: every setting, including the original command, the cut, the real size, colors, encoding, data size and the estimated time.

## Format (format 1)

`decoder.h` decodes, `convert.py` encodes; `convert.py` also has a Python decoder (`get_decoded_values`), used for its self-check after converting and for `stream.py` checking files. Flash and streaming share this format.

The pixels go row by row, width × height of them, each drawn as scaleX × scaleY on the screen.

**raw**: each pixel as it is, the bits continuing across rows (no padding at the end of a row)

| Color mode | Each pixel |
|---|---|
| `COLORS_2` | 1 bit, the first pixel in bit 7 |
| `COLORS_16` | 4 bits, the first pixel in the high 4 bits |
| `COLORS_256` | 1 byte (the palette number) |
| `COLORS_565` | 2 bytes, little endian |

**runs**: a stretch of pixels of the same color, may cross rows

| Color mode | A run |
|---|---|
| `COLORS_2` | 1 byte: bit 7 = the color number, bits 0 ~ 6 = n − 1 (1 ~ 128 pixels) |
| `COLORS_16` | 1 byte: high 4 bits = n, low 4 bits = the color number; when n = 0, the next byte + 16 pixels (16 ~ 271) |
| `COLORS_256` | 2 bytes: [n − 1][the color number] (1 ~ 256) |
| `COLORS_565` | 3 bytes: [n − 1][RGB565 low][RGB565 high] (1 ~ 256) |

The runs of `COLORS_16`, `COLORS_256`, `COLORS_565` are the same as video's format (format 3).

## Names and sources

The start of every .h says which file and which options it was converted from:

```
// options: --fit 120x80 --palette 16
// source: assets/sunset.jpg sha256 1413…
```

`PICTURE("path", "options")` in a program finds a converted file by these two lines: the same file with the same options (the order and the form `--fit=…` do not matter) is used as it is, only otherwise is it converted. So converted by hand or by a macro, each is converted only once.

| Rule | Explanation |
|---|---|
| Names | the rule of `convert.py`: `{file name}_{settings}`, the struct named the same (`picture::name`) |
| Files outside `assets/` | the folders go in front of the file name (relative to the project; outside the project, to the home folder), e.g. `~/Music/Game/title.mp3` → `music_game_title_…`, so files with the same name in different folders never mix |
| Characters that are not letters or digits (e.g. Japanese) | left out, with a 6-digit hash added instead, e.g. `16 戦い：ウォロ.aiff` → `…_16_7b5dc7_…` |
| Equal names (the name does not hold every option, or another source) | `_2`, `_3`… added, never replacing another file |
| The source changed | a warning when compiling; `python src/tools/assets.py --force` converts it again (replacing the one with the same name) |
| The source moved or deleted | it still compiles as long as the output is there (the old path in the program still finds it) |
| Unwanted ones | `python src/tools/assets.py --unused` lists those no program (`.bak` included) uses; delete them yourself |
