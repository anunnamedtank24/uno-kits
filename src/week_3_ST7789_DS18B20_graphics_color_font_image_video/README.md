# week_3_ST7789_DS18B20_graphics_color_font_image_video

English | [中文](讀我.md)

The full tests of the M130T (1.3 inch 240×240 ST7789), the DS18B20 temperature sensor, color pictures, and three assignments.

Only one file in `src/` may have `setup()` / `loop()` at a time; the others must be renamed to `.bak`. `hide.py` keeps only the one to run:

```bash
python src/hide.py week_3_ST7789_DS18B20_graphics_color_font_image_video/assignments/2_ripples     # all others become .bak, assignments/2_ripples stays (or drops its .bak)
python src/hide.py 'w*3*as*1'              # "*" matches in parts, needs quotes
```

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_screen.cpp` | the full test of the screen, shapes, text, animations | M130T |
| `tests/2_temperature.cpp` | DS18B20 temperature as text on the serial every `PERIOD_MS` | DS18B20 |
| `tests/3_pictures.cpp` | color pictures ([lib/image](../../lib/image/README.md)): 16 / 256 / 565 / 2 colors, clipping, 240×240 full color streamed from the computer | M130T |
| `assignments/1_temperature_display.cpp` | assignment 1: temperature, raw value and frame rate on the screen, changes shown by color | M130T + DS18B20 |
| `assignments/2_ripples.cpp` | assignment 2: ripples (an artwork), the rain changing randomly | M130T |
| `assignments/3_rays.cpp` | assignment 3: two rays, rotation and shaking in a parameter space | M130T |

## Wiring

UNO + M130T (1.3 inch 240×240, no CS), the same for every file:

| M130T | UNO |
|---|---|
| SCL | D13 |
| SDA | D11 |
| DC | `DC_PIN` |
| RES | `RES_PIN` |
| BLK | `BLK_PIN` |
| VCC / GND | 3.3V / GND |

DS18B20 (`tests/2_temperature`, `assignments/1_temperature_display`):

| DS18B20 | UNO |
|---|---|
| VCC | 5V |
| DQ | `DQ_PIN` (needs a 4.7 kΩ pull-up to VCC: module boards like the KY-001 have it already; a bare TO-92 and the waterproof probe need one added, and the program then uses `DS18B20::TO92`, `DS18B20::WaterproofProbe`) |
| GND | GND |

# tests/1_screen.cpp: screen, shapes, text, video

Each test draws one picture, waits `HOLD_MS`, clears the screen and runs the next, forever. The comment above each test says "what you should see".

## Tests

| `tests[i]` | Function | What it tests |
|---|---|---|
| 0 | `testColors` | initialization, RGB/BGR order, inversion: red → green → blue → white → black → yellow |
| 1 | `testBorder` | size and offset: a white frame touching all four edges, the corners red, green, blue, yellow |
| 2 | `testRamRect` | `fillWindow`, `writeWindow`, including blocks right against the left, right and bottom edges |
| 3 | `testPoint` | points: one every 10 px |
| 4 | `testHVLines` | three ways of writing horizontal and vertical lines; the three of each group must match |
| 5 | `testLines` | three ways of writing slanted lines: a fan, a star, an X |
| 6 | `testRects` | three ways of writing rectangles, outlined and filled |
| 7 | `testTriangles` | three ways of writing triangles; the outlined ones must lie exactly on the edges of the filled ones |
| 8 | `testCircles` | two ways of writing circles |
| 9 | `testBorders` | outlined border widths 1, 6, 40: colored borders over gray filled shapes of the same size, with no gray showing outside |
| 10 | `testClip` | shapes right at the screen's edges (0 and 239): nothing may be cut off, and nothing may wrap to the other side |
| 11 | `testTextChars` | every character of the font, with no gaps and no overlaps |
| 12 | `testTextTypes` | strings, `F()`, characters, 8 ~ 32 bit integers |
| 13 | `testTextFloat` | floating point, `setDecimals`, nan, inf |
| 14 | `testTextArrays` | arrays, `setSeparator` |
| 15 | `testTextDefault` | characters outside the font's range drawn as `?` |
| 16 | `testTextLayout` | transparent background, the returned x, `fontHeight`, `F()`, text right at the edges |
| 17 | `testTextLineBreak` | `setAutoLineBreak`, `setLineSpacing` |
| 18 | `testVideoFlash` | `video::FlashPlayer`: plays `VIDEO_MS`, then stops on purpose for `LATE_MS` to see it jump to an I-frame and catch up |
| 19 | `testVideoStream` | `video::StreamPlayer`: play, pause, seek, double speed, looping off until the end |
| 20 | `testOffScreen` | shapes and text partly off the screen: only the part on the screen shows, nothing wraps to the other side |

## Running only some tests

- **`ONLY_TESTS` in the settings**: 0 = all in turn; to run only some, set their bits, e.g. `(1UL << 18)` runs only `tests[18]`, `(1UL << 14) | (1UL << 15)` runs two.
- `tests[19]` needs the computer side: without `stream.py` it waits `STREAM_WAIT_MS`, showing waiting for stream.py.

## tests[19]: streaming

Needs `stream.py` on the computer, details in [lib/stream/README.md](../../lib/stream/README.md):

```bash
python lib/stream/source/stream.py      # pip install pillow pyserial; close the serial monitor first
```

- `UnoSerial` uses D0 / D1, so this file cannot use Arduino's `Serial`.
- Opening the port resets the board: better start `stream.py` first, then upload; started later, the program starts over (see [lib/stream](../../lib/stream/README.md#streampy)).
- The board waits at most `STREAM_WAIT_MS`; the bottom of the screen shows the current step, position, frame number and state.

## Why the headers of lib/ are found

These files used to be in `src/test/`, which PlatformIO's dependency finder skips, so `platformio.ini` lists the folders of `lib/` directly in `lib_deps`, and a program in any subfolder of `src/` compiles.

# tests/2_temperature.cpp: DS18B20 temperature on the serial

## Seeing the result

```bash
pio device monitor        # platformio.ini already sets monitor_speed (= BAUD)
```

At power-on it prints the resolution once, then a line every `PERIOD_MS`:

```
DS18B20 on D2
resolution 12 bits, conversion 750 ms
23.50 C (raw 376, 751 ms)
```

- In parentheses: the raw value (1/16 °C a step) and how long this measurement took.
- `85.00` gets a note: it is also the value of a sensor just powered on, before any conversion.
- Errors print the reason, e.g. `[ERROR] no sensor (check DQ on D2, the pull-up, VCC / GND)`.
- The serial uses `Uno::UnoSerial<BAUD>` (`BAUD` of the settings, the same as `monitor_speed` in platformio.ini), so Arduino's `Serial` cannot be used at the same time; opening the monitor resets the board, and the first two lines are printed again.

# tests/3_pictures.cpp: color pictures

Color pictures on the M130T. The pictures are all in `lib/image/output/`, the source files in `lib/image/assets/`.

The wiring is as at the top (M130T). The OLED also uses D13 / D11: the two cannot be connected at the same time.

## Tests

| `tests[i]` | Function | Picture | What you should see |
|---|---|---|---|
| 0 | `testPalette16` | `sunset_flash_120x80_palette_16` (jpg, scale 2) | a sunset: a purple-to-orange sky, a yellow sun, three layers of dark mountains, with clear color bands; below, how many ms the drawing took |
| 1 | `testPalette256` | `sunset_flash_80x53_palette_256` (webp) | the same sunset, smaller, the sky smoother than in 0 |
| 2 | `testPalette565` | `sunset_flash_60x40_palette_565` (png) | the same sunset, smaller still, full color |
| 3 | `testPalette2` | logo (transparent png), a dithered sunset | a white logo above, a dotted black-and-white sunset below |
| 4 | `testClip` | the same picture in the four corners, half of each off the screen | a piece of the sunset in each corner, nothing wrapping to the other side |
| 5 | `testStream` | `sunset_stream_240x240_palette_565` (heic, `--fill`) | with `stream.py` running: the whole screen is the sunset (about 2 seconds to draw), the ms at the top left; without it: red text `error 4` |

- This file has only been checked to compile (the M130T was not connected at the time).
- The decoder was verified on the computer with a fake screen: 8 combinations of "colors × encoding", scaling, clipping, all 34,838 pixels right.

# assignments/1_temperature_display.cpp: temperature display

The DS18B20's temperature and raw value and the screen's frame rate, redrawn every frame.

```
Temp: 23.5 \circ C       (DQ unplugged: Temp: no sensor)
Raw: 376                (DQ unplugged: Raw: no sensor)
FPS: 85.3
```

- **`\circ` is 5 characters**: the font has only ASCII, no °, so it is written literally.
- **Gone the moment it is unplugged**: no waiting for the conversion (`conversionMs()`); every frame asks whether the conversion is done; after unplugging, the next read is `ERROR_NO_SENSOR`, and the same frame shows `no sensor`.
- **Only the numbers change color**: the labels and units are a fixed gray (`LABEL_LUMA`).

## Colors

| State | Color |
|---|---|
| the same as the last value | grayish white |
| rising | mixed towards red |
| falling | mixed towards blue |

- **Doubling**: the change ÷ the unit = steps; the share of red (blue) doubles each step: 1/16, 1/8, 1/4, 1/2, all, so 5 steps reach the end.
- **The same brightness**: grayish white, red and blue at the ends are all computed to the same brightness `VALUE_LUMA` (0.299 R + 0.587 G + 0.114 B), so the mixes in between are equally bright and only the hue changes.
- **Temp and Raw change color together**: both use the same change of raw. Temp shows only one decimal while raw steps by 0.0625 °C, so the color may change while the Temp number does not.
- **The values line up by themselves**: the labels are drawn in `setup()`, and the largest end position returned by `drawTexts` is used, so another font lines up too.

| Setting | Explanation |
|---|---|
| `TEMP_UNIT` | the size of one temperature step; decimals allowed (multiplied by 1000 into an integer when compiling, no floating point at run time) |
| `FPS_UNIT` | the size of one frame-rate step |
| `VALUE_LUMA` | the brightness of the numbers; must be above 0.299 (pure red is at most this bright), otherwise a compile error |
| `LABEL_LUMA` | the brightness of the labels and units |

# assignments/2_ripples.cpp: ripples

Drops fall at random onto black water; each sends out one or more rings of ripples (`RING_COUNTS`) that spread outward, fade and pass through each other. No input needed: it plays by itself forever. `No. {n}` at the bottom left is the number of the drop.

## The rain: a random walk + a sigmoid

```
walk moves −WALK_STEP ~ +WALK_STEP every WALK_MS, bouncing back at ±WALK_LIMIT
rain = 1 / (1 + e^(−walk / SIGMOID_SCALE))      0 = gentle, 1 = heavy rain
```

A sigmoid is flat at both ends and steep in the middle, so most of the time it is a steady "gentle" or "heavy rain", switching within a few seconds.

| | Gentle (rain = 0) | Heavy rain (rain = 1) |
|---|---|---|
| time between drops | `CALM_DROP_MIN_MS` ~ `CALM_DROP_MAX_MS` | `POUR_DROP_MIN_MS` ~ `POUR_DROP_MAX_MS` |
| the radius where ripples vanish | `CALM_FADE_RADIUS` | `POUR_FADE_RADIUS` |
| the radius of the drop's dot | `CALM_DROP_RADIUS` | `POUR_DROP_RADIUS` |
| rings a drop | picked at random from `RING_COUNTS` | the heavier, the more often only 1 ring |

## How it stays smooth (without a frame buffer)

The UNO has only 2 KB of RAM and the whole screen takes 112 KB, so it cannot be drawn in memory first and sent afterwards.

1. **Only what changed is sent**: when a ring grows by 1 px, it sends one band from the old inner edge (painted black) to the new outer edge (colored). The part in between is rewritten along the way, so changing the brightness needs no separate redraw; pixels another ring erased are restored the next time too.
2. **Fewer windows**: each window opened takes 32 µs (CASET, RASET, RAMWR). The top and bottom parts of the circle use horizontal lines, the left and right parts vertical lines, the 8 symmetric places sent together, about 2.3r windows; the parts off the screen are skipped before any command is sent.
3. **The radius follows the time**: the radius is computed from the time since the drop fell, so the speed stays the same however many rings there are.
4. **A load budget**: one update takes about 1 + 0.24r ms (measured), a ring takes about `SPEED` × (1 + 0.12 × the vanishing radius) ms a second. When the total would go over the budget (`BUDGET`), the drop does not fall, so each update moves only 1 px.

| Measured | Load | The slowest frame |
|---|---|---|
| gentle | 50 ~ 80% | about 20 ms |
| heavy rain | 72 ~ 90% | 18 ms |

| Setting | Explanation |
|---|---|
| `SPEED` | the spreading speed |
| `BORDER` | the width of a ring |
| `WALK_STEP` / `WALK_LIMIT` | how fast the rain changes; the smaller `WALK_STEP`, the longer heavy rain or gentle lasts |
| `SIGMOID_SCALE` | the smaller, the more sudden the switch |
| `BUDGET` | the share of the time the rings may take: higher is denser, but over 100% the rings jump 2 ~ 3 px at a time |

# assignments/3_rays.cpp: two rays

Two rays each start from a random point near the center, stop at the screen's edge, wait a little, then take a new point. The lines drawn are never cleared, slowly piling up into a pattern of two interwoven sets of lines. No text, no input.

| Item | Rule |
|---|---|
| slope | the first is always `M`, the second always `N` (mathematical slopes, y upward; the signs are fixed) |
| direction | each new point picks at random which way to shoot, e.g. up-right or down-left when m > 0 |
| start | within a square of ±`START_RANGE` around the center |
| growing | the whole line to the edge is computed at the start, and each frame draws only the newly grown pixels |
| speed, wait | two points in a parameter space, see below |

## The parameter space: fixed center of mass and distance

The (speed, wait time) of the two rays are two points in a parameter space, always symmetric about the center of mass (`SPEED_MID`, `WAIT_MID`), always 2 × `PARAM_RADIUS` apart; only the angle of the line between them moves. So when one is fast the other is slow, when one waits long the other waits short.

```
angle = turned + SHAKE_ANGLE · sin(2π · SHAKE_HZ · t)          rotation + shaking
spin  = sign(L) · SPIN_MAX · (1 − e^(−|L| / SPIN_SCALE))       the angular momentum L walks at random, through a concave function
```

- **The concave function**: `1 − e^(−x)` is steep near 0 and flat far away. As soon as L leaves 0 a little it turns clearly, and never faster than `SPIN_MAX` however big L gets; it reverses only when L crosses 0, so most of the time there is a clear direction of rotation.
- **Turning from power-on**: L starts at ±`WALK_LIMIT` / 2.

| Setting | Explanation |
|---|---|
| `M`, `N` | the slopes of the two |
| `START_RANGE` | above 119 clamped to 119 (`START_LIMIT`): the center is only 120 px from the edge, and with a start off the screen the length of an outward ray would be negative |
| `SPEED_MID` ± `SPEED_SPAN` | the center and range of the speed (really `SPEED_MID` ± `PARAM_RADIUS` × `SPEED_SPAN`) |
| `WAIT_MID` ± `WAIT_SPAN` | the center and range of the wait (really `WAIT_MID` ± `PARAM_RADIUS` × `WAIT_SPAN`) |
| `PARAM_RADIUS` | how far apart the two points are: 0 = the same, 1 = as different as possible |
| `SPIN_MAX` / `SPIN_SCALE` | the fastest spin, how early the concave curve flattens |
| `SHAKE_ANGLE` / `SHAKE_HZ` | how big and how fast the shaking is |
