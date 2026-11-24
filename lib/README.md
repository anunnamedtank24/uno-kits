# lib

English | [中文](讀我.md)

The libraries of this project, header files only. Hardware comes in as template parameters, so every library depends only on "interfaces", never on another library; another board only replaces `Uno/`.

## Files

In the four categories (see [Categories and names](#categories-and-names)):

```
lib/
│   ── board ──
├── Uno/            ATmega328P: pins (digital, PWM, analog, interrupts), SPI, UART, timers, background tasks; the UNO version of every interface; board.json: the board's numbers for the tools on the computer
│   ── drivers ──
├── ST7789/         ST7789 color screen: commands, windows, modules (M130T)
├── SSD1306/        SSD1306 OLED: commands, the screen in the board's RAM, modules (128×64, 128×32)
├── DS18B20/        DS18B20 temperature sensor (1-Wire)
├── TCRT5000/       TCRT5000 reflective infrared sensor: HW-006 line tracking board, 4-pin analog board
├── L298N/          L298N dual H-bridge motor driver: DC motors, steppers, the red board; device: two-wheeled car
├── NEC/            the NEC protocol of infrared remotes: receiving, sending, keys, modules (HX1838, infrared LED, 17-key remote)
├── bluetooth/      Bluetooth serial (SPP) and BLE with a UART: AT commands, EN as KEY or power switch, connected (STATE), modules (HC-05, HC-06, BT05); tool: packets; source: packet.py, ble.py
├── switch/         switches (GND, VCC, OUT); tool: click, double click, long press
├── buzzer/         active / passive buzzers
├── LED/            LEDs (brightness by PWM): one, RGB; tool: blinking, breathing, fading, heartbeat
├── ultrasonic/     ultrasonic ranging (Trig / Echo): HC-SR04; tool: median filter
├── analog/         analog sensors: knob, photoresistor, joystick, NTC thermistor, sound module
│   ── assets ──
├── font/           fonts (.ttf → .h): a program writes FONT("any path") and it is converted
├── image/          still pictures (.jpg / .png / .webp / .heic → .h): Flash or streamed from the computer
├── video/          animations and videos (.gif / .mp4 / .mov / .webm … → .h): Flash or streamed from the computer
├── audio/          sounds (.wav / .mp3 / .m4a / .ogg … → notes, rhythms or samples) and players: Flash or streamed from the computer
│   ── common ──
├── color/          color formats: RGB565, RGB888, RGB444, RGB332, BW…; the screen's ColorFormat
├── graphics/       shapes and text: drawn on any screen with fillWindow
└── stream/         the protocol to ask the computer for files: the board's Puller, the computer's stream.py (shared by pictures, animations, sounds)
```

| Category | Folder | namespace | Entry | Interfaces it depends on |
|---|---|---|---|---|
| board | [Uno](Uno/README.md) | `Uno` | include the file of each interface you use | Arduino (`millis`, `micros`) |
| driver | [ST7789](ST7789/README.md) | `ST7789` | `ST7789.h` | Bus, ColorFormat |
| driver | [SSD1306](SSD1306/README.md) | `SSD1306` | `SSD1306.h` | Bus, ColorFormat |
| driver | [DS18B20](DS18B20/README.md) | `DS18B20` | `DS18B20.h` | Wire |
| driver | [TCRT5000](TCRT5000/README.md) | `TCRT5000` | `TCRT5000.h` | Pin, AnalogPin |
| driver | [L298N](L298N/README.md) | `L298N` | `L298N.h` | Pin (output), Pwm |
| driver | [NEC](NEC/README.md) | `NEC` | `NEC.h` | InterruptPin, Carrier, Remote |
| driver | [bluetooth](bluetooth/README.md) | `bluetooth` | `bluetooth.h` | Link, Pin (output: Key), Pin (input: State) |
| driver | [switch](switch/README.md) | `switches` | `switch.h` | Pin |
| driver | [buzzer](buzzer/README.md) | `buzzer` | `buzzer.h` | Pin, Tone |
| driver | [LED](LED/README.md) | `LED` | `LED.h` | Pwm |
| driver | [ultrasonic](ultrasonic/README.md) | `ultrasonic` | `ultrasonic.h` | Trigger, Echo |
| driver | [analog](analog/README.md) | `analog` | `analog.h` | AnalogPin, Pin |
| asset | [font](font/README.md) | `font` | `font.h` | — |
| asset | [image](image/README.md) | `image`, `picture` | `image.h` | Screen (with ColorFormat), Stream |
| asset | [video](video/README.md) | `video`, `animation` | `video.h` | Screen (with ColorFormat), Stream |
| asset | [audio](audio/README.md) | `audio`, `melody` | `audio.h` | Sound, SampleOut, Stream |
| common | [color](color/README.md) | `color` | `color.h` | — |
| common | [graphics](graphics/README.md) | `graphics` | `graphics.h` | Screen (with ColorFormat) |
| common | [stream](stream/README.md) | `stream` | `stream.h` | Link |

## Categories and names

The rule: **`lib/xxx/` is a chip or a way of communicating, `module/xxx.h` is a part you actually buy**. The code follows the chip's commands or the way of communicating: parts with the same chip or the same protocol share the code, and only what differs between parts goes into `module/` (size, initialization, pins, key codes).

The libraries are in four categories, each with a fixed structure:

| Category | What it is | Folders | Named after | Structure |
|---|---|---|---|---|
| **Board** | the hardware of one board: pins, timers, serial…; implements the interfaces of the other categories | `Uno` | the board | one `Uno_*.h` per interface |
| **Driver** | a part connected to the board | with a chip: `ST7789`, `SSD1306`, `DS18B20`, `TCRT5000`, `L298N`<br>with a protocol: `NEC`, `bluetooth` (SPP)<br>without a chip (the exception): `switch`, `buzzer`, `LED`, `ultrasonic`, `analog` | the chip, the protocol; without a chip, the kind of part | core (`basic/` or the root) + `module/` (the parts you buy) + when needed: `tool/` (common features built on it), `device/` (a whole wired-up thing) |
| **Asset** | content converted from files on the computer, and the classes that play it | `font`, `image`, `video`, `audio` | the kind of asset | code + `source/` (`convert.py`, `check.py`, `stream_handler.py`) + `assets/` + `output/` |
| **Common** | no hardware, no converting: specifications and algorithms the other categories share | `color` (color formats), `graphics` (drawing), `stream` (the protocol to ask the computer for files) | what it defines | header files in the root; `stream` also has the computer's `source/stream.py` |

- **How to tell**: connects hardware → board or driver (the board is "the board itself", a driver is "a part connected to it"); has a `convert.py` → asset; everything else → common.
- **Which way the dependencies go**: assets, drivers and common never include another library; they work together only through template parameters (interfaces); the board implements these interfaces.
- **`module/`, `tool/`, `device/`**:

  | Folder | What goes in | Examples |
  |---|---|---|
  | `module/` | parts you actually buy: only what differs from other parts | `HC_SR04.h`, `KY_023.h`, `L298N_board.h` |
  | `tool/` | common features built on the core, for any part | `switch/tool/clicks.h` (click, double click, long press), `LED/tool/effect.h` (breathing, blinking), `ultrasonic/tool/median.h`, `ST7789/tool/init.h` |
  | `device/` | a whole thing wired up with this chip: one class is one way of wiring, with adjustable parameters | `L298N/device/two_wheel_car.h` |

  All three use only their own library's core, so they still depend on no other library; combinations across libraries (e.g. a line-following car = L298N + TCRT5000) are written in the program.
- **Why parts without a chip are the exception**: by the rule they would be named after the way of communicating (`GPIO/`, `tone/`, `PWM/`), but then active and passive buzzers would land in two folders, which is hard to find, so they keep the name of the kind of part.
- **File names in `module/`**: the part's name; when the same part is wired in different ways, add the pin count (`active_buzzer_2_pins.h`, `active_buzzer_3_pins.h`, `switch_2_pins.h`). A module only writes what differs for the part (size, initialization, levels, checks of the wiring); the shared code is in the core.
- **The categories exist only in the documents, the folders have one level**: PlatformIO's `lib/` sees only one level of folders; more levels need `lib_extra_dirs` and make the include paths longer.

## Include rules

**A program includes only each library's entry** (named like its folder: `SSD1306.h`, `buzzer.h`, `LED.h`…); the entry includes the core and every `module/`. Do not write `#include "module/xxx.h"`.

| Why | Explanation |
|---|---|
| there are many `module/` folders | PlatformIO adds every library's folder to the include path, so `"module/xxx.h"` is looked up in every library and **the first one found is used**. If two libraries happen to have a module with the same name, the wrong one is used silently, with no error |
| entries never share a name | an entry's file name is the library's name, always different |
| inside a library, relative paths | the entry's `"module/xxx.h"` and a module's `"../basic/xxx.h"` are looked up first in their own folder, so another library is never found |
| no extra space | modules are all templates and inline: what is not used is not compiled into the program |

So the folders need no `{chip}_module/` names: as long as programs never include a module directly, equal names are never mixed up.

**Renamed** (old programs must follow):

- `I2C/` → `SSD1306/`: I2C is a way of wiring; this folder holds the commands of the SSD1306 chip; I2C itself is in `Uno_I2C.h`.
- `temp_sensor/` → `DS18B20/`: named after the chip; the class went from `temp_sensor::DS18B20` to `DS18B20::Thermometer`.
- `infrared/` → `NEC/`: "infrared" is too vague; the code handles the NEC protocol; the receiver and the remote are in `module/`.

## Fonts, pictures, videos, sounds: use the files directly

A program names the file directly and it is converted before compiling: no need to run `convert.py` first, and the file need not be in `assets/`:

```cpp
using Title = FONT("~/Fonts/Inter.ttf", "--height 16");                      // font::…
using Cat   = PICTURE("photos/cat.jpg", "--fit 128x64 --palette 2");          // picture::…
using Orbit = ANIMATION("orbit.gif", "--width 64 --mono");                    // animation::…
using Chime = MELODY("success.m4a");                                          // melody::…
```

| Item | Explanation |
|---|---|
| Path | absolute, `~/…`, relative to the .cpp that writes it; if not found, the project folder, then `lib/<kind>/assets/` |
| Options | one string, **exactly** the options of `convert.py` (see each `source/README.md`), with the same defaults; may be left out |
| Left out | the defaults of `convert.py`: fonts `--height 16`; pictures, videos, sounds `--target flash` (sounds also `--sound passive --quality medium`) |
| Where it goes | `lib/<kind>/output/`, **exactly the same** as running `convert.py` by hand (the same file name, the same content), no separate place |
| Who converts | `src/tools/assets.py` (`extra_scripts` in `platformio.ini`): before every build it finds these four macros in `src/`; if `output/` already has one converted from "the same file with the same options" it is used, **only otherwise is it converted** |
| How converted ones are found | every .h starts with `// source: path sha256 …` and `// options: …` (only the file name and what the file says count); the order of the options and the form `--fit=…` do not matter |
| The board's numbers | the board's, not the converters': the `board.json` that `custom_board` of `platformio.ini` points to (UNO: [Uno/board.json](Uno/board.json): flash size and the attribute `PROGMEM`, clock, drawing cycles, buzzer and sample clocks, serial), passed to `convert.py` as `--board`; the `// board: name sha256 …` at the start of every .h records which was used, and it is converted again before compiling when the numbers differ |
| Names | files in `assets/`: as before (`sunset_flash_120x80_palette_16`); anywhere else: the folders in front (relative to the project, or to the home folder outside the project), e.g. `~/Mine/4_assets/audio/…/16 戦い：ウォロ.aiff` → `mine_4_assets_audio_…_16_7b5dc7_flash_passive_medium` (the Japanese left out, a 6-digit hash added); equal names get `_2`, `_3`, never replacing another |
| Source changed / gone | changed: a warning when compiling, `python src/tools/assets.py --force` converts it again; moved or deleted: it still compiles as long as the output is there |
| How it becomes a type | at compile time the macro turns the path and options into a hash; `output/registry.h` has the type of each hash (imitating a "factory": give a file, get its type); that type can also be written by its name |
| Limits | both arguments must be string constants (no variables, no macros), because the source is read before compiling |
| Space | the same as before: types not used take no Flash |

`output/selected.h` is written before every build and lists **every** .h of `output/`, so converted by hand or by a macro, each can be used by its name (`picture::sunset_flash_…`) without editing anything. Listing all of them **takes no Flash**: the data is in `static const` arrays (with the board's flash attribute) inside `inline` functions; a function never called is never generated, and the linker drops what is unused (measured: Flash and RAM exactly the same as listing only the used ones).

Unwanted files: `python src/tools/assets.py --unused` lists those no program (`.bak` included) uses; delete them yourself. To delete all, or convert all again: `python src/clean.py` (`--rebuild`), see [src/README.md](../src/README.md#cleanpy).

## Where to add things later

| To add | Where | The Uno interface needed |
|---|---|---|
| other SPI / I2C screens (ILI9341, SH1106…) | a new controller folder (`ILI9341/`), or `module/` for a new board with the same chip | the existing Bus |
| RC522 RFID reader (SDA, SCK, MOSI, MISO, IRQ, RST, 3.3V) | `RFID/` (namespace `MFRC522`, named after the chip like the screens), `module/RC522.h` | a new `Uno_SPI.h` (general SPI devices: CS + read / write; `Uno_ST7789.h` is for the screen only) |
| other NEC remotes (TV, 21-key Car MP3) | one file in `NEC/module/` | — |
| other infrared protocols (Sony SIRC, Philips RC5) | named after the protocol: `SIRC/`, `RC5/` | the existing InterruptPin |
| DHT11 temperature and humidity | named after the chip: `DHT11/` | Pin + `nowUs` (its own single-wire protocol) |
| other motor driver chips (TB6612FNG, L9110S, A4988 stepper) | named after the chip: `TB6612FNG/`…; `DCMotor`, `Stepper` can follow `L298N/` | Pin, Pwm |
| servo motors (SG90) | `servo/module/` (no common chip, named after the kind of part) | Pwm at 50 Hz (Timer1) |
| other analog sensors (soil moisture, pressure, flame, analog Hall) | one file in `analog/module/` (mostly `Input` plus a conversion) | AnalogPin |
| 28BYJ-48 stepper + ULN2003 board | named after the chip: `ULN2003/` (unipolar stepper, 4 INs) | Pin (output) |
| other ultrasonic rangers (JSN-SR04T waterproof, US-100) | one file in `ultrasonic/module/` (change `MIN_MM`, `MAX_MM`) | the existing Trigger, Echo |
| other TCRT5000 boards (KY-033) | one file in `TCRT5000/module/` | Pin |
| infrared obstacle modules (FC-51, HW-201: LED and receiver apart) | named after the kind of part: `IR_obstacle/` (no common chip) | Pin |
| 7-segment displays, LED matrices (MAX7219) | named after the chip: `MAX7219/` | `Uno_SPI.h` |
| other SPP Bluetooth modules (JDY-31) | one file in `bluetooth/module/` | the existing Link, Pin |
| other BLE modules with a UART (HM-10, JDY-08) | one file in `bluetooth/module/` (their AT commands); the computer's side is `source/ble.py` | the existing Link, Pin |

Each depends only on the interfaces of `Uno/`, never on another library; a new hardware feature first gets an `Uno_*.h` in `Uno/`.

## Interfaces

An interface is "a struct with only static functions" (duck typing): a library takes it as a template parameter, and compiled it is as fast as writing the registers directly. The full list of each interface is at the top of the header that uses it, and in the "介面需求" (interface requirements) of each library's INTERFACE.md.

Every UNO implementation inherits `Uno::UnoBoard` (`nowMs`, `nowUs`, `delayMs`, `delayUs`, `readFlash8/16`, `stopInterrupts`, `restoreInterrupts`), and the pins branch further from `UnoBasicPin` (see [the levels of Uno](Uno/INTERFACE.md#分級)), so the time, Flash and interrupt functions in "Functions needed" below are on every UNO interface.

| Interface | Who needs it | Functions needed (summary) | UNO implementation |
|---|---|---|---|
| Bus (SPI screen) | `ST7789` | `init`, `write8/16`, `cmdMode`, `dataMode`, `select`…, `delayMs`, `nowMs`, `readFlash8/16` | `Uno::UnoST7789` (`Uno_ST7789.h`) |
| Bus (I2C) | `SSD1306` | `init`, `start`, `write8`, `read8`, `stop`, `delayMs`, `nowMs`, `readFlash8/16` | `Uno::UnoI2C` (`Uno_I2C.h`) |
| Screen | `graphics`, `image`, `video` | types `ColorFormat`, `BusType` (its `readFlash8/16`, `nowMs`), `width`, `height`, `fillWindow`, `writeWindow`; `video` also needs `setWindow`, `writePixel`, `endWrite` | `ST7789::M130T`, `SSD1306::OLED128x64` |
| Link | `stream::Puller`, `bluetooth::Port` (`HC05`, `HC06`; a Port is a Link itself), `bluetooth::Packet` | `init`, `available`, `read8`, `write8`, `clearInput`, `readFlash8` (Puller), `flush`, `nowMs`, `delayMs` (bluetooth) | `Uno::UnoSerial` (`Uno_serial.h`), `Uno::UnoSoftSerial` (`Uno_soft_serial.h`: any two pins) |
| Stream | `image::StreamImage`, `video::StreamPlayer`, `audio::StreamPlayer`, `StreamSamplePlayer` | `CHUNK_SIZE`, `OK`, `ERROR_*`, `begin`, `write8/16/32`, `writePath`, `waitAnswer`, `available`, `pullLength`, `receive`, class `Answer` | `stream::Puller<Link, CHUNK>` (`lib/stream`) |
| ColorFormat | `ST7789::Window`, `SSD1306::Frame` (the screen declares it); graphics, font, image, video take it from `Screen::ColorFormat` | `Value`, `BITS`, constants, `color(r, g, b)`, `red/green/blue`, `fromRGB565`; ST7789 also needs `R/G/B_BITS`, `ORDER`, `…Field` | `color::Packed<…>` (`RGB565`, `RGB888`, `RGB444`…), `color::BW<threshold>` (`lib/color`) |
| Wire | `DS18B20` | `init`, `reset`, `write8`, `read8`, `readBit`, `nowMs` | `Uno::UnoOneWire` (`Uno_one_wire.h`) |
| Pin (input) | `switch`, `TCRT5000`, the joystick's button | `init`, `read`, `nowMs` | `Uno::UnoInputPin` (`Uno_pin.h`), `UnoInterruptPin`, `UnoAnalogPin` (all read digital too) |
| Pin (output) | `buzzer::ActiveBuzzer2Pins`, `3Pins`, `ultrasonic::Ranger` (Trigger, only `init`, `write`), `L298N::Bridge` (IN) | `init`, `write`, `nowMs`, `nowUs`, `readFlash8/16` | `Uno::UnoOutputPin` (`Uno_pin.h`) |
| Pwm | every module of `LED`, `L298N::Bridge` (EN or IN: whichever is a PWM pin sets the speed; an IN also needs `nowMs`, `nowUs`, `stopInterrupts`) | `init`, `write(duty)` | `Uno::UnoPwmPin` (`Uno_pin.h`) |
| InterruptPin | `NEC::Receiver`, `ultrasonic::Ranger` (Echo) | `init(onChange)`, `read`, `nowUs`, `nowMs`, `stopInterrupts`, `restoreInterrupts` | `Uno::UnoInterruptPin` (`Uno_interrupt_pin.h`) |
| AnalogPin | all of `analog`, `TCRT5000::AnalogReflector` | `init`, `readAnalog`, `readAnalogAverage`, `referenceMillivolts`, `nowMs`, constant `MAX` | `Uno::UnoAnalogPin` (`Uno_analog_pin.h`) |
| Background | anything with `update()` (the classes of `L298N` are protected) | a function without arguments | `Uno::UnoBackground` (`Uno_background.h`) |
| Carrier | `NEC::Transmitter` | `init`, `on`, `off`, `delayUs`, `nowMs` | `Uno::UnoIrCarrier` (`Uno_ir_carrier.h`) |
| Remote | programs using `NEC` | `ADDRESS`, `toKey(code)`, `toCommand(key, command)` | `NEC::Remote17KeyNumbersTop`, `Remote17KeyArrowsTop` |
| Tone | `buzzer::PassiveBuzzer2Pins`, `3Pins` | `init`, `start`, `stop`, `realHz`, `nowMs`, `readFlash8/16`, constant `IDLE_LEVEL_HIGH` | `Uno::UnoTone` (`Uno_tone.h`) |
| Sound | `audio::Player`, `StreamPlayer` | constant `HAS_PITCH`, `init`, `start`, `stop`, `nowMs`, `readFlash8/16` | every module of `buzzer` |
| SampleOut | `audio::SamplePlayer`, `StreamSamplePlayer` | `init`, `start(rate, next)`, `stop`, `realRate`, `stopInterrupts`, `restoreInterrupts`, `readFlash8/16` | `Uno::UnoPcmOut` (`Uno_pcm_out.h`) |

## Messages and errors

Messages show up in three places, each with its own rules:

| When | Who says it | Where you see it | Where it is written |
|---|---|---|---|
| running Python on the computer (`convert.py`, `check.py`, `stream.py`, `src/tools/assets.py`, `src/hide.py`, `src/clean.py`) | that script | the terminal | the script `print`s it |
| compiling C++ | a library's `static_assert`, warnings | the terminal (`pio run`), the Problems list of VS Code | next to the line that checks (in the header) |
| while the board runs | functions return result codes; the program decides whether to show them | **not in the terminal**: the serial (`pio device monitor`, `[BOARD]` of `stream.py`), a screen, LEDs | the library returns a code, the program prints it |

**Why there is no `lib/error` or `lib/Uno/error`**: a compile message must be written on the line of the `static_assert` and cannot move; run-time codes belong to each class (`StreamImage::ERROR_NO_ANSWER`). A shared library would have to be included by every library, which breaks "libraries never depend on each other". So what is shared is the **rules**, written here, and each library follows them.

### Python

| Tag | Used for |
|---|---|
| `[INFO]` | what is being done, a summary of the result |
| `[SUCCESS]` | something is done (a file saved, an answer sent) |
| `[WARNING]` | it can go on, but the result may not be what you want (beyond the recommended range, slow, much RAM) |
| `[ERROR]` | it cannot go on; a bug of the program itself is `[ERROR] internal: …` |
| `[BOARD]` | text the board sent, printed by `stream.py` |

- **Form**: `[tag] who: what`, e.g. `[ERROR] week_3/…/1_screen.cpp:12: convert.py failed`.
- **More detail**: on the next lines, indented 4 spaces, without a tag.
- **Separators**: when doing several jobs at once (e.g. converting several files), a line `---------- name ----------` before each, like the section marks of the programs.
- **Wrong arguments**: `parser.error()` of `argparse` (prints the usage and the error, exit code 2), no traceback.

### Compiling (C++)

| Kind | How | Example |
|---|---|---|
| error: must never be used | `static_assert(condition, "namespace::Class: what is wrong (how to fix it)")` | `"Uno::UnoST7789: DC is required and must be 0 ~ 19"` |
| warning: works but not recommended | `namespace::warn::Name<condition>`: a deprecated constant whose message says "what happens, how to avoid it" | `image::warn::PaletteInRam`, `Uno::warn::…` |
| errors of a macro | `"MACRO_NAME(...): …"` | `"PICTURE(...): …"` |

- Messages are in English (like the code) and always start with `namespace::Class:`, so who said it can be found even in hundreds of lines of compiler output.
- What can be checked when compiling is not left for run time.

### At run time (the board)

| Rule | Explanation |
|---|---|
| only success / failure | return a `bool` (e.g. the OLED's `init()`) |
| several reasons | return a `uint8_t` code: `OK` = 0, failures are `ERROR_NAME` (≥ 1), constants in the class |
| one protocol, the same codes | streaming always uses `Stream`'s: `OK`, `ERROR_NOT_FOUND` (1) ~ `ERROR_NO_MEMORY` (6); a class's own codes come after (`video::StreamPlayer::ERROR_TOO_BIG` = 7) |
| libraries print nothing | a library does not know whether there is a serial or a screen; it only returns codes |
| when a program prints | on the serial, as in Python: a line about a problem starts with `[ERROR] …` or `[WARNING] …`; plain measurements have no tag |

## Prerequisites overview

The assumptions that span the libraries; details in the "前提條件" (prerequisites) of each INTERFACE.md:

| Prerequisite | Who has it | The shared way |
|---|---|---|
| **Color format** | declared by the screens (`ST7789`, `SSD1306`); used by `graphics`, `font`, `image`, `video` | written only on the screen: `M130T<Bus, color::RGB565>`, `OLED128x64<Bus, color::BW<>>`; the other libraries always use `Screen::ColorFormat`, and the type of a color is always `Screen::ColorFormat::Value`. A format the screen does not support is stopped when compiling |
| **Colors of files** | `image`, `video` (`convert.py`'s outputs, streams) | always RGB565, changed with `ColorFormat::fromRGB565()` when drawing; a palette is changed once at the start |
| **Data in Flash** | the outputs of `font`, `image`, `video`, `audio` (the ADPCM step table included) | the arrays get the board's flash attribute (`flash.attribute` of `board.json`); reading is always through the board's `readFlash8/16` (UNO: `pgm_read_*`). No library but the board writes `PROGMEM` |
| **Streaming** | `image::StreamImage`, `video::StreamPlayer`, `audio::StreamPlayer`, `StreamSamplePlayer` | all take a `Stream` parameter (`stream::Puller<Link, CHUNK>`) and share the result codes (`OK`, `ERROR_NOT_FOUND` ~ `ERROR_NO_MEMORY`); the computer runs one `stream.py` |
| **RAM taken at run time** | only the palette of streamed 256 colors (`HeapPalette` of `image`, `video`) | a compile warning (`PaletteInRam`); a failed `malloc` returns `ERROR_NO_MEMORY`; `image` `free`s it before `draw()` returns, `video` at `close()` / the next open / when the object goes away. All other RAM is in objects or statics, which the compiler counts |
| **Calling `update()` all the time** | `audio`, `video`, `switch`, `LED::Effect`, `TCRT5000`, `analog`, `ultrasonic`, `L298N` | functions return at once, `update()` does the next step; it can be handed to `Uno::UnoBackground` |
| **Timers, interrupts** | `Uno::UnoTone`, `UnoPcmOut` (Timer1, Timer2), `UnoIrCarrier` (Timer2), `UnoInterruptPin` (pin change interrupts) | the conflicts are in [Uno/INTERFACE.md](Uno/INTERFACE.md); whatever can be checked when compiling is a `static_assert` or a warning |
| **One serial** | `stream` (`Uno::UnoSerial`, D0 / D1) | Arduino's `Serial` and `pio device monitor` cannot be used at the same time |

## Dependencies

```
Uno_ST7789.h ─(Bus)─▶ ST7789::Datasheet ▶ Window ▶ M130T ──────┐
Uno_I2C.h ───(Bus)─▶ SSD1306::Datasheet ▶ Frame ▶ OLED128x64 ─┤ Screen
                                                                ▼
                     graphics::Shapes ▶ Text (◀─ font) = graphics::Canvas<Screen>
                                                                │
                     image::FlashImage, StreamImage ◀───────────┤ (writeWindow: any screen)
                     video::FlashPlayer, StreamPlayer ◀─────────┘ (setWindow: any screen)
color ─(ColorFormat)─▶ ST7789::M130T, SSD1306::OLED… ─(Screen::ColorFormat)─▶ graphics, font, image, video
Uno_serial.h ─(Link)─▶ stream::Puller ─(Stream)─▶ image::StreamImage, video::StreamPlayer, audio::StreamPlayer, StreamSamplePlayer

Uno_one_wire.h ─(Wire)─▶ DS18B20::Thermometer
Uno_pin.h ─────(Pin)──▶ switches::Switch2Pins, Switch3Pins ─▶ tool: switches::Clicks
Uno_pin.h ─────(Pin)──▶ TCRT5000::Reflector ▶ HW006
Uno_analog_pin.h ─(AnalogPin)─▶ TCRT5000::AnalogReflector (4-pin board)
Uno_analog_pin.h ─(AnalogPin)─▶ analog::Input ▶ Potentiometer, Photoresistor; Joystick ▶ KY023; Thermistor ▶ KY013; SoundLevel ▶ KY037
Uno_pin.h ───(Trigger)─┐
Uno_interrupt_pin.h ─(Echo)─▶ ultrasonic::Ranger ▶ HCSR04 ─▶ tool: ultrasonic::Median
Uno_pin.h ─────(Pin)──▶ buzzer::ActiveBuzzer2Pins, 3Pins ──┐
Uno_tone.h ────(Tone)─▶ buzzer::PassiveBuzzer2Pins, 3Pins ─┴─(Sound)─▶ audio::Player, StreamPlayer
Uno_pcm_out.h ─(SampleOut)──────────────────────────────────────▶ audio::SamplePlayer, StreamSamplePlayer
Uno_pin.h ─────(Pwm)──▶ LED::SingleLED ▶ LED2Pins, RGBLED4Pins, TrafficLight4Pins; tool: LED::Effect
Uno_pin.h ─(Pin, Pwm)─▶ L298N::Bridge (L298NBoard::A, B, any wiring) ─(Bridge)─▶ L298N::DCMotor, Stepper ─▶ device: TwoWheelCar
Uno_background.h ─(background)─▶ any update() (e.g. car.update())
Uno_interrupt_pin.h ─(InterruptPin)─▶ NEC::Receiver (HX1838) ─(Code)─▶ NEC::Remote17Key… ─▶ NEC::Key
Uno_ir_carrier.h ─(Carrier)─▶ NEC::Transmitter (IRLED, KY005) ◀─(Key → command)─ NEC::Remote17Key…
Uno_serial.h ─(Link)─┬─▶ bluetooth::Port ▶ HC05 (+ Key, State), HC06 (+ State) ─(Link)─▶ tool: bluetooth::Packet, stream::Puller
Uno_pin.h (Key, State)┘
```

- **Another board**: replace only `Uno/`, writing structs with the same functions for the new board.
- **Another screen module**: add a file in `ST7789/module/` or `SSD1306/module/`.
- **Another remote**: add a struct with `ADDRESS` and `toKey` in `NEC/module/`; the program changes one line, `using Remote`.
- **Another thing that makes sound**: write a struct with the Sound functions, and `audio::Player` plays on it.
- **`color` and `font` depend on nothing**; `graphics` itself has no color constants.
- **Another color format for the screen**: change only the screen's `ColorFormat` (e.g. `M130T<Bus, color::RGB888>`); graphics, font, image, video stay as they are; a new format is a struct with the ColorFormat members (see [color/INTERFACE.md](color/INTERFACE.md)).
- **Streaming a new kind**: the board uses `stream::Puller`, the computer gets a `lib/<kind>/source/stream_handler.py`, which `stream.py` loads by itself.

## README format

Every README has the same structure and wording; sections a library does not have are left out.

**Two languages**: `README.md` is written only in English; the Chinese goes into `讀我.md` in the same folder. Each is a **sentence-by-sentence translation** of the other: the same sections, tables, sentences and order; changing one means changing the other in the same edit. Both have `English | [中文](讀我.md)` under the first line (the Chinese one `[English](README.md) | 中文`); links point to files of the same language (a `README.md` links to other `README.md`, a `讀我.md` to other `讀我.md`), and `INTERFACE.md` is only in Chinese.

**Three documents per library**:

| File | For whom | Sections (in order, missing ones left out) |
|---|---|---|
| `README.md`, `讀我.md` | users: how to use it, how to wire it, what to watch out for | title + the "Needs / Gives" table → one sentence "details in INTERFACE.md" → a paragraph of explanation → `## Usage` → other usages → `## Wiring` → topic sections for users → `## Input limits` → `## Caveats` |
| `INTERFACE.md` | developers: writing new packages, another board, other parts | `# Name：介面` + the same "Needs / Gives" table → one sentence "usage in 讀我.md" → `## 前提條件` → `## 檔案` → `## 介面需求` (or `## 提供的介面`) → `## 函式` → `## 參數` → topic sections for developers → `## 輸入限制` → `## 資源` |

The two repeat some things (the "Needs / Gives" table, the input limits): so neither side has to jump back and forth.

| Section | Content |
|---|---|
| "Needs / Gives" table | seen first: the interfaces needed (duck typing; in parentheses the ready UNO implementation) → the classes you get and what they do |
| `## Usage` | the shortest complete example (pins first as `constexpr uint8_t X_PIN = …;`, then put into `Uno::Uno…<X_PIN>`), last line "Complete examples: `src/…`" |
| Other usages | examples of extra classes or features, e.g. `## Clicks`, `## Sending`, `## Using the files directly (recommended)` |
| `## Wiring` | the hardware's pin table, power, pull-ups, currents |
| Topic sections for users | the part's names, how it works, how to choose (formats and quality, the three targets, resolution…) |
| `## 前提條件` (prerequisites) | a table "Item / Condition": what the library assumes: interfaces, color format, streaming, calling `update()` all the time, timers and interrupts, RAM taken at run time and when it is freed |
| `## 檔案` (files) | the folder tree, one line per file |
| `## 介面需求` (interface requirements) | the static functions the template parameters need (duck typing), the ready implementations |
| `## 函式` (functions) | every public function, each explanation starting with a verb ("draws…", "returns…", "sets…") |
| `## 參數` (parameters) | template parameters, defaults |
| Topic sections for developers | protocols, data formats, how it draws, why it is designed this way |
| `## Input limits` | a table "Item / Range / Beyond it" |
| `## 資源` (resources) | RAM, Flash, time, the hardware it takes (pins, timers, interrupts) |
| `## Caveats` | a table "Situation / Result / What to do": what cannot be done, common problems |

**The READMEs of `source/`, `output/`, `module/`**: they are for developers anyway and have no `INTERFACE.md`; they too have an English `README.md` and a Chinese `讀我.md`.

**The README of `source/`**: a paragraph (packages needed) → `## Files` → `## Usage` → `## convert.py` (files supported, parameters, steps, errors and warnings) → `## check.py` → (`## stream_handler.py`: for `stream.py` of `lib/stream`).

**The README of `output/`**: a paragraph → `## Files` → `## selected.h, registry.h` → `## The current…` → `## The generated struct` → `## Format (format N)` → `## Names and sources`.

**The README and programs of `src/`**: see [src/README.md](../src/README.md).

**Wording**:

| Use | Not |
|---|---|
| function, return, call | method, return value, invoke |
| pin, Flash, RAM, cycle, duty cycle | lead, flash, memory (when RAM is meant), period count, on-ratio |
| default, range, upper limit, lower limit | factory setting, interval |
| interface (a duck-typing struct) | API contract, protocol |
| "Beyond it" uses only: compile error, compile warning, clipped, clamped, not drawn / silent, returns an error, `convert.py` error, not checked, — | anything else |
| a space between number and unit: 20 ms, 2 KB; ranges as 0 ~ 19 | 20ms, 0-19 |

## Comparison

### How unreasonable inputs are handled

| Handling | When | Examples |
|---|---|---|
| **Compile error** | known at compile time to never work | a pin that does not exist, an SPI pin used as DC / RES / CS / BLK, no CS but not SPI mode 3, baud rate off by > 5%, `FlashImage` / `FlashPlayer` given a stream-only picture or animation, a color format the screen does not support (ST7789 + RGB332 / BW, SSD1306 + colors) |
| **Compile warning** | works, but not recommended | two signals on one pin, D0 / D1 used, BLK not a PWM pin or on Timer0, baud rate off by > 2%, `StreamImage` / `StreamPlayer` given a flash-only picture or animation, or 256 colors (`PaletteInRam`: `malloc` at run time) |
| **Clipped** | at run time, drawing off the screen | every shape, text, `writeWindow`, picture: only the part on the screen is drawn |
| **Clamped** | at run time, a parameter out of range | SSD1306: `setMultiplexRatio`, `setClock`, `setPrecharge` (other commands keep only the bits the datasheet defines); `UnoTone`: 16 ~ 20000 Hz |
| **Not drawn / silent** | at run time, the parameter itself makes no sense | width or height ≤ 0, radius < 0, border ≤ 0, empty string; an animation that does not fit the screen; hz, count, ms of `audio::Player` being 0 |
| **Returns an error** | at run time, the hardware does not answer or the data is broken | DS18B20: `ERROR_NO_SENSOR`, `ERROR_CRC`…; OLED: `init()`, `display()` return false; `StreamImage::draw()`, `StreamPlayer::error()` |
| **`convert.py` error / warning** | when the data is made on the computer | font, image, video, audio: beyond the format's limits is an error, beyond the recommended range a warning |
| **Not checked** | checking is too costly or impossible | `setWindow` off the screen, raw datasheet commands, a `readFlash` pointer not in Flash, a `writeWindow` array too short, triangle corners ≥ 2048 apart, how a buzzer is wired (external supply, PWM) |

### What is clipped

| Thing | Clipped | Why |
|---|---|---|
| `fillWindow`, `writeWindow` of `ST7789::Window`, `SSD1306::Frame` | ✅ clipped here | the lowest level: clipping once is enough |
| shapes and text of `graphics` | ✅ | all go through `fillWindow` |
| the two classes of `image` | ✅ | a row is drawn in segments with `writeWindow` |
| the two players of `video` | ❌, checked at the start | a frame streams straight into one window (`setWindow`) |
| `setWindow`, raw datasheet commands | ❌ | they set the controller directly |

### Where floating point is used

The UNO has no hardware floating point: one division takes about 500 cycles, and it takes extra Flash. It is compiled in only when a program uses one of these:

| Uses floating point | For | Extra Flash (measured) |
|---|---|---|
| `drawLineAngle` | sin / cos | about 0.6 KB (when the program already has other floating point) |
| `drawTexts` drawing `float` / `double` | turning it into decimal | about 1.3 KB |
| `DS18B20::Thermometer::toCelsius` | returns a `float` | the floating point library |
| other shapes, pictures, videos, melodies | none | — |

### RAM (sizeof measured on the AVR; "about" is an estimate)

| Object | bytes | Content |
|---|---|---|
| `ST7789::M130T<Bus, ColorFormat>` | about 10 | width, height, offsets; half a pixel of 12 bits |
| `graphics::Canvas<M130T<Bus, ColorFormat>>` | about 29 | the above + font pointer, text settings |
| `SSD1306::OLED128x64<Bus, ColorFormat>` | 1043 | the whole screen 1024 + the changed range of each page 16 + address, state 3 |
| `graphics::Canvas<OLED128x64<Bus, ColorFormat>>` | 1062 | the above + font pointer, text settings: less than half of the UNO's RAM is left |
| `image::FlashImage<Screen>` | about 115 | a segment of colors 64 + palette 32 + state |
| `image::StreamImage<Screen, Stream>` | about 180 | the above + `Stream::Answer` (CHUNK = 64); streaming 256 colors `malloc`s a palette while drawing |
| `video::FlashPlayer<Screen>` | 309 | decoder 293 (a row buffer of 240, palette 32) + time |
| `video::StreamPlayer<Screen, Stream>` | about 394 | decoder + `Stream::Answer` (CHUNK = 64) + state; about 346 with CHUNK = 16; streaming 256 colors `malloc`s a palette until `close()` |
| `audio::Player<Sound>` | about 32 | state, time, melody pointer, encoding; the melody is in Flash |
| `audio::StreamPlayer<Sound, Stream>` | about 30 + CHUNK | state + `Stream::Answer` |
| `audio::StreamSamplePlayer<SampleOut, Stream>` | about 30 + BUFFER (default 512) | a ring buffer of samples (static) |
| `DS18B20::Thermometer<Wire>` | 1 | resolution; reading uses another 9 bytes of stack |
| `switches::Switch3Pins<Pin>` (or `Switch2Pins`) | about 16 | state, times, counts |
| `NEC::Receiver<InterruptPin>` | about 30 (static) | a queue of 4 codes, decoding state, times |
| `LED::RGBLED4Pins<…>`, `TrafficLight4Pins<…>` | 3 | the brightness of three colors |

The UNO has only 2048 bytes of RAM. Objects not needed afterwards (pictures, players) go inside a function, and their RAM comes back when it ends.

### Drawing time (estimates, 16 MHz, SPI 8 MHz)

| Action | About how many cycles | Explanation |
|---|---|---|
| one window | 480 | setting CASET, RASET, RAMWR |
| one pixel | 44 | SPI sends 2 bytes; the hardware limit at 8 MHz is 32 |
| the whole screen | 2.5 M (about 160 ms) | 240×240 pixels |
| triangle | about 100 per point of its bounding box | every point checked, slower |

### Does it wait (does loop() block)

Most classes "start something, then `loop()` calls `update()` every time": the function returns at once, and `update()` does the next step when it is time, so one `loop()` can look after several things at once (the car driving, an LED breathing, a buzzer singing, a button being read). `update()` can also run in the background in `Uno::UnoBackground`, and then even a `delay()` in `loop()` does not matter.

| Does not wait (returns at once, call `update()` all the time) | Waits (returns when done) |
|---|---|
| `L298N`: every move of `DCMotor`, `Stepper`, `TwoWheelCar` (`car.forward(SPEED)`, `car.forward(SPEED, ms)` only go into the queue) | `TwoWheelCar::wait()`: waits until the queued moves are done |
| `audio::Player`, `SamplePlayer`: `play()` | `open()` of `audio::StreamPlayer`, `StreamSamplePlayer`: waits for the computer's answer (at most 3 s); afterwards each `update()` waits at most about 1 ms |
| `video::FlashPlayer`, `StreamPlayer`: `start()` / `open()` | `draw()` of `image`: the whole picture (`StreamImage` also waits for the computer to send it) |
| the filters of `switches`, `Clicks`, `LED::Effect`, `TCRT5000`, `analog` | every drawing function of `graphics`: returns when drawn (see the times above); `display()` of `SSD1306`: returns when sent (the whole screen about 64 ms) |
| `ultrasonic::Ranger::update()` | `Ranger::measure()`: at most about 40 ms |
| `DS18B20`: `startConversion()` → `isConversionDone()` → `readRaw()` | `DS18B20::readCentiCelsius()`: at most `conversionMs()` + 50 ms (about 800 ms at 12 bits) |
| `NEC::Receiver`: the interrupt queues codes, nothing is missed while the program is busy | `NEC::Transmitter::send()`: 67.5 ms a key |
| — | `Uno::UnoAnalogPin::readAnalog()`: about 112 µs; `delayMs`, `delayUs` |

Do not use waiting functions next to things that need good timing: e.g. a car driven by `update()` in `loop()` does not change its move while `readCentiCelsius()` holds things up for 800 ms. Use the non-waiting way, or put `update()` into `Uno::UnoBackground`.

## When parameters are decided

The data of font, image, video, audio is converted beforehand by `source/convert.py`. Parameters come in four stages; the earlier, the more Flash, RAM and time the board saves, but changing them means converting or uploading again:

| Stage | How to change | What changing needs |
|---|---|---|
| ① when converting (`convert.py`, or macros like `FONT(...)`: converted before compiling) | command-line arguments / the options string of the macro, written into the data of the `.h` | macro: change the string and upload (converted again by itself); by hand: convert again, upload again |
| ② when compiling (template parameters, which type) | types in the program, e.g. `StreamImage<Screen, StreamLink>`, the screen's `ColorFormat` | upload again |
| ③ when starting (the arguments of `start`, `open`, `play`) | can differ every time a new one starts, but not while playing | — |
| ④ any time | normal functions, also while playing | — |

### font

| Stage | Parameters |
|---|---|
| ① when converting | the font file, size (`--height` / `--width` / `--size`), char range (`--first`, `--last`), the char used for missing ones (`--default`) |
| ② when compiling | which font type (all are listed in `selected.h`, `registry.h`; unused ones take no Flash) |
| ④ any time | `setFont<Font>()` (switching between compiled-in fonts), position and color of every drawing, `setDecimals`, `setSeparator`, `setLineSpacing`, `setAutoLineBreak` |
| cannot change | the size and weight of the letters: another size is another converted font |

### image

| Stage | Parameters |
|---|---|
| ① when converting | `--target` (Flash or stream), size (`--width` / `--height` / `--resize` / `--fit` / `--fill`), `--scale`, `--resample`, colors (`--palette`, `--colors`, `--dither`, `--threshold`, `--invert`, `--background`), `--encoding`, `--max-bytes` |
| ② when compiling | `SEGMENT` (pixels drawn at a time, RAM), the Stream's `CHUNK` (bytes `StreamImage` receives at a time), the types of Screen (with color format) and Stream, which picture (type) |
| ③ when starting | `draw<Picture>(x, y)`: which picture, where (the part off the screen is clipped) |
| cannot change | scale, colors, inversion: all in the data |

### video

| Stage | Parameters |
|---|---|
| ① when converting | `--target`, size and `--scale`, `--resample`, colors (`--palette`, `--colors`, `--dither`, `--mono`, `--threshold`, `--invert`, `--background`), cutting (`--start`, `--duration`), frame rate (`--max-fps`, `--min-fps`), size in bytes (`--max-bytes`, `--reduce`), `--keyframe`, `--tile` |
| ② when compiling | the Stream's `CHUNK` (`StreamPlayer`), the types of Screen (with color format) and Stream, which animation (type) |
| ③ when starting | `FlashPlayer::start<Animation>(x, y)`; `StreamPlayer::open<Animation>(x, y, play, loop, speed)` |
| ④ any time | `StreamPlayer`: `play`, `pause`, `stop`, `seek`, `setSpeed`, `setLoop`, `close`. `FlashPlayer` only has `update()`: another animation or position needs a new `start` |
| cannot change | frame rate, scale, colors; the speed and looping of `FlashPlayer` (it always keeps the original timing and loops) |

### audio

| Stage | Parameters |
|---|---|
| ① when converting | `--target`, `--sound` (notes for passive / a rhythm for active / samples), `--quality`; notes: `--quantize`, `--transpose`, `--range`, `--min-note`, `--min-rest`, `--speed`; rhythm: `--hz`; samples: `--rate`, `--encoding`, `--gain`; detection: `--silence`, `--voicing`, `--onset`; cutting: `--start`, `--duration` |
| ② when compiling | which pin, which buzzer (`UnoTone<PIN, IDLE_HIGH>`, `PassiveBuzzer2Pins` / `3Pins`, `UnoPcmOut<PIN>`), `GAP_MS` (the gap between notes), `CHUNK`, `BUFFER` (streaming), which sound (type) |
| ③ when starting | `play<Melody>(repeat)`, `open<Melody>(repeat)`: which one, repeated or not |
| ④ any time | `setVolume` (every player), `stop`; the frequency and length of `tone`, `beep`, `beeps`, `slide` of `Player` (different on every call) |
| cannot change | the speed, transposition and tone of a converted sound: in the data (`--speed`, `--transpose`); to change them live, play the notes one by one with `tone()`, e.g. `src/week_5_NEC/assignments/4_piano.cpp` |

**Why most are decided when converting**: the UNO has only 2 KB of RAM and 16 MHz. Scaling, changing colors and resampling are computed once on the computer; computing them for every frame and every sample on the board would be too slow. So when many sets of parameters are needed, convert several files (the macro written several times with different options); the unused ones are not compiled in.

## Shared conventions

- **Compile-time checks**: pins, baud rates and animation targets report errors with `static_assert`; usages that are not recommended print warnings through deprecated members (`Uno::warn::*`, `image::warn::*`, `video::warn::*`, `audio::warn::*`); how messages are written: see [Messages and errors](#messages-and-errors).
- **Names**:
  - folders, files and namespaces are lowercase; abbreviations and proper names keep their form (ST7789, SSD1306, DS18B20, NEC, HX1838, I2C, LED, RGB565, M130T, Uno).
  - names of several words use underscores (`flash_player.h`, `Uno_serial.h`, `remote_17_key.h`).
  - classes, structs and template type parameters use UpperCamelCase.
- **Classes**: declared first, defined after `// ================= definitions =================`; every function is marked `inline` and never written on one line.
- **Generated data** (font, image, video, audio):
  - `source/convert.py` makes `output/*.h`, one file per conversion, all on one level (the data of `--target stream` is in the `#if 0` of the same file, never compiled by the board); the files of macros like `FONT(...)` are converted before compiling by `src/tools/assets.py` with the same `convert.py`, into the same place.
  - `output/selected.h` (everything) and `output/registry.h` (the types of the macros) are both written by `src/tools/assets.py` before every build.
  - `source/check.py` checks that the files of `output/` have the current format and unchanged sources; `convert.py` runs it after every conversion.
  - the data is in `static const` arrays inside inline functions, with the board's flash attribute (UNO: `PROGMEM`): what a program does not use is dropped by the compiler and takes no Flash.
  - the board's numbers (flash size, attribute and header, clock, drawing cycles, buzzer and sample clocks, serial) are never hard-coded in the converters: they are in `board.json` in the board's folder, and `convert.py --board …` reads `custom_board` of `platformio.ini` by default; `src/tools/assets.py` passes PlatformIO's setting to it, and `check.py` checks that the `// board:` of every output has its hash.
- **Python**: run with plain `python`; a script that imports other files of the project has `sys.dont_write_bytecode = True` as its first line (before any import), so no `__pycache__` is made.
- **How programs (`src/`) are written**: the settings, parts, sections and names at the top, see [src/README.md](../src/README.md#how-a-program-is-written).

## Compiling

- **C++17**: `platformio.ini` replaces the default `-std=gnu++11` with `-std=gnu++17`.
- **`-mcall-prologues`**: saving registers at the start and end of functions uses shared code: about 1.5 KB smaller, a few cycles more per call.
- **`inline`**: makes no difference to the size whether written or not (measured exactly the same); with LTO the compiler decides by itself what to inline.
- **`lib_deps`**: `platformio.ini` lists every folder of `lib/` directly (add new libraries there too), so a program in any subfolder of `src/` compiles (PlatformIO's dependency finder skips some subfolders).
