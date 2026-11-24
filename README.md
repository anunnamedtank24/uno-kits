# Uno-kits

English | [中文](讀我.md)

Libraries and class programs for the Arduino UNO, in four categories (see [lib/README.md](lib/README.md#categories-and-names)):

| Category | Libraries |
|---|---|
| **Board** | `Uno`: pins, PWM, analog, interrupts, SPI, I2C, 1-Wire, USB serial, square waves, sample output, infrared carrier, background tasks |
| **Drivers** | screens (ST7789 color, SSD1306 OLED), DS18B20 temperature, TCRT5000 line tracking, L298N motors, NEC infrared remotes, switches, buzzers, LEDs, ultrasonic ranging, analog sensors (knob, light, joystick, thermistor, sound) |
| **Assets** | fonts, pictures (jpg / png / webp / heic), animations and videos (gif / mp4 / mov / webm…), sounds (wav / mp3 / m4a / ogg…): turned into `.h` on the computer, kept in Flash or streamed from the computer |
| **Common** | color formats (`color`), shapes and text (`graphics`), the protocol to ask the computer for files (`stream`) |

- Hardware is abstracted with templates; compiled, it is as fast as writing the registers directly.
- Fonts, animations, pictures, sounds: a program names the file directly (`FONT("~/x.ttf", "--height 16")`, `PICTURE(...)`, `ANIMATION(...)`, `MELODY(...)`), and it is converted before compiling; animations and pictures pick the fastest way to draw by the UNO's drawing time.
- The libraries never include each other; they work together only through interfaces (template parameters). Every interface of the UNO inherits the same base (`Uno::UnoBoard`), so another board only replaces `lib/Uno/`.
- Streaming (`lib/stream`) and color formats (`lib/color`) are template parameters too: the computer runs a single `stream.py`; the color format is declared only on the screen (`M130T<Bus, color::RGB565>`), and RGB888 or RGB444 is a change of that one place.
- **Every library has two kinds of documents**: `README.md` (English) and `讀我.md` (Chinese) for users (usage, wiring, caveats), each a sentence-by-sentence translation of the other; `INTERFACE.md` for developers (prerequisites, interface requirements, every function, parameters, resources).
- **Shapes and text are clipped**: what is off the screen is not drawn (clipped once, at the lowest level: `fillWindow` / `writeWindow`). Animations are not clipped: starting one checks that the whole animation is on the screen. The "Input limits" of each `README.md` give the range of every parameter.

## Folders

```
.
├── platformio.ini     PlatformIO settings (Arduino UNO, C++17; runs src/tools/assets.py before compiling; the board's numbers custom_board)
├── lib/               libraries, see lib/README.md
│   ├── Uno/           board: ATmega328P pins, SPI, I2C, 1-Wire, serial, timers, background tasks
│   ├── ST7789/        driver: ST7789 color screen + modules (M130T)
│   ├── SSD1306/       driver: I2C OLED (SSD1306): commands + the screen kept in the board's RAM
│   ├── DS18B20/       driver: DS18B20 temperature sensor (1-Wire)
│   ├── TCRT5000/      driver: reflective infrared sensor (HW-006 line tracking, 4-pin analog board)
│   ├── L298N/         driver: motor driver: DC, stepper, two-wheeled car; every wiring
│   ├── NEC/           driver: the NEC protocol of infrared remotes: receiver (HX1838), sending, keys, 17-key remote
│   ├── bluetooth/     driver: Bluetooth serial (SPP) and BLE: HC-05, HC-06, BT05 (BLE); AT commands, packets, the computer's packet.py, ble.py
│   ├── switch/        driver: switches (GND, VCC, OUT); click, double click, long press
│   ├── buzzer/        driver: active / passive buzzers
│   ├── LED/           driver: LEDs (PWM): one, RGB, traffic light; blinking, breathing
│   ├── ultrasonic/    driver: ultrasonic ranging (HC-SR04)
│   ├── analog/        driver: analog sensors: knob, light, joystick, thermistor, sound
│   ├── font/          asset: fonts (.ttf → .h)
│   ├── image/         asset: still pictures (.jpg / .png / .webp / .heic → .h, Flash or streamed from the computer)
│   ├── video/         asset: animations and videos (.gif / .mp4 / .mov / .webm … → .h, Flash or streamed from the computer)
│   ├── audio/         asset: sounds (.wav / .mp3 / .m4a / .ogg … → notes, rhythms or samples) and players
│   ├── color/         common: color formats: RGB565, RGB888, RGB444, RGB332, BW…; the screen's ColorFormat
│   ├── graphics/      common: shapes and text
│   └── stream/        common: asking the computer for files: the board's stream::Puller, the computer's stream.py (shared by pictures, animations, sounds)
└── src/
    ├── hide.py                                                 keeps one program, renames the others to .bak (with git mv)
    ├── clean.py                                                deletes caches and converted assets; --rebuild converts all of them again
    ├── tools/assets.py                                         converts the fonts, pictures, videos, sounds a program names, before compiling
    ├── README.md                                               how a program is written: settings, parts, sections, names, the weeks' README.md
    ├── week_1_analog/                                          tests of the analog sensors, see src/week_1_analog/README.md
    ├── week_2_LED/                                             LED brightness and effects, see src/week_2_LED/README.md
    ├── week_3_ST7789_DS18B20_graphics_color_font_image_video/  M130T tests, temperature sensor, color pictures, assignments, see src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md
    ├── week_4_SSD1306_switch_buzzer_audio/                     tests and assignments of the OLED, black-and-white pictures, switches, button gestures, buzzers, playing any sound file, see src/week_4_SSD1306_switch_buzzer_audio/README.md
    ├── week_5_NEC/                                             tests of the infrared remote, assignments with an RGB LED, a moving square, a staff, a piano, see src/week_5_NEC/README.md
    ├── week_6_ultrasonic/                                      tests of HC-SR04 ultrasonic ranging and the median filter, assignments with distance controlling volume and pitch, see src/week_6_ultrasonic/README.md
    ├── week_7_L298N/                                           tests of the L298N motor driver (one motor, two, voltage, PWM, stepper, two-wheeled car, background remote, a car with speed on EN), see src/week_7_L298N/README.md
    ├── week_8_TCRT5000/                                        tests of the HW-006 line tracking board (TCRT5000), see src/week_8_TCRT5000/README.md
    └── week_9_bluetooth/                                       tests of the HC-05 / HC-06 Bluetooth modules: AT commands, a round trip computer → USB → UNO → Bluetooth → computer, see src/week_9_bluetooth/README.md
```

## Getting started

Needs [PlatformIO](https://platformio.org/) (the VS Code extension or the CLI).

```bash
pio run                  # compile
pio run -t upload        # upload
```

**`pio` says `command not found` in the terminal**: the PlatformIO extension of VS Code installs `pio` in `~/.platformio/penv/bin/`, and only the terminals the extension opens itself ("New Terminal" in the PlatformIO sidebar) have it on the PATH. A normal terminal needs it added once:

```bash
echo 'export PATH="$PATH:$HOME/.platformio/penv/bin"' >> ~/.zshrc
source ~/.zshrc
which pio                # ~/.platformio/penv/bin/pio
```

It goes at the end of the PATH, so `python` and `pip` stay the ones you had (`penv/bin` also has PlatformIO's own `python`).

- Only one program in `src/` may have `setup()` / `loop()`; the others must be renamed to `.bak`: `python src/hide.py week_4_SSD1306_switch_buzzer_audio/tests/1_oled` keeps only that one.
- **Every program starts the same way**: a description, then `// ---------- settings ----------` with everything that can be changed (the wiring's pins `*_PIN`, `BAUD`, times, speeds, volume, the files to use…), then `// ---------- parts ----------` building the parts from these settings (`Uno::UnoInputPin<SWITCH_PIN>`). Changing a pin or tuning a value only touches the settings, no need to look further down. The full rules are in [src/README.md](src/README.md).
- Wiring: see [src/week_1_analog/README.md](src/week_1_analog/README.md#wiring) (analog sensors), [src/week_2_LED/README.md](src/week_2_LED/README.md#wiring) (LEDs), [src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md](src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md#wiring) (M130T), [src/week_4_SSD1306_switch_buzzer_audio/README.md](src/week_4_SSD1306_switch_buzzer_audio/README.md#wiring) (OLED, switches, buzzers), [src/week_5_NEC/README.md](src/week_5_NEC/README.md#wiring) (infrared receiver, RGB LED), [src/week_6_ultrasonic/README.md](src/week_6_ultrasonic/README.md#wiring) (HC-SR04), [src/week_7_L298N/README.md](src/week_7_L298N/README.md#wiring) (L298N), [src/week_8_TCRT5000/README.md](src/week_8_TCRT5000/README.md#wiring) (HW-006), [src/week_9_bluetooth/README.md](src/week_9_bluetooth/README.md#wiring) (HC-05, HC-06).

```cpp
#include "Uno_ST7789.h"
#include "ST7789.h"
#include "graphics.h"
#include "color.h"
#include "font.h"

constexpr uint8_t DC_PIN = 8;
constexpr uint8_t RES_PIN = 10;
constexpr uint8_t BLK_PIN = 9;

using Bus = Uno::UnoST7789<DC_PIN, RES_PIN, Uno::NONE, BLK_PIN>;   // DC, RES, CS (none), BLK
using ScreenColor = color::RGB565;                                 // the screen's color format: graphics, font, image, video follow it
using Screen = graphics::Canvas<ST7789::M130T<Bus, ScreenColor>>;
Screen screen;

void setup() {
    screen.init();
    screen.drawFilledScreen(ScreenColor::BLACK);
    screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
    screen.drawTexts(10, 10, ScreenColor::WHITE, "hello ", 42);
}

void loop() {
}
```

## Python tools

Converting and streaming fonts, pictures, videos and sounds runs on the computer and needs Python 3:

```bash
pip install pillow pyserial numpy
brew install ffmpeg      # decodes videos and sound files (without it, macOS's afconvert reads most sound formats too)
```

**Usually you never run them yourself**: a program writes a macro such as `FONT(...)`, and `src/tools/assets.py` converts it before `pio run` (see [lib/README.md](lib/README.md#fonts-pictures-videos-sounds-use-the-files-directly)). The Python of the converters: `ASSET_PYTHON=/path/to/python` in `.env` (this computer's own, not in git; an `ASSET_PYTHON` environment variable comes first), else `custom_asset_python` in `platformio.ini` (`python3`).

**The board's numbers are not hard-coded in the converters**: the flash size and how constants are kept there (`PROGMEM`, `avr/pgmspace.h` on the UNO), the clock, the cycles of drawing on a screen, the buzzer's frequency range, the sample clock, the serial baud rate are all in [lib/Uno/board.json](lib/Uno/board.json) in the board's folder, and `platformio.ini` points to it with `custom_board`. The converters and `stream.py` take it as the `--board` parameter. For another board, write a `board.json` for it, change this line, then run `python src/clean.py --rebuild`.

| Tool | What it does |
|---|---|
| `src/tools/assets.py` | converts the files of `FONT(...)` and the other macros of the program before compiling (only those not converted yet); by hand: `python src/tools/assets.py` (`--force` converts again, `--unused` lists the unused ones) |
| `src/clean.py` | deletes `__pycache__`, `.pio/build` and the converted assets (`lib/<kind>/output/*.h`); `--rebuild` converts all of them again, `--dry-run` only lists them (see [src/README.md](src/README.md#cleanpy)) |
| `lib/font/source/` | [font/source/README.md](lib/font/source/README.md) |
| `lib/video/source/` | [video/source/README.md](lib/video/source/README.md) |
| `lib/image/source/` | [image/source/README.md](lib/image/source/README.md) (HEIC: install `pillow-heif` too; without it, macOS's `sips` is used) |
| `lib/audio/source/` | [audio/source/README.md](lib/audio/source/README.md) |
| `lib/stream/source/stream.py` | streaming: `python lib/stream/source/stream.py`, just keep it running; it sends whatever picture, animation or sound the board asks for; see [stream/README.md](lib/stream/README.md#streampy) |

No `__pycache__` is made: the scripts that import other files (`convert.py` imports `check.py`, `src/tools/assets.py`, `stream.py` loads each `stream_handler.py`) start with `sys.dont_write_bytecode = True`, so plain `python` is fine.

## Documents

Every library has three: `README.md` (English) and `讀我.md` (Chinese) for users, each a sentence-by-sentence translation of the other, so changing one means changing the other; `INTERFACE.md` for developers (writing new packages, another board, other parts).

| Library | README.md (users; Chinese: 讀我.md) | INTERFACE.md (developers) |
|---|---|---|
| [lib/README.md](lib/README.md) | overview of the libraries, the four categories (board, drivers, assets, common), interfaces, the rules of messages and errors, prerequisites, dependencies, document format, **comparison tables** (how inputs are handled, clipping, floating point, RAM, time, what waits) | — |
| Uno | [README](lib/Uno/README.md): usage, input limits, caveats (timers, interrupt conflicts) | [INTERFACE](lib/Uno/INTERFACE.md): every interface's functions, levels, compile-time checks, resources |
| ST7789 | [README](lib/ST7789/README.md): usage, color modes, wiring; [module/](lib/ST7789/module/README.md) adding a module | [INTERFACE](lib/ST7789/INTERFACE.md): the Bus interface, functions, clipping, resources |
| SSD1306 | [README](lib/SSD1306/README.md): usage, wiring, caveats | [INTERFACE](lib/SSD1306/INTERFACE.md): command table, I2C format, why the screen is kept in RAM, init sequence, speed |
| DS18B20 | [README](lib/DS18B20/README.md): usage, wiring, resolution, caveats | [INTERFACE](lib/DS18B20/INTERFACE.md): functions, error codes, timing |
| TCRT5000 | [README](lib/TCRT5000/README.md): usage, 4-pin board, wiring, how it works, the potentiometer | [INTERFACE](lib/TCRT5000/INTERFACE.md): functions, parameters, resources |
| L298N | [README](lib/L298N/README.md): usage, every wiring, wiring, every pin of the board, truth table, ratings and limits, steppers | [INTERFACE](lib/L298N/INTERFACE.md): the H-bridge, functions of DC / stepper motors and the two-wheeled car |
| NEC | [README](lib/NEC/README.md): usage, sending, wiring, part names, the remote's codes, repeats | [INTERFACE](lib/NEC/INTERFACE.md): functions, the NEC protocol, resources |
| switch | [README](lib/switch/README.md): usage, Clicks, wiring, two pins and three pins, debouncing | [INTERFACE](lib/switch/INTERFACE.md): functions, parameters |
| buzzer | [README](lib/buzzer/README.md): usage, wiring, two pins and three pins, volume, active and passive, caveats | [INTERFACE](lib/buzzer/INTERFACE.md): functions, parameters, resources |
| LED | [README](lib/LED/README.md): usage, Effect, wiring, resistors, common cathode / common anode, PWM and the eye's brightness | [INTERFACE](lib/LED/INTERFACE.md): functions, parameters, resources |
| ultrasonic | [README](lib/ultrasonic/README.md): usage, Median, wiring, how it works, results, temperature and accuracy | [INTERFACE](lib/ultrasonic/INTERFACE.md): functions, parameters, resources |
| analog | [README](lib/analog/README.md): usage, wiring, part names (knob, light, joystick, NTC, sound module) | [INTERFACE](lib/analog/INTERFACE.md): functions, resources |
| font | [README](lib/font/README.md): usage, using the files directly; [source/](lib/font/source/README.md) converting, [output/](lib/font/output/README.md) format | [INTERFACE](lib/font/INTERFACE.md): the flow |
| image | [README](lib/image/README.md): usage, using the files directly, colors and sources, drawing on the OLED; [source/](lib/image/source/README.md) converting and the stream protocol, [output/](lib/image/output/README.md) format | [INTERFACE](lib/image/INTERFACE.md): functions, parameters, how it draws, resources |
| video | [README](lib/video/README.md): usage, using the files directly, the three targets, timing; [source/](lib/video/source/README.md) converting and the stream commands, [output/](lib/video/output/README.md) format | [INTERFACE](lib/video/INTERFACE.md): functions, parameters, resources |
| audio | [README](lib/audio/README.md): usage, using the files directly, wiring, formats and quality, Flash or stream, hand-written melodies; [source/](lib/audio/source/README.md) converting, [output/](lib/audio/output/README.md) format | [INTERFACE](lib/audio/INTERFACE.md): functions, parameters, streaming, resources |
| color | [README](lib/color/README.md): usage, the formats there are, where they are used, conversions | [INTERFACE](lib/color/INTERFACE.md): the members of a ColorFormat, writing a new format |
| graphics | [README](lib/graphics/README.md): usage, outlined and filled, what each value draws | [INTERFACE](lib/graphics/INTERFACE.md): functions, one color a pixel, resources |
| stream | [README](lib/stream/README.md): usage, `stream.py`, caveats | [INTERFACE](lib/stream/INTERFACE.md): functions, parameters, protocol, resources |

| Programs | Contents |
|---|---|
| [src/README.md](src/README.md) | how a program is written: settings, parts, sections, names; `hide.py`, `clean.py`; the format of the weeks' README.md |
| [src/week_1_analog/README.md](src/week_1_analog/README.md) | tests of the analog sensors: knob, photoresistor, joystick, NTC, sound module |
| [src/week_2_LED/README.md](src/week_2_LED/README.md) | LED brightness, blinking, breathing, fading, heartbeat |
| [src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md](src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md) | M130T tests, temperature sensor, color pictures, assignments 1 ~ 3 |
| [src/week_4_SSD1306_switch_buzzer_audio/README.md](src/week_4_SSD1306_switch_buzzer_audio/README.md) | tests of the OLED, black-and-white pictures, switches, buzzers; assignments 1 ~ 4 |
| [src/week_5_NEC/README.md](src/week_5_NEC/README.md) | tests of the infrared remote; assignments 1 ~ 4 (RGB LED, moving square, staff, piano) |
| [src/week_6_ultrasonic/README.md](src/week_6_ultrasonic/README.md) | tests of HC-SR04 ultrasonic ranging; assignment 1 (switch + distance → buzzer volume), assignment 2 (distance → pitch) |
| [src/week_7_L298N/README.md](src/week_7_L298N/README.md) | tests of the L298N: one motor, two motors, voltage control, PWM, stepper, two-wheeled car, background remote, a car with speed on EN |
| [src/week_8_TCRT5000/README.md](src/week_8_TCRT5000/README.md) | tests of the HW-006 line tracking board |
| [src/week_9_bluetooth/README.md](src/week_9_bluetooth/README.md) | tests of the HC-05 / HC-06 Bluetooth modules: AT commands, a round trip computer → USB → UNO → Bluetooth → computer |
