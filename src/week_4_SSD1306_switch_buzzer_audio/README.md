# week_4_SSD1306_switch_buzzer_audio

English | [中文](讀我.md)

Tests of the I2C OLED (SSD1306, 0.96 inch 128×64), black-and-white pictures on the OLED ([lib/image](../../lib/image/README.md); the test of color pictures is in [week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/3_pictures](../week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md#tests3_picturescpp-color-pictures)), switches ([lib/switch](../../lib/switch/README.md)), buzzers ([lib/buzzer](../../lib/buzzer/README.md), melodies in [lib/audio](../../lib/audio/README.md)), and four assignments. The OLED library: see [lib/SSD1306/README.md](../../lib/SSD1306/README.md).

Every file is `.bak` now; keep the one to run:

```bash
python src/hide.py week_4_SSD1306_switch_buzzer_audio/tests/1_oled     # keep only this one (drop its .bak), the others become .bak
pio run -t upload
```

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_oled.cpp` | OLED: size, shapes, text, clipping, speed, every kind of command in the datasheet, black-and-white pictures (Flash and streamed) | I2C OLED |
| `tests/2_switch.cpp` | the switch's state as text on the serial | switch module |
| `tests/3_active_buzzer.cpp` | active buzzer: on and off, beeps, rhythms | active buzzer |
| `tests/4_passive_buzzer.cpp` | passive buzzer: frequencies (measured), scales, slides, melodies | passive buzzer |
| `tests/5_files.cpp` | using the files directly: `FONT`, `PICTURE`, `ANIMATION`, `MELODY` (converted before compiling) | OLED + passive buzzer |
| `tests/6_clicks.cpp` | click, double click, multi-click, long press of one button (`switches::Clicks`), shown by LED effects (week_2) | button + LED |
| `tests/7_play_file.cpp` | any sound file on the computer (`MELODY("path")`), turned into notes when compiling, played by the buzzer | passive buzzer |
| `tests/8_graphics.cpp` | every function of graphics (shapes, text) on the OLED: points, lines, rectangles, triangles, circles, border widths, edges, clipping, every kind of number, decimals, arrays, line breaks | I2C OLED |
| `assignments/1_photo_switch.cpp` | assignment 1: pressing the switch shows the next photo | OLED + switch module |
| `assignments/2_animation.cpp` | assignment 2: an animation on the OLED | OLED |
| `assignments/3_photo_music.cpp` | assignment 3: photos in turn, each with a different piece of music at the same time | OLED + passive buzzer |
| `assignments/4_circle_dots.cpp` | assignment 4: a big circle eats pixel dots that live and die on their own, and when grown to its limit shrinks and sprays dots out | OLED |

## Wiring

| OLED | UNO |
|---|---|
| GND | GND |
| VCC | 5V (or 3.3V) |
| SCL | `SCL_PIN` |
| SDA | `SDA_PIN` |

- **Software I2C**: `Uno::UnoI2C<SCL_PIN, SDA_PIN>` makes the timing in software, so `SCL_PIN`, `SDA_PIN` can be any two pins, not only the UNO's hardware I2C pins (A4 / A5).
- **Measured**: address 0x3C answers; the status byte is `0x43` at power-on (display off), `0x03` after initialization (display on). The low 4 bits are 3, as for an SSD1306; an SH1106 would give 8.
- **`SCL_PIN` on D13**: the on-board LED of D13 blinks along; the transfer is not affected.
- **With the M130T**: its SPI pins are fixed at D13 / D11, so `SCL_PIN`, `SDA_PIN` must not be on them.

Switch module (`tests/2_switch`, `assignments/1_photo_switch`):

| Switch | UNO |
|---|---|
| GND | GND |
| VCC | 5V |
| OUT | `SWITCH_PIN` |

Buzzers (`tests/3_active_buzzer`, `tests/4_passive_buzzer`, `tests/5_files`, `tests/7_play_file`, `assignments/3_photo_music`), two pins:

| Buzzer | UNO |
|---|---|
| the active one's + (long leg) | `BUZZER_PIN` |
| the passive one's + | `BUZZER_PIN` (100 Ω in between is safer, see [lib/buzzer](../../lib/buzzer/README.md#wiring)) |
| - | GND |

- **Telling them apart**: the one that sounds all the time on 5V is active; the silent one is passive.
- **The passive one on D9 / D10**: Timer1 toggles the pin directly, with no CPU taken (other pins use an interrupt); D9 / D10 cannot be used for PWM at the same time.

Button and LED (`tests/6_clicks`):

| Part | Pins | UNO |
|---|---|---|
| button (2 pins) | one, the other | **`BUTTON_PIN`** (with the UNO's pull-up), GND |
| LED | long leg, short leg | 220 Ω → **`LED_PIN`**, GND |

# tests/1_oled.cpp: every feature of the OLED

Each test first shows `test {n}` for `TITLE_MS`, then draws its own picture, waits `HOLD_MS` and moves on, forever. The comment above each test says "what you should see".

Text uses the 23 px font (8×23 a character, 16 a line, two lines). The 12 px font is only 4 px wide and too rough, so it is not used.

- **Initialization fails**: when `init()` returns false (no answer at 0x3C) the program stops and the screen stays black. Check the wiring and power then.
- **Drawn in RAM first**: the first 6 tests draw into the screen in the board's RAM, then send it with `display()`.
- **Commands directly**: the later tests use the controller's commands directly (contrast, scrolling, fading…), without changing the screen in RAM.

| `tests[i]` | Function | What it tests | What you should see |
|---|---|---|---|
| 0 | `testBorder` | size, direction, initialization | a frame touching all four edges, a 6×6 block in each corner, a cross in the middle, `128 x 64` the right way (not mirrored), the header pins at the top |
| 1 | `testShapes` | the shapes of `graphics` | radial lines on the left; outlined / filled rectangles, circles, triangles on the right |
| 2 | `testText` | the 23 px font | `Hello, OLED!` / `AaBbCc 123`, then `int -1234` / `f 3.14` |
| 3 | `testOffColor` | color 0 drawn over lit points | the upper half black text on white `black/white`, the lower half white text on black `white/black` |
| 4 | `testClip` | clipping | a quarter circle in each corner, half a rectangle on the left, `cut off` cut off on the right, nothing wrapping to the other side |
| 5 | `testSpeed` | the speed of partial updates | a ball bouncing for 4 seconds, then the frames a second and how many ms the whole screen takes (measured about 64 ms) |
| 6 | `testContrast` | 81h contrast | the same picture from dark to bright to dark, showing the current value |
| 7 | `testInverse` | A6h / A7h, A4h / A5h | inverted for 1 second, then back; all lit for 1 second, then back |
| 8 | `testStatus` | AEh / AFh, reading the status | off for 1 second, then the two status bytes: off `0x43` (bit 6 = 1), on `0x03` |
| 9 | `testScroll` | 26h / 27h / 29h, 2Eh / 2Fh, A3h | scrolling right 2 seconds, left 2 seconds, diagonally 2 seconds, finally back in place |
| 10 | `testStartLine` | 40h ~ 7Fh | the picture rolls up a full round (64 rows), back in place |
| 11 | `testMirror` | A0h / A1h, C0h / C8h | mirrored left-right → mirrored up-down → upside down → normal |
| 12 | `testFadeBlink` | 23h | blinking for 4 seconds, then fading out (staying dark), finally back |
| 13 | `testZoom` | D6h | `zoom` twice as big for 2 seconds (the `hidden` in the lower half cannot be seen), then back |
| 14 | `testFlashImage` | `image::FlashImage`, black-and-white pictures | a lightning bolt and three bars in a ring (the logo); then a dotted sunset and mountains; finally the sunset half off the right edge (clipped) |
| 15 | `testStreamImage` | `image::StreamImage` | start `lib/stream/source/stream.py` first: the same logo, with how many ms the transfer took at the bottom right; without it, `no answer` after 3 seconds |

## Running only some tests

- **`ONLY_TESTS` in the settings**: 0 = all in turn; to run only some, set their bits, e.g. `(1UL << 14) | (1UL << 15)` runs only the two picture tests.

## tests[15]: streaming

```bash
python lib/stream/source/stream.py     # start it first, then upload; it serves pictures, animations and sounds
```

Measured (OLED, the logo 128×64 black and white, 369 bytes): `stream.py` prints `369 bytes in 0.07 s`. The 240×240 full-color sunset (19 KB) also arrived complete, in 1.65 s.

## When it looks wrong

| What you see | Possible cause |
|---|---|
| all black | `init()` failed: VCC / GND, SCL / SDA swapped, or the address is 0x3D (change to `Screen screen(0x3D);`) |
| only the upper half, every other row | it is really a 128×32 panel: use `SSD1306::OLED128x32` (already included by `SSD1306.h`) |
| mirrored left-right or upside down | the module is mounted the other way: `FLIP = true` in the settings (`init(true)`) turns it by 180° |
| 2 columns of noise on the right, everything shifted | the controller is really an SH1106 (132 columns), which this library does not support |
| garbled now and then | wires too long or pull-ups too weak: slow down with `I2C_KHZ = 100` in the settings |

# tests/2_switch.cpp: switch

The switch module's state as text on the serial:

```bash
pio device monitor        # platformio.ini already sets monitor_speed (= BAUD)
```

```
switch on D2, SWITCH_ACTIVE_HIGH = true
starts off, level LOW
      level LOW, off for 1000 ms                      a line every second: the pin's level, how long the current state has lasted
on  (level HIGH), presses 1, was off for 2300 ms      printed on every change: the new state, level, which press, how long the last state lasted
off (level LOW), presses 1, was on for 180 ms
```

- **Measured**: D2 not pressed is `level LOW`, `off`. I cannot test the pressing: please press it and see.
- **Shows `off` while pressed**: your module outputs LOW when pressed; change `SWITCH_ACTIVE_HIGH` in the settings to `false`. `assignments/1_photo_switch` needs the same change.
- **Serial**: uses `Uno::UnoSerial<BAUD>`, so Arduino's `Serial` cannot be used at the same time.

# tests/3_active_buzzer.cpp: active buzzer

An active buzzer (+ on `BUZZER_PIN`). Each test ends with a pause of `PAUSE_MS`, and after all of them it starts over; the serial prints which test runs and what you should hear.

| Test | What you should hear |
|---|---|
| 0 `testOnOff` | a sound for 1 second (`tone()` / `stop()`) |
| 1 `testBeep` | one beep of 100 ms |
| 2 `testBeeps` | three beeps, then eight very fast ones (eight countable beeps, not one long one) |
| 3 `testSos` | Morse code SOS: three short, three long, three short |
| 4 `testRhythm` | the rhythm of Twinkle Twinkle, all at one pitch (an active one has only one pitch) |
| 5 `testNoWait` | a 500 ms beep, counting how many rounds `loop` runs meanwhile |

- **Measured**: `testNoWait` printed 58758 rounds: the player never makes the program wait. I cannot hear the other tests: please listen.
- **Never a sound**: + and - swapped, or it is really a passive one (run `tests/4_passive_buzzer` instead).
- **Sounding when it should be quiet**: it may be a 3-pin module (VCC, I/O, GND, sounding on LOW). I/O on `BUZZER_PIN`, VCC on 5V, and `Beeper` changed to `buzzer::ActiveBuzzer3Pins<BuzzerPin>` (see [lib/buzzer](../../lib/buzzer/README.md#two-pins-and-three-pins)).

# tests/4_passive_buzzer.cpp: passive buzzer

A passive buzzer (+ on `BUZZER_PIN`). It runs one test after another like `tests/3_active_buzzer`; the frequency is read back from the pin and measured, so **it can be tested without a buzzer**.

| Test | What you should hear |
|---|---|
| 0 `testA4` | A4 = 440 Hz, 1 second |
| 1 `testRange` | ten frequencies from 50 Hz to 15 kHz, low to high; loudest at about 2 ~ 4 kHz (the buzzer's resonance) |
| 2 `testScale` | the C major scale up and down |
| 3 `testSlide` | a siren: 200 → 3000 Hz and back, 1.5 seconds each |
| 4 `testBeeps` | three beeps at 2 kHz |
| 5 `testCues` | start-up (upward), success (short, upward), error (low, downward) |
| 6 `testMelodies` | Twinkle Twinkle, Ode to Joy (equal notes in a row must be told apart) |

```
passive buzzer on D9 (Timer1 output)
test 0: A4 1 s, 440 Hz: plays 440, measured 440
test 1: range
  50 Hz: plays 50, measured 50
  ...
  8000 Hz: plays 8000, measured 8000
  15000 Hz: plays 15009, measured 15010
```

- **Measured**: on D9 (Timer1 toggling it directly) and on D7 (toggled by an interrupt: `BUZZER_PIN` changed to 7), all ten frequencies measured match `realHz()`; the whole round of seven tests ran and started over.
- **15009**: the timer's ticks are whole numbers, and the nearest to 15 kHz is 15009 Hz; `realHz()` tells you the real value.
- **Another pin**: change `BUZZER_PIN`. D9 / D10 take no CPU; other pins use an interrupt.

The photos and animations are black and white, because each point of the OLED is only lit or dark.

| | Photo / animation | Conversion command (inside `lib/image` or `lib/video`) |
|---|---|---|
| sunset | `sunset_flash_128x64_palette_2` | `python source/convert.py assets/sunset.heic output --target flash --fill 128x64 --palette 2 --dither` |
| moon | `moon_flash_128x64_palette_2` | `... assets/moon.jpg ... --palette 2 --dither` |
| city | `city_flash_128x64_palette_2` | `... assets/city.png ... --palette 2 --threshold 100` |
| logo | `logo_flash_stream_128x64_palette_2` | `... assets/logo.png --target flash stream --fit 128x64 --palette 2` |
| animation | `orbit_flash_64x64_palette_16_mono` | `python source/convert.py assets/orbit.gif output --target flash --fit 64x64 --mono --keyframe 8` |

For your own photos: put them into `lib/image/assets/` (jpg, png, webp, heic all work), convert them with the commands above, and change the names in `photos[]` of the assignment.

# tests/5_files.cpp: using the files directly

Fonts, pictures, animations and sounds named directly by their file paths, converted by `src/tools/assets.py` before compiling (the first build takes a few seconds for each, afterwards the cache is used):

```cpp
using Title = FONT("/System/Library/Fonts/Supplemental/Arial Bold.ttf", "--height 16");   // an absolute path
using Moon  = PICTURE("../../../lib/image/assets/moon.jpg", "--fit 64x64 --palette 2 --dither"); // relative to this file
using Orbit = ANIMATION("orbit.gif", "--width 64 --mono");                                 // only a file name: lib/video/assets/
using Chime = MELODY("success.m4a");                                                       // no options: the defaults
```

Wiring: OLED (`SCL_PIN`, `SDA_PIN`), passive buzzer (+ through 100 Ω to `BUZZER_PIN`).

1. Text in the system font Arial Bold 16, playing `success.m4a` at the same time.
2. A photo of the moon (64 × 64, dithered).
3. The orbit animation for `ANIMATION_MS`.

For your own files: change the paths and option strings, then `pio run -t upload`.

# tests/6_clicks.cpp: button gestures

One button as several keys (`switches::Clicks`), each gesture switching to another LED effect (`LED::Effect`, see week_2_LED), with the gesture printed on the serial too:

| Button | LED |
|---|---|
| one press | blinks `BLINK_TIMES` times |
| two presses | breathing |
| three or more | heartbeat |
| held | fades brighter, and a bit brighter every `REPEAT_MS` while held on |
| released (after a long press) | fades darker |

- **A click comes a little late**: after the release it waits `DOUBLE_MS` to be sure no second press follows.
- **Tuning the feel**: `DOUBLE_MS` (the gap of a double click), `LONG_MS` (how long counts as a long press), `REPEAT_MS` (how often it repeats while held).

# tests/7_play_file.cpp: playing any sound file

`MELODY("/any/path/song.aiff")`: before compiling, `src/tools/assets.py` finds the loudest pitch at each moment and turns it into notes for the buzzer (one note at a time: chords become the main melody).

| Options | Effect |
|---|---|
| (none) | medium quality: e.g. a battle theme of 4.5 minutes → 283 notes, 2 KB |
| `"--quality high"` | catches more notes: the same piece gives 619 notes, 4.3 KB |
| `"--start 30 --duration 20"` | only 20 seconds of it |
| `"--transpose -12"` | an octave lower |
| `"--sound pcm --quality low"` | the real sound (samples), but Flash holds only a few seconds; a whole song must be streamed from the computer (`--target stream`) |

- Orchestral pieces with many instruments at once give a more broken melody; a single instrument or a hummed voice works best.
- Listen first: `python lib/audio/source/convert.py file /tmp/x --preview /tmp/x.wav` writes a wav of "how the board will play it".

# tests/8_graphics.cpp: every function of graphics

The same shape and text tests as `week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen`, made 128 × 64 black and white: `ON` (= `ScreenColor::ON`) is a lit point, `OFF` a dark one. Text uses the small 4 × 12 font (32 characters a line, 5 lines), titles the 23 px font.

| Test | What it tests | What you should see |
|---|---|---|
| 0 | `drawPoint` | a grid of a point every 8 px |
| 1 | `drawHLine…`, `drawVLine…` (Size / Points / Center) | 3 equally long horizontal lines, 3 equally long vertical lines |
| 2 | `drawLinePoints`, `drawLineAngle`, `drawLineDelta` | a fan on the left, a star of a line every 30° in the middle (the longest is 0°, to the right), an X on the right |
| 3 | three ways of writing rectangles | the top row outlined, the bottom row filled, equal sizes in a row |
| 4 | three ways of writing triangles | the top row outlined, the bottom row filled |
| 5 | two ways of writing circles | equal circles; the dark point on the filled circle on the right is the point given |
| 6 | border widths 1, 4, 20 of outlined shapes | the border grows inward; wider than half it becomes filled |
| 7 | shapes just touching the edges | all complete, nothing cut off |
| 8 | every character of the font (drawn on from the returned x) | `' '` ~ `'~'` in three lines, no gaps, no overlaps |
| 9 | strings, characters, every kind of integer | each line the comment lists |
| 10 | `float`, `double`, `setDecimals` | `3.14`, `nan inf -inf`, `ovf`, 0 and 4 decimals |
| 11 | arrays, `setSeparator` | no separator by default, `, `, `\|` |
| 12 | characters the font does not have | replaced by `?`; the big digit font has no `:` |
| 13 | transparent background, the returned x, an empty call, `fontHeight` | dark text on a lit bar; `empty ok fontHeight=12` |
| 14 | `setAutoLineBreak`, `setLineSpacing` | `\n` breaks lines; a sentence too long wraps at the right |
| 15 | shapes off the screen | only the part on the screen is drawn, nothing wraps to the other side |

- **`display()` after drawing**: graphics only changes the screen in RAM; each test sends it once at the end.
- **Running only some**: `ONLY_TESTS`, e.g. `(1UL << 2) | (1UL << 9)`.
- **About 75 % of RAM used**: the screen itself takes 1 KB (see [lib/SSD1306](../../lib/SSD1306/README.md)).

# assignments/1_photo_switch.cpp: the switch changes the photo

- The first photo at power-on; each press (`wasPressed()`) shows the next, and after the last one the first comes again.
- The photo is drawn into the OLED's screen in RAM, then `display()` sends the changed parts.
- Flash 7.2 KB, RAM 1087 B (the OLED's screen alone takes 1 KB).

# assignments/2_animation.cpp: an animation on the OLED

- `video::FlashPlayer` draws straight on the OLED: the OLED's `Frame` also provides `setWindow` / `writePixel`, drawing into the screen in RAM; `display()` is called after each frame.
- The 64×64 orbit in the middle (x = 32), looping forever.
- Measured 16.5 ~ 17 fps (the animation itself is 16.7 fps), no skipped frames, the slowest frame (an I-frame) 66 ms.
- Flash 4.6 KB, RAM 1369 B.

# assignments/3_photo_music.cpp: photos with music

Wiring: OLED (`SCL_PIN`, `SDA_PIN`) + passive buzzer (+ through 100 Ω to **`BUZZER_PIN`**, - to GND).

| Order | Photo | Music | Length |
|---|---|---|---|
| 1 | sunset | Twinkle Twinkle Little Star | 6.4 s |
| 2 | moon | Ode to Joy | 6.9 s |
| 3 | city | C major scale | 4.3 s |
| 4 | logo | start-up sound | 0.5 s (the photo stays at least `MIN_SLIDE_MS`) |

- **When it changes**: the photo changes when the music starts; when the music ends (and the photo has shown at least `MIN_SLIDE_MS`), it pauses `PAUSE_MS`, then the next pair; after the last pair the first comes again.
- **Playing at the same time**: the music is played by `audio::Player` on Timer1, without waiting; the OLED's `display()` takes about 64 ms only when the photo changes, and the music goes on (the square wave comes from the timer), only a note change at that moment comes a bit late.
- **Other music or photos**: change the two arrays `photos[]`, `songs[]`, which must be equally long (a compile error otherwise). Music is converted with `lib/audio/source/convert.py`, photos with `lib/image/source/convert.py`.
- **Volume**: `VOLUME` in the settings, 0 ~ 255 (255 the loudest, 128 6 dB quieter, 64 12 dB quieter), see [the volume of lib/buzzer](../../lib/buzzer/README.md#volume).
- **A 3-pin passive module**: change `Speaker` to `buzzer::PassiveBuzzer3Pins<Uno::UnoTone<BUZZER_PIN, true>>`.
- Flash 8.6 KB, RAM 1164 B. Already uploaded to the board; the picture and sound are for you to see and hear.


# assignments/4_circle_dots.cpp: the big circle and the pixel dots that live and die

- **The big circle**: its direction keeps changing a little at random, while it turns towards the dot it ate last (at most `STEER` radians a second); once there, only the random changes remain; it bounces off the edges.
  - for a dot at the edge, it goes only as far as the circle still fits.
  - a dot it touches disappears and the circle's area grows by `AREA_PER_DOT`, so the area is proportional to the dots eaten.
  - when the radius reaches `R_MAX` it shrinks back to `R_START` and sprays out half the dots eaten (at most `BURST_MAX`) all around.
  - the sprayed dots slow down and land after `BURST_MS`, becoming normal dots.
- **The pixel dots**: live and die by the rules you gave.
  - each frame picks `UPDATES_PER_FRAME` dots at random, computes w, ρ, P_die, P_rep, P_sex once for each, then rolls the dice.
  - each dot has its own lifetime: the probability of living to age t is S(t) = e^(−(t/τ)^β) (Weibull); past `LIFE_MAX` it always dies.
  - with fewer than `MIN_DOTS`, one is added at random, so they never die out.

## Lifetime

- **No lookup table**: the lifetime is drawn at birth as L = τ · (−ln U)^(1/β), with U uniform random in (0, 1]. An L drawn this way satisfies exactly P(L > t) = e^(−(t/τ)^β), the same result as rolling dice against a table at every moment, but computed only once a dot (`powf` + `logf`, at birth).
- **Afterwards only counting down**: every `LIFE_TICK_MS` all dots go down by 1 and die at 0. 1 byte a dot, so at most 255 ticks.
- **`LIFE_MAX`**: the most ticks a dot lives; the few whose drawn lifetime is longer all die then (S(`LIFE_MAX`) is small: e^−(`LIFE_MAX`·`LIFE_TICK_MS`/τ)^β).
- **What β means**: with β = 1 death is equally likely at any age (no memory); with β > 1 the older, the likelier to die; with β < 1 the young die more easily.
- **And P_die**: the original `P_DIE_0` (dying even when alone) was "natural death", now mostly left to the lifetime, so it is kept small. Death from crowding, C_die · ρ², stays as it was.
- **Simulated on the computer first** (without the big circle): with τ = 10 s, β = 2 the number of dots stays at 130 ~ 150, about the same as the original P_DIE_0 = 0.02.

## Problems with the rules and what was done

| Problem | What was done |
|---|---|
| `if P_rep_i:` is a probability, not a condition | random numbers: born only when `random < P` |
| with w = 0 (no neighbors), the formulas of ρ and P_sex divide by 0 | rewritten as ρ = w / (w + ρ₀), P_sex = ρ / (ρ + P_sex₀): mathematically the same, 0 when w = 0 |
| \hat v is not defined | sexual: towards a random one of the K nearest neighbors (the partner); asexual: a random direction |
| updating all at once or in order | in order favors the dots in front; all at once needs another copy of the decisions. Instead one dot is picked at random each time and applied at once (asynchronous updating), with the same expectation |
| children born nearer than the equilibrium spacing keep crowding the same clump, and when the clump gets too crowded it all dies | simulated on the computer first: with δ_asex = 3 the population fell to about 5; changed to 8 px it stays at 60 ~ 150, the nearest dots on average 3 ~ 4 px apart |
| with C_rep ≤ P_die₀, even when sparse they die faster than they are born | the comment says `C_REP` must be larger than `P_DIE_0` |
| a pixel may hold several dots; dots may be born off the screen | at most one dot a pixel; no birth on a pixel that has a dot, off the screen, or beyond `MAX_DOTS` |

"The density of pixel dots larger than the pixel unit" I take as the spacing of dots being larger than a pixel, as otherwise they look stuck together into one lump; hence the δ above and "one dot a pixel".

## Parameters

| Parameter | Meaning |
|---|---|
| `K` | how many nearest dots to look at |
| `RHO_0` | ρ = 0.5 when w equals it; bigger = more dots |
| `P_DIE_0`, `C_DIE` | death: the base probability (small: mostly left to the lifetime), how much crowding adds |
| `TAU`, `BETA` | lifetime: the probability of living to τ is e^−1 = 37 % (on average about 0.89 τ when β = 2); β see [Lifetime](#lifetime) |
| `LIFE_TICK_MS`, `LIFE_MAX` | the unit and the limit of the lifetime (in ticks, at most 255) |
| `C_REP` | birth: the probability when alone |
| `P_SEX_0` | when ρ equals it, half the children are born sexually |
| `DELTA_SEX`, `DELTA_ASEX` | how far a child is from its parent |
| `MAX_DOTS` | 3 bytes a dot (x, y, the lifetime left) |
| `UPDATES_PER_FRAME` | how many dots each frame checks: the bigger, the faster things change and the lower the frame rate |
| `R_START`, `R_MAX` | the size range of the big circle |
| `AREA_PER_DOT` | how much area a dot eaten adds: growing from `R_START` to `R_MAX` takes π(`R_MAX`² − `R_START`²) / `AREA_PER_DOT` dots, which must stay < 255 (`eaten` is uint8) |
| `GROW_EASE`, `SHRINK_EASE` | how far the radius drawn moves towards the real size each frame: growing follows slowly; shrinking at a burst is fast, back to `R_START` in a few frames |
| `STEER` | how fast it turns towards the dot it ate last: at 3, a dot beside it would be circled forever without being reached (simulated: at 6 it takes at most about 2 seconds) |

## Measured (with RHO_0 = 0.55, running 2 minutes)

- About 12.5 fps: the whole screen is redrawn every frame, and sending it alone takes 64 ms; the slowest frame 98 ms (the frame of a burst).
- The number of dots rises and falls between 4 and 57; the big circle reaches its limit about every 45 seconds.
- Too few dots, so `RHO_0` was raised (about twice the dots in the simulation).
- **RAM**: 1659 B (9 B more for the target followed, 150 B for each dot's lifetime). The sprayed dots store neither position nor velocity, only direction and initial speed (3 bytes), the position computed by formula, saving 312 B.
