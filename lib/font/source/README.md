# font/source

English | [中文](讀我.md)

Tools that run on the computer. Needs Python 3, `pip install pillow`. All run inside `lib/font`.

## Files

| File | What it does |
|---|---|
| `convert.py` | font → `output/*.h`, and adds the new one to `output/selected.h` |
| `check.py` | checks every file of `output/`: the current format, an unchanged source (`convert.py` runs it after every conversion) |

## Usage

```bash
cd lib/font
python source/convert.py assets/menlo.ttf output --height 23
python source/convert.py assets/menlo.ttf output --height 46 --first 48 --last 57
python source/check.py
```

## convert.py

### Files supported

| Extension | Read by |
|---|---|
| `.ttf`, `.otf` | Pillow (FreeType) |

### Parameters

| Parameter | Default | Explanation |
|---|---|---|
| `--height` / `--width` / `--size` | `--height 16` | one at most: the grid height, the width of the widest character, or FreeType's font size |
| `--first` / `--last` | 32 / 126 | the range of character codes |
| `--default` | `?` | the character drawn for characters outside the range |
| `--name` | `{font}_{parameter}_{value}_first_{f}_last_{l}_default_{code}` (see the file names below) | the file and struct name |
| `--board FILE` | `custom_board` of `platformio.ini` (UNO: `lib/Uno/board.json`) | the board's numbers: flash size and how constants are kept there (UNO: `PROGMEM`, `avr/pgmspace.h`), clock, drawing cycles… Not counted in `// options:`, recorded in `// board:` |

- **File names**: `{font}_{parameter}_{value}_first_{f}_last_{l}_default_{code}.h`, the struct named the same. Files outside `assets/` get their folders in front; equal names get `_2`, `_3`, never replacing another (see [output/README.md](../output/README.md#names-and-sources)).

### Steps

Each character is stored as a set of shapes (points, horizontal lines, vertical lines, rectangles). The shapes cover exactly the character's pixels; they may overlap, but never draw outside the character. The cheapest set is found by drawing time:

1. find every maximal rectangle (cannot grow up, down, left or right).
2. find the cheapest set by branch and bound.
3. shrink the overlaps as much as possible.
4. sort them by size into points, horizontal lines, vertical lines, rectangles.

**Drawing time** (the board's `board.json`, estimated for the UNO): about 480 cycles a shape, about 44 cycles a pixel, about 6 cycles a stored byte. Compared with drawing point by point, small text takes only about 30% of the time, large text about 21%.

### Errors and warnings

| Situation | Handling |
|---|---|
| the font file does not exist, the extension is not supported | error, no file made |
| a range or size beyond the format's limits (see [README.md](../README.md#input-limits)) | error, no file made |
| the default character is not in the font | error |

## check.py

| Kind | Item |
|---|---|
| error | `selected.h` missing or without an include guard |
| error | an included file missing, not in `output/`, including itself, included twice |
| error | a file made by an old `convert.py` (no `// format:`, or not the current `FORMAT_VERSION`) |
| error | no `struct font::{file name}` in the file (e.g. it was renamed) |
| error | the source font differs from when it was made (sha256 compared) |
| error | the `// board:` of a file is not the board of `custom_board` in `platformio.ini` (or its numbers changed): `python src/clean.py --rebuild` |
| warning | the source font not found (cannot tell if it changed) |
| warning | a .h in `output/` that is not selected |

The exit code is 1 when the check fails.
