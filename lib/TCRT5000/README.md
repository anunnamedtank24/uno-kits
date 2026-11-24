# TCRT5000

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| the digital output (OUT): a `Pin`: `init()`, `read()`, `nowMs()` (`Uno::UnoInputPin<PIN>`) | `TCRT5000::HW006` (`Reflector`): reflecting or dark, just changed, for how long |
| the analog output (A0 of the 4-pin board): `AnalogPin` (`Uno::UnoAnalogPin<A0>`) | `TCRT5000::AnalogReflector`: how strong the reflection is |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

The TCRT5000 reflective infrared sensor (on a small board with a comparator), `namespace TCRT5000`.

## Usage

```cpp
#include "Uno_pin.h"
#include "TCRT5000.h"

constexpr uint8_t OUT_PIN = 4;            // the board's OUT
TCRT5000::HW006<Uno::UnoInputPin<OUT_PIN>> sensor;

void setup() {
    sensor.init();
}

void loop() {
    sensor.update();                       // call it every loop, it never waits
    if (sensor.becameDark()) {
        // just reached a black line (or left a white surface)
    }
}
```

Complete example: `src/week_8_TCRT5000/tests/1_line_tracking.cpp` (the state printed to the serial).

## The 4-pin board: how strong the reflection is (A0)

The TCRT5000 board with four pins (VCC, GND, D0, A0) has one more pin, A0: the voltage of the phototransistor itself. D0 is 0 / 1 like the HW-006 (use `Reflector`); A0 tells **how strong** the reflection is.

| Pin | UNO |
|---|---|
| VCC, GND | 5V, GND |
| A0 | A0 ~ A5 (`Uno::UnoAnalogPin<A0>` ~ `<A5>`) |
| D0 | any digital pin (if wanted, with `Reflector`) |

```cpp
constexpr uint8_t REFLECTION_PIN = A0;                         // the board's A0
TCRT5000::AnalogReflector<Uno::UnoAnalogPin<REFLECTION_PIN>> sensor;
sensor.init();
// in loop:
sensor.update();
uint16_t r = sensor.reflection();       // 0 (no reflection) ~ 1023 (full reflection)
```

| Function | What it does |
|---|---|
| `update()` | reads once (about 112 µs); returns true when `isReflecting()` just changed |
| `reflection()` | 0 ~ 1023, the bigger the more reflection (white, near); already smoothed |
| `percent()` | 0 ~ 100 between the darkest and brightest seen so far (learns by itself) |
| `setThreshold(on, off)` | reflecting when it rises to on, dark when it falls below off (a gap between, so it does not flicker) |
| `isReflecting()`, `isDark()` | the same meaning as for `Reflector` |

- **A0 on the board is inverted**: the more reflection, the lower the voltage; `reflection()` is already turned the right way.
- **Smoother line following**: with two or more sensors, the difference of `reflection()` tells how far off the black line it is, so turns can be bigger or smaller.

## Wiring

| HW-006 | UNO |
|---|---|
| VCC | 5V (3.3 ~ 5 V both fine) |
| GND | GND |
| OUT (some boards print D0) | any digital pin, e.g. **D4**; no pull-up (the board drives it itself) |

## Names

| Name | What it is |
|---|---|
| **TCRT5000** | the sensing part: an infrared LED and a phototransistor side by side, facing the same way (the black part with two round eyes at the front of the board) |
| **LM393** | the comparator on the board: turns how much the phototransistor receives into HIGH / LOW on OUT |
| **HW-006** | the board's model (the part), in `module/HW_006.h`; a line tracking module |

By the rule "`lib/` is named after the chip", what decides how the code is written is the TCRT5000 (what counts as reflecting), so the folder is `TCRT5000/`. The 4-pin board is also in `module/` (`TCRT5000_4_pins.h`); other TCRT5000 boards (KY-033) will go there too.

## How it works

| What is in front | Infrared | OUT | The board's signal LED | `isReflecting()` |
|---|---|---|---|---|
| white paper, a bright surface, within about 1 ~ 25 mm | much comes back | **LOW** | on | true |
| black (tape, ink), too far, nothing | almost nothing comes back | **HIGH** | off | false (`isDark()`) |

- **The blue potentiometer**: sets "how much reflection counts". Turn it until the LED is on over white paper and off over the black line.
- **Line tracking**: a black line absorbs infrared, so `isDark()` is true on the line.
- **Afraid of the sun**: sunlight has a lot of infrared; outdoors or by a window it stays "reflecting".
- It senses "something there or not"; it cannot measure distance.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| pin of `Pin` | 0 ~ 19; D0 / D1 compile warning (USB serial) | compile error |
| `FILTER_MS` | 0 ~ 255 ms | — |
| interval of `update()` | the more often the better; the faster a line-following car, the more often | longer than `FILTER_MS`: a black line passed quickly may be missed |
| `changeCount()` | 0 ~ 65535 | stays at 65535 |
