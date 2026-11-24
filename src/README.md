# src

English | [中文](讀我.md)

The programs to upload to the UNO. Each program is one `.cpp`, and only one at a time (PlatformIO compiles every `.cpp` in `src/`, but there can be only one `setup()` / `loop()`); the others are renamed to `.cpp.bak`.

```bash
python src/hide.py week_7_L298N/tests/6_car     # keep only this one (drop its .bak), the others become .bak
pio run -t upload                       # compile, upload
pio device monitor                      # watch the serial (at BAUD, = monitor_speed in platformio.ini)
```

## Files

```
src/
├── README.md     this file: how a program is written (English)
├── 讀我.md       how a program is written (Chinese, a sentence-by-sentence translation of README.md)
├── hide.py         keeps one program, renames the others to .bak (with git mv, so git keeps the files' history)
├── clean.py        deletes caches and converted assets (__pycache__, .pio/build, lib/<kind>/output/*.h)
├── tools/
│   └── assets.py   run by PlatformIO before every build: converts the files of FONT(...) and the other macros (not a program: never compiled)
└── week_{N}_{lib}/ one folder a week, named after the week and the libraries new that week (hardware first, helpers after)
    ├── README.md, 讀我.md
    ├── tests/
    │   └── {n}_{description}.cpp    tests: one part or one library's features, checked one by one
    └── assignments/
        └── {n}_{description}.cpp    assignments: something made with these parts
```

| Folder | Contents |
|---|---|
| [week_1_analog/](week_1_analog/README.md) | analog sensors: knob, photoresistor, joystick, NTC, sound module |
| [week_2_LED/](week_2_LED/README.md) | LED brightness and effects (blinking, breathing, fading, heartbeat) |
| [week_3_ST7789_DS18B20_graphics_color_font_image_video/](week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md) | M130T tests, temperature sensor, color pictures, assignments 1 ~ 3 |
| [week_4_SSD1306_switch_buzzer_audio/](week_4_SSD1306_switch_buzzer_audio/README.md) | tests of the OLED, black-and-white pictures, switches, button gestures, buzzers, playing any sound file; assignments 1 ~ 4 |
| [week_5_NEC/](week_5_NEC/README.md) | tests of the infrared remote; assignments 1 ~ 4 (RGB LED, moving square, staff, piano) |
| [week_6_ultrasonic/](week_6_ultrasonic/README.md) | tests of HC-SR04 ultrasonic ranging and the median filter; assignments 1, 2 |
| [week_7_L298N/](week_7_L298N/README.md) | tests of the L298N: one motor, two motors, voltage control, PWM, stepper, two-wheeled car, background remote, a car with speed on EN |
| [week_8_TCRT5000/](week_8_TCRT5000/README.md) | tests of the HW-006 line tracking board |
| [week_9_bluetooth/](week_9_bluetooth/README.md) | tests of the HC-05 / HC-06 Bluetooth modules: AT commands, a round trip computer → USB → UNO → Bluetooth → computer |

## hide.py

| Command | Result |
|---|---|
| `python src/hide.py week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen` | keeps only `week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp`, all others `.bak` |
| `python src/hide.py src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp.bak` | the same (what is before `src/` and after the first `.` is dropped, so a path can be pasted as it is) |
| `python src/hide.py week_8` | keeps the one program containing `week_8`, if there is exactly one |
| `python src/hide.py '7*car'` | `*` is anything: the one containing `7`, then `car` (needs quotes) |
| `python src/hide.py` | all `.bak`, nothing is compiled |

When not exactly one matches, nothing is changed and an error is printed.

## clean.py

Deletes what the tools made and can make again: every `__pycache__`, PlatformIO's `.pio/build`, the `output/*.h` of `lib/font`, `image`, `video`, `audio` (`README.md` and `讀我.md` stay).

