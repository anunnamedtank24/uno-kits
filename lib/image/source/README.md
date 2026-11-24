# image/source

English | [中文](讀我.md)

Tools that run on the computer. Needs Python 3, `pip install pillow` (streaming also needs `pyserial`). For HEIC, `pillow-heif` can be installed too; without it, macOS uses its built-in `sips`. All run inside `lib/image`.

## Files

| File | What it does |
|---|---|
| `config.json` | the parameter ranges and defaults of the two targets (flash, stream); the board's numbers (clock, flash, drawing cycles) are in its `board.json` (`--board`) |
| `convert.py` | picture → `output/*.h`, and adds the new one to `output/selected.h` |
| `check.py` | checks every file of `output/`: the current format, an unchanged source (`convert.py` runs it after every conversion) |
| `stream_handler.py` | used by `stream.py` of [lib/stream](../../stream/README.md) to read pictures and answer the board's `'I'`; not run on its own |

## Usage

```bash
cd lib/image
python source/convert.py assets/sunset.jpg output --target flash --fit 120x80 --scale 2 --palette 16
python source/check.py
cd ../.. && python lib/stream/source/stream.py     # streaming: it serves pictures, animations and sounds
```

## convert.py

### Files supported

| Extension | Read by |
|---|---|
| `.jpg`, `.jpeg`, `.png`, `.webp` | Pillow |
| `.heic`, `.heif` | `pillow-heif` (if installed); otherwise macOS `sips` first turns it into a temporary png, deleted afterwards |

- **EXIF orientation**: photos are turned upright first, the same way as in the photo album.
- **Transparency**: alpha-blended onto `--background` (default 000000); half-transparent edges are blended too, not just cut.
- **Other extensions**: an error.

### Targets

`--target` takes one or two; left out it is `flash`:

| | `--target flash` | `--target stream` | `--target flash stream` |
|---|---|---|---|
| where the pixels are | `output/x.h`, burned into Flash | in the `#if 0` of `output/x.h`, staying on the computer | `output/x.h` |
| what `output/x.h` has | pixels + path | only size, colors, path | pixels + path |
| colors (`--palette`) | 2, 16 (default), 256, 565 (warning) | 2, 16, 256 (warning: the palette is sent first, the board takes RAM at run time), 565 (default) | 2, 16 (default), 256, 565 (warning) |
| ranges | flash's | stream's | what both allow (the intersection) |

### Size (resolution)

Every size means the size "on the screen", that is, after scaling up. The same as `lib/video/source/convert.py`:

| Parameter | Explanation |
|---|---|
| (none given) | the picture's own size (the stored width and height must both be ≤ 240; photos usually need a size) |
| `--width W` or `--height H` | only one: the aspect ratio is kept |
| `--width W --height H` | both: exactly this size (stretched) |
| `--resize F` / `--resize FXxFY` | a multiple of the original size, decimals allowed, e.g. `0.1` |
| `--fit WxH` | keeps the aspect ratio, as big as fits in a W×H box |
| `--fill WxH` | keeps the aspect ratio, covers W×H, cutting off the excess from the middle |
| `--scale N` / `--scale NXxNY` | the board draws each point NX×NY times (1 ~ 8). The data stores only "screen size ÷ scale": resolution traded for data size |
| `--resample` | the scaling method: `nearest` (pixel art), `bilinear`, `bicubic`, `lanczos` (default) |

- Over 240×240 on the screen is only a warning: the picture is clipped, but the extra data is wasted.
- A size that is not a whole multiple of scale is rounded, with a warning giving the real size.

### Colors

| Parameter | Explanation |
|---|---|
| `--palette 2` | black and white, 1 bit. Gray ≥ `--threshold` (default 128) is white; `--dither` uses Floyd–Steinberg dithering; `--invert` swaps black and white |
| `--palette 16` | a palette of up to 16 colors (median cut), 4 bits |
| `--palette 256` | a palette of up to 256 colors, 1 byte. flash: the palette is read straight from Flash; stream: the palette is sent first and the board keeps it in RAM taken at run time (warning) |
| `--palette 565` | each pixel its own RGB565 color, 2 bytes |
| `--colors N` | the palette uses only N colors (16 / 256) |
| `--dither` | dithering (2 / 16 / 256): smoother gradients, but shorter runs |
| `--background` | the color transparent parts are put on, default 000000 |

- The palette keeps only the colors really used, renumbered 0 ~ n − 1.

### Encoding and size

