# SSD1306

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Bus` (I2C): `init()`, `start(address, read)`, `write8()`, `stop()`… (`Uno::UnoI2C<SCL, SDA>`) | `SSD1306::OLED128x64`, `OLED128x32`: a Screen (`graphics`, `image`, `video` all draw on it), a whole screen in RAM, `display()` sends only what changed; the panel's own commands (contrast, scrolling, fade-out…) |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

The driver of I2C OLED screens with the SSD1306 controller, `namespace SSD1306`. It is a Screen itself, and the functions of `graphics`, `image` and `video` are exactly the same as on the ST7789.

References: Solomon Systech SSD1306 datasheet Rev 1.1 (8.1.5 I2C interface, chapter 9 command table Table 9-1, chapter 10 command descriptions), and the charge pump application note that comes with the datasheet (command 8Dh).

## Usage

```cpp
#include "Uno_I2C.h"
#include "SSD1306.h"              // include only the entry: it includes basic/ and every module
#include "graphics.h"
#include "font.h"
#include "color.h"

constexpr uint8_t SCL_PIN = 13;                              // any two pins
constexpr uint8_t SDA_PIN = 11;
using Bus = Uno::UnoI2C<SCL_PIN, SDA_PIN>;
using ScreenColor = color::BW<128>;                          // black and white: brightness ≥ 128 is lit
using Screen = graphics::Canvas<SSD1306::OLED128x64<Bus, ScreenColor>>;
Screen screen;                                               // a module at 0x3D: Screen screen(0x3D);

void setup() {
    if (!screen.init()) {                                    // false: no answer at 0x3C
        return;
    }
    screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
    screen.drawHollowRectSize(0, 0, 128, 64, ScreenColor::WHITE);
    screen.drawTexts(10, 26, ScreenColor::WHITE, "hello ", 42);
    screen.display();                                        // sent only when everything is drawn
}
```

- **`display()` after drawing**: shapes and text only change the screen in the board's RAM; `display()` sends the changed parts to the screen.
- **Colors**: the color format is always the 1-bit `color::BW<threshold>` (a color format is a compile error); draw with `ScreenColor::WHITE` / `BLACK` (= `ON` / `OFF`). Colored pictures and animations light each point by its brightness and the threshold.
- **Pictures**: `lib/image` with `--palette 2` (black and white), see [image](../image/README.md#drawing-on-the-oled).
- **Videos**: `video::FlashPlayer<Screen>` plays directly; convert with `--mono`; call `display()` after each frame.

Complete examples: `src/week_4_SSD1306_switch_buzzer_audio/tests/1_oled.cpp` (every kind of command), `src/week_4_SSD1306_switch_buzzer_audio/assignments/2_animation.cpp` (video).

## Wiring

| OLED | UNO |
|---|---|
| GND | GND |
| VCC | 5V or 3.3V (the module has a regulator) |
| SCL | any digital pin, e.g. D13 |
| SDA | any digital pin, e.g. D11 |

- **Any pins**: `Uno::UnoI2C` is software I2C. The UNO's hardware I2C is only on A4 / A5 and not used here.
- **Pull-up resistors**: the module usually has 4.7 ~ 10 kΩ already; `UnoI2C` also turns on the UNO's internal pull-ups of about 35 kΩ by default.
- **Address**: most modules are 0x3C (the back of the module often says 0x78, the value shifted left by one); with SA0 on VCC it is 0x3D.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| `WIDTH` of `Frame` | 1 ~ 128 | compile error |
| `HEIGHT` of `Frame` | 8 ~ 64, a multiple of 8 | compile error |
| coordinates of shapes, text, `fillWindow`, `writeWindow` | anything | clipped |
| width or height | > 0 | not drawn |
| command arguments | see the command table | extra bits are masked (e.g. a page keeps only the low 3 bits); `setMultiplexRatio`, `setClock`, `setPrecharge` clamped |
| no answer at the address | — | returns an error: `init()`, `display()`, `readStatus()`, `ok()` return false, never hang |
| the array of `writeWindow` | at least w × h | not checked |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| all black | `init()` failed | check VCC / GND, SCL / SDA swapped; the address may be 0x3D |
| only the upper half, every other row | it is really a 128×32 panel | use `SSD1306::OLED128x32` |
| mirrored left-right or upside down | the module is mounted the other way | `init(true)` turns it by 180° |
| 2 columns of noise on the right, everything shifted | the controller is really an SH1106 (common at 1.3 inch, 132 columns, no 20h ~ 22h) | not supported |
| garbled now and then | wires too long or pull-ups too weak | slow down: `Uno::UnoI2C<SCL_PIN, SDA_PIN, true, 100>` |
| calling `display()` while scrolling | the datasheet says not to change the RAM while scrolling | not checked: `deactivateScroll()` first, then `displayAll()` |
| on the same D11 / D13 as the M130T | two screens cannot share the same pins | move the OLED to other pins |
| not enough RAM | one OLED takes 1 KB, the UNO has only 2 KB | keep other objects inside functions as much as possible (returned when done) |