| Command | Result |
|---|---|
| `python src/clean.py --dry-run` | only lists what would be deleted |
| `python src/clean.py` | deletes |
| `python src/clean.py --rebuild` | deletes, then converts every file again from its `// source:` and `// options:` (same file names); one whose source is gone is not deleted, with a warning |

- **After deleting without converting**: the next build converts again only the files of macros like `FONT(...)` in the program; programs that use names directly (`font::menlo_…`, `picture::sunset_…`) need `--rebuild`, or `git restore lib` to get them back (the converted files are in git).
- **When to use `--rebuild`**: after changing a `convert.py`, after changing the board or its numbers (`custom_board` in `platformio.ini` and the `board.json` it points to), or to make every output use the latest form (about 10 s). The converters' Python is the same as for `src/tools/assets.py`: `ASSET_PYTHON` in `.env`, else `custom_asset_python` in `platformio.ini`.
- With `.pio/build` deleted, the next build compiles everything again (a bit slower, the same result).

## File names

| What | Name | Example |
|---|---|---|
| a week | `week_{N}_{lib}`: N is the week; `{lib}` is the **libraries used for the first time** that week (folder names of `lib/`, with the case of `lib/`), joined with `_` | `week_3_ST7789_DS18B20_graphics_color_font_image_video` |
| a test | `week_…/tests/{n}_{description}.cpp`: n from 1; the description is a few lowercase English words joined by underscores | `week_7_L298N/tests/6_car.cpp` |
| an assignment | `week_…/assignments/{n}_{description}.cpp`: n is the assignment's number | `week_5_NEC/assignments/4_piano.cpp` |

- **The order of `{lib}`**: hardware libraries first (`ST7789`, `DS18B20`, `switch`, `L298N`…, the stars of the week), then the helpers, always in the order `graphics`, `color`, `font`, `image`, `video`, `audio`, `stream` (the drawing basics first, then assets). Libraries of earlier weeks are not written again (week_4 also uses `graphics`, but week_3 already has it); `Uno` is used every week and never written.
- **Which week a new program goes into**: the week where the hardware library it mainly tests first appears. It may use parts of earlier weeks (e.g. `week_4_SSD1306_switch_buzzer_audio/tests/6_clicks` shows button gestures on the LED of week_2), not of later weeks.
- **No matching week**: start a new week; things more basic, used by other parts, go into an earlier week (e.g. analog input → `week_1_analog`).
- **One program testing features of several libraries**: split it into one test per library, each in its own week.

## How a program is written

Every program has this order from top to bottom, with comments in English:

```cpp
#include <Arduino.h>                  // 1. includes: Arduino.h, Uno_*.h, then the libraries' entries
#include "Uno_serial.h"
#include "Uno_pin.h"
#include "L298N.h"

// UNO + L298N board + 2 DC motors as a two-wheeled car: moves with times, told on the USB serial
//   pio device monitor        (platformio.ini: monitor_speed = BAUD)
//                                    2. description: first line "UNO + parts: what it does", then how to see the result, what to know first

// ---------- settings ----------
                                      // 3. everything that can be tuned
// wiring: L298N with both EN jumpers ON
//   +12V -> battery +   GND -> battery - AND the UNO's GND
constexpr uint8_t IN1_PIN = 5;              // IN1: a PWM pin (3, 5, 6, 9, 10, 11)
constexpr uint32_t BAUD = 1000000;          // USB serial: the same as monitor_speed in platformio.ini

constexpr uint8_t SPEED = 128;              // 0 ~ 255
constexpr uint16_t PAUSE_MS = 800;          // braked between the moves

// ---------- parts ----------
                                      // 4. the parts built from the settings
using Link = Uno::UnoSerial<BAUD>;
using Car = L298N::TwoWheelCar<Board, IN12_SIDE, IN12_POSITIVE, IN34_POSITIVE>;
Car car;

// ---------- squares ----------
                                      // 5. the program's own features, one topic a section (any name)
// ---------- tests ----------
                                      // 6. tests (only test programs)
// ---------- main ----------
                                      // 7. setup(), loop()
void setup() { ... }
void loop() { ... }
```

