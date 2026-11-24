# switch

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Pin`: `init()`, `read()`, `nowMs()` (`Uno::UnoInputPin<PIN>`) | `switches::Switch2Pins`, `Switch3Pins`: debounced switches (on or not, just pressed, how many presses, for how long); `switches::Clicks`: click, double click, long press, repeat while held |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Switches, `namespace switches` (`switch` is a C++ keyword and cannot be a namespace).

## Usage

```cpp
#include "Uno_pin.h"
#include "switch.h"

constexpr uint8_t SWITCH_PIN = 2;                 // the module's OUT
switches::Switch3Pins<Uno::UnoInputPin<SWITCH_PIN>> button;
// switches::Switch2Pins<Uno::UnoInputPin<SWITCH_PIN, true>> button;   // a bare 2-pin button: on SWITCH_PIN and GND

void setup() {
    button.init();
}

void loop() {
    button.update();                              // call it every loop, it never waits
    if (button.wasPressed()) {
        // just pressed (true only once per press)
    }
}
```

Complete examples: `src/week_4_SSD1306_switch_buzzer_audio/tests/2_switch.cpp` (the state as text on the serial), `src/week_4_SSD1306_switch_buzzer_audio/assignments/1_photo_switch.cpp` (one press, the next photo).

## Clicks: one button as several buttons

```cpp
constexpr uint8_t BUTTON_PIN = 2;
switches::Switch2Pins<Uno::UnoInputPin<BUTTON_PIN, true>> button;
switches::Clicks<decltype(button)> clicks(button);

void setup() { clicks.init(); }            // inits the button too
void loop() {
    switch (clicks.update()) {             // only call this (it calls the button's update)
        case switches::CLICK:        /* one press */ break;
        case switches::DOUBLE_CLICK: /* two presses */ break;
        case switches::LONG_PRESS:   /* held 0.8 seconds */ break;
        default: break;
    }
}
```

| Event | When |
|---|---|
| `CLICK` | pressed and released once, and no other press within `doubleMs` (300 ms) |
| `DOUBLE_CLICK` | two quick presses |
| `MULTI_CLICK` | 3 or more quick presses; `count()` is how many |
| `LONG_PRESS` | held for `longMs` (800 ms), reported before release |
| `REPEAT` | held on after a long press: once every `repeatMs` (150 ms) (like holding a keyboard key) |
| `LONG_RELEASE` | released after a long press |

| Function | What it does |
|---|---|
| `init()` | inits the button too |
| `update()` | returns the event that just happened, `NOTHING` if none |
| `count()` | how many presses the last click / double click / multi-click had |
| `isPressed()` | whether it is held now |
| `setDoubleMs(ms)` | the longest gap between two presses (default 300; 0 = no double clicks, `CLICK` on every release at once) |
| `setLongMs(ms)`, `setRepeatMs(ms)` | the time of a long press (default 800), the interval of repeats (default 150; 0 = no repeats) |

- **A click comes 300 ms late**: it waits to see if a second press follows. Without double clicks, `setDoubleMs(0)` reports at once.
- Complete example: `src/week_4_SSD1306_switch_buzzer_audio/tests/6_clicks.cpp` (click, double click, long press change the LED effect).

## Wiring

| Module | UNO |
|---|---|
| GND | GND |
| VCC | 5V |
| OUT | any digital pin, e.g. D2 |

A bare 2-pin switch: one side on the pin, the other on GND, with `Switch2Pins<Uno::UnoInputPin<PIN, true>>`.

## Two pins and three pins

| Part | Examples | Wiring | Use |
|---|---|---|---|
| a bare 2-pin switch | buttons, reed switches, tilt switches (ball) | one pin on the pin, one on GND | `Switch2Pins<Uno::UnoInputPin<PIN, true>>`: internal pull-up, HIGH when not pressed, LOW when pressed |
| a 4-pin tactile button | the square button common on breadboards | the four pins are connected in pairs: **take two diagonal ones** as the two pins | the same |
| a 3-pin module (GND, VCC, OUT) | button modules, TTP223 touch, tilt, reed, Hall modules | OUT on the pin; the module drives the level itself | `Switch3Pins<Uno::UnoInputPin<PIN>>`, no pull-up; add `false` for modules that are LOW when pressed |

- **Two pins always need a pull-up**: with the switch open nothing drives the pin, and without a pull-up it reads random noise.
- **ACTIVE_HIGH of three pins**: touch modules and most button modules are HIGH when pressed (the default); some button, tilt and Hall modules are LOW. When unsure, run `src/week_4_SSD1306_switch_buzzer_audio/tests/2_switch.cpp`.

## Debouncing

A mechanical contact bounces back and forth several times in the few ms it touches or separates. Without handling, one press counts as several.

| Situation | Handling |
|---|---|
| the level keeps bouncing | the timer restarts on every bounce, the state does not change |
| the new level stays for `DEBOUNCE_MS` | the state changes, `update()` returns true, the press or release is recorded |
| the cost | every change shows `DEBOUNCE_MS` (20 ms) late |

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| pin of `Pin` | 0 ~ 19; D0 / D1 compile warning (USB serial) | compile error |
| `DEBOUNCE_MS` | 0 ~ 255 ms; 0 = no debouncing | — |
| interval of `update()` | the more often the better | longer than `DEBOUNCE_MS`: very short presses may be missed |
| `pressCount()` | 0 ~ 65535 | starts again from 0 after that |
| `stateMs()` | `millis()` wraps to 0 after about 49 days | differences stay right (unsigned subtraction) |