| Parameter | Explanation |
|---|---|
| `--encoding auto` | computes both raw and runs and uses the smaller (default; raw when equal) |
| `--encoding raw` | each pixel as it is (1 / 4 / 8 / 16 bits) |
| `--encoding runs` | a stretch of the same color stored as one entry |
| `--max-bytes N` | when the data is over N, the size is multiplied by 0.85 and it is done again until it fits; still too big at 4 px, an error |
| `--name` | your own file and struct name |
| `--board FILE` | the board's numbers: flash size and how constants are kept there (UNO: `PROGMEM`, `avr/pgmspace.h`), clock, drawing cycles…; by default `custom_board` of `platformio.ini` (UNO: `lib/Uno/board.json`). Not counted in `// options:`, recorded in `// board:` |

- **File names**: `{file name}_{flash|stream|flash_stream}_{width}x{height}_palette_{2|16|256|565}.h`, the struct named the same. Files outside `assets/` get their folders in front; equal names (e.g. differing only by `--dither`) get `_2`, `_3`, never replacing another (see [output/README.md](../output/README.md#names-and-sources)).

### Steps

1. **Reading**: read by extension, turned upright (EXIF), made RGBA.
2. **Size**:
   - computes the size on the screen; `--fill` first cuts the excess off the middle.
   - divided by `--scale` gives the size to store; scaled with `--resample`, then blended onto the background color.
3. **Colors**:
   - 565: each pixel straight to RGB565.
   - 2: to gray, then black and white by the threshold or dithering.
   - 16 / 256: a palette by median cut, with dithering if asked.
4. **Encoding**: raw and runs, chosen by `--encoding`.
5. **Estimated time**: about 900 cycles a segment (`SEGMENT` = 32 screen pixels), about 60 cycles a screen pixel, reading a byte takes 6 cycles from flash and 176 cycles from the stream (1 Mbaud); the values are in the board's `board.json` (the UNO's; stream computed from the clock and the baud rate).

The format of the files made: see [output/README.md](../output/README.md).

### Errors and warnings

| Situation | Handling |
|---|---|
| the file does not exist, the extension is not supported, it cannot be read | error, no file made |
| beyond the hard limits of the format or the board (stored width and height ≤ 240, scale ≤ 8) | error, with a hint on which parameter to change |
| decoding the encoded data with the Python decoder does not give it back | error (a bug of the program), no file made |
| `--max-bytes` too small even at 4 px | error |
| over the board's flash (`flash.bytes`, UNO: 32256 bytes) with `--target flash` | error: `--max-bytes`, a smaller size or palette, or `--target stream` |
| over 240×240 on the screen | warning: it is clipped, the extra data is wasted |
| beyond the recommended ranges of `config.json` (data size as a share of the board's flash: `recommendedMaxFlashShare`) | warning |

## check.py

The same rules as `lib/video/source/check.py`, only with the namespace `picture` and looking at `getPixels()`:

| Kind | Item |
|---|---|
| error | `selected.h` missing or without an include guard |
| error | an included file missing, not in `output/`, including itself, included twice |
| error | the file's `// format:` is not the current `FORMAT_VERSION` (1) |
| error | no `struct picture::{file name}` in the file |
| error | the source picture differs from when it was made (sha256 compared) |
| error | no pixels in the file (those of `--target stream` are in `#if 0`) |
| error | the `// board:` of a file is not the board of `custom_board` in `platformio.ini` (or its numbers changed): `python src/clean.py --rebuild` |
| warning | the source picture not found |
| warning | a .h in `output/` that is not selected |

## stream_handler.py

`stream.py` of [lib/stream](../../stream/README.md) (finds the board, lets go of the port while uploading, sends piece by piece with `'D'` / `'R'`) hands it each `'I'`:

- **Safe**: opens only `.h` files inside `lib/image`; files of `--target stream` (pixels in `#if 0`) and `--target flash` both work.
- **Checks the file**: the pixels are decoded once when read, so a broken file is answered with an error, never sent halfway.
- **Prints each picture**: how many bytes were sent, how many seconds it took.

### The answer of a picture

| Direction | Content |
|---|---|
| board → computer | `'I'`, CHUNK, path length, path |
| computer → board | `'D'`, then the board pulls piece by piece with `'R'` ([the protocol of lib/stream](../../stream/INTERFACE.md#協定)) |

| Answer | Content |
|---|---|
| `'H'` picture | width, height, scaleX, scaleY, color mode, encoding, color count (0 = 256), data length (uint32), palette (color count × RGB565; none for 565), pixels |
| `'E'` error | code (1 not found, 2 unreadable, 3 not streamable), message length, message |

The board draws while receiving, so it needs a buffer of only CHUNK.

| Situation | What the board does |
|---|---|
| no `'D'` within 3 seconds | `ERROR_NO_ANSWER` |
| a piece not complete within 200 ms | the remaining pixels are drawn black, `ERROR_BROKEN` |
| a header that makes no sense | `ERROR_BROKEN` |