### 1. include

`#include <Arduino.h>` → the `Uno_*.h` used (the UNO's interfaces) → the libraries' entries (`L298N.h`, `audio.h`…). Include only the entries, never files in `module/` (why: see [lib/README.md](../lib/README.md#include-rules)).

### 2. Description

| Line | What to write | Example |
|---|---|---|
| first line | `// UNO + parts (module name): what it does` | `// UNO + HC-SR04 ultrasonic distance module: every measurement goes to the USB serial` |
| then | how to see the result (`pio device monitor`, a `stream.py` to start first) | `//   pio device monitor        (platformio.ini: monitor_speed = BAUD)` |
| then | what happens, what to know first, how to tune it | `// SPIN_90_MS depends on the car and the floor: measure it` |

The wiring is not written here but next to the pins in the settings.

### 3. settings

Changing a pin or tuning a value touches only this section, no need to look further down. **Put everything tunable here, the more the better.**

| What goes in | How | Example |
|---|---|---|
| fixed wiring (power, GND, hardware SPI) | a `// wiring:` comment | `// wiring: M130T VCC -> 3.3V, GND -> GND, SCL -> D13, SDA -> D11 (hardware SPI: fixed)` |
| every pin that can change | `constexpr uint8_t X_PIN = …;`, the comment says the pin's name on the part and its limits; analog pins as `A0`; not connected: `Uno::NONE` | `constexpr uint8_t ENA_PIN = 5;  // ENA: a PWM pin (3, 5, 6, 9, 10, 11)` |
| wiring options | `bool`, enums | `SWITCH_ACTIVE_HIGH`, `IN12_SIDE` |
| serial speed | `BAUD` (the same as `monitor_speed` in `platformio.ini`) | `constexpr uint32_t BAUD = 1000000;` |
| defaults of the libraries' template parameters | listed, even when equal to the default | `DEBOUNCE_MS`, `I2C_KHZ`, `GAP_MS`, `IR_QUEUE`, `STREAM_CHUNK` |
| the screen's color format | ST7789: `using ScreenColor = …;` (another format changes only this line); the OLED is black and white only, with the threshold `BW_THRESHOLD` | `using ScreenColor = color::RGB565;` |
| times, speeds, brightness, volume, thresholds | the name with its unit | `HOLD_MS`, `TONE_HZ`, `MIN_MM`, `SUPPLY_V`, `VOLUME` |
| which file to use | `FONT(...)`, `PICTURE(...)`, `ANIMATION(...)`, `MELODY(...)` | `using Song = MELODY("~/x.aiff", "--quality high");` |
| order tables of tests, mapping tables | arrays | `LADDER[]`, `NOTE_HZ[]`, `MOVES[]` |
| running only some tests | `ONLY_TESTS`: 0 = all, bit i = only test i | `constexpr uint32_t ONLY_TESTS = (1UL << 14) \| (1UL << 15);` |

- **One `constexpr` a line**, with the type that just fits (`uint8_t`, `uint16_t`, `int16_t`…); the comment gives the range, the unit or where it connects.
- **Names**: uppercase with underscores; pins `_PIN`; the unit last: `_MS`, `_US`, `_HZ`, `_KHZ`, `_MM`, `_CM`, `_V`; counts `_COUNT`.
- **Groups**: the wiring (pins, wiring options, `BAUD`) first, a blank line, then the parameters of the behavior.

### 4. parts

The parts built from the settings; no numbers appear here:

```cpp
using Link = Uno::UnoSerial<BAUD>;
using Button = switches::Switch3Pins<Uno::UnoInputPin<SWITCH_PIN>, SWITCH_ACTIVE_HIGH, DEBOUNCE_MS>;
Button button;
```

- **Never write `Uno::UnoInputPin<2>` directly**: a pin is always an `X_PIN` first.
- **Types get a name with `using`**: UpperCamelCase, named after the part's role (`Bus`, `Screen`, `Link`, `Speaker`, `Car`, `Button`), not its model.
- Objects (variables) use lowerCamelCase (`screen`, `car`, `speaker`).
- **Colors**: constants as `ScreenColor::RED`, `ScreenColor::color(r, g, b)`; variables and parameters holding a color use `using Color = ScreenColor::Value;`, never `uint16_t` or `RGB565::` (so another format needs no change of the code).
- Other wirings go in a comment next to it: `// 3-pin module: PassiveBuzzer3Pins<Uno::UnoTone<BUZZER_PIN, true>>`.

### 5. The program's own sections

`// ---------- name ----------`, the name in lowercase English, saying what the section does (`rings`, `keys`, `distance -> pitch`). Function names in lowerCamelCase, starting with a verb (`drawRing`, `printResult`); one comment line above each function says what it does.

### 6. tests (test programs)

```cpp
// ---------- tests ----------

// 0. palette 16, scale 2: 60 x 40 stored, 120 x 80 on the screen
//    expected: a sunset, flat color bands
void testPalette16() { ... }

void (*const tests[])() = {
    testPalette16,
    ...
};
```

- One function `testXxx()` per test, with the comment `// N. what it tests` above it and `//    expected: what you should see` on the next line; N is its position in `tests[]`, from 0.
- Pick tests with `ONLY_TESTS`; do not comment out entries of `tests[]`.
- A test with a single flow (e.g. printing the distance forever) has no `tests[]`, and its section is named `test`.

### 7. main

`setup()` and `loop()` come last, always after `// ---------- main ----------`.

- **`loop()` must not block**: the parts' actions follow "start + `update()` in every `loop()`" (the car, LED effects, buzzers, buttons all do); `delay()` only for "pause so people can look" in tests. Which functions wait: see [the "Does it wait" of lib/README.md](../lib/README.md#does-it-wait-does-loop-block).
- **Printing to the serial**: `Link::printText`, `printNumber`, `printDecimal` (decimals), `printHex`, `printPin` (a pin's name, which follows a pin change), with `"\r\n"` for a new line. A line about a problem starts with `[ERROR] …` or `[WARNING] …` (like the Python tools, see [the "Messages and errors" of lib/README.md](../lib/README.md#messages-and-errors)); plain measurements have no tag. Arduino's `Serial` is not used (it is bigger, and fights `Uno::UnoSerial` for the same UART).

## The weeks' READMEs

Every `week_{N}_{lib}/README.md` (English) and `讀我.md` (Chinese, a sentence-by-sentence translation) has this order:

| Section | Content |
|---|---|
| `# week_{N}_{lib}` | a paragraph (the week's parts, linked to the READMEs of `lib/`), how to use `hide.py`, the table of programs "File / Contents / Hardware needed" |
| `## Wiring` | each part's pins → the program's `*_PIN` (and the rules a pin must follow), which programs use them |
| `# tests/{n}_{description}.cpp: description`, `# assignments/…` | one section per program, **tests first, assignments after**: what you will see, how to use it, what can be tuned in the settings, what to do when it looks wrong |

- A program's section uses `#` (level one), its subsections `##`; in a week's README, programs of the same week are written `tests/6_car`, those of other weeks in full: `week_7_L298N/tests/6_car`.
- Settings are named by their names: "change `SCL_PIN`, `SDA_PIN`", not "change the numbers of `Uno::UnoI2C<13, 11>`".
- **No values of the settings**: a README says what a setting means and does ("waits `HOLD_MS`", "`BUZZER_PIN`: D9 or D10"), never what it is now ("waits 2 seconds", "D9"): the values live only in the program, where they are tuned on the hardware. Parameter tables have no value column, and no example tables are worked out from the current values.
- The wording follows the READMEs of lib ([the "Wording" of lib/README.md](../lib/README.md#readme-format)).
