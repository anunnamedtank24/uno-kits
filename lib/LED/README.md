# LED

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Pwm`: `init()`, `write(duty)` (`Uno::UnoPwmPin<PIN>`) | `LED::LED2Pins`, `RGBLED4Pins`, `TrafficLight4Pins`: brightness 0 ~ 255 as the eye sees it; `LED::Effect`: blinking, breathing, fading, heartbeat |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

LEDs with brightness by PWM, `namespace LED`. The brightness is converted for how the eye sees it (0 ~ 255 looks evenly brighter), not used directly as the duty cycle.

## Usage

```cpp
#include "Uno_pin.h"
#include "LED.h"

constexpr uint8_t RGB_RED_PIN = 3;                     // all must be PWM pins: 3, 5, 6, 9, 10, 11
constexpr uint8_t RGB_GREEN_PIN = 5;
constexpr uint8_t RGB_BLUE_PIN = 6;
constexpr uint8_t LIGHT_RED_PIN = 9;
constexpr uint8_t LIGHT_GREEN_PIN = 10;
constexpr uint8_t SINGLE_PIN = 11;

LED::RGBLED4Pins<Uno::UnoPwmPin<RGB_RED_PIN>, Uno::UnoPwmPin<RGB_GREEN_PIN>, Uno::UnoPwmPin<RGB_BLUE_PIN>> rgb;
LED::TrafficLight4Pins<Uno::UnoPwmPin<LIGHT_RED_PIN>, LED::NoPwm, Uno::UnoPwmPin<LIGHT_GREEN_PIN>> trafficLight;   // red and green only
LED::LED2Pins<Uno::UnoPwmPin<SINGLE_PIN>> single;

void setup() {
    rgb.init();
    rgb.set(255, 128, 0);                                  // orange
    rgb.setBlue(LED::levelToBrightness(3, 10));            // blue at step 3 (of 10)
    trafficLight.init();
    trafficLight.setGreen(255);
    single.init();
    single.set(128);                                       // looks half as bright
}
```

Complete example: `src/week_5_NEC/assignments/1_rgb_remote.cpp` (a remote sets the red and green of a traffic light and a blue LED).

## Effect: blinking, breathing, fading, heartbeat

```cpp
constexpr uint8_t LED_PIN = 6;
LED::LED2Pins<Uno::UnoPwmPin<LED_PIN>> light;
LED::Effect<decltype(light)> effect(light);

effect.breathe(2000);          // one breath every 2 seconds
// in loop: effect.update();
```

| Function | What it does |
|---|---|
| `steady(brightness)` | a fixed brightness |
| `blink(onMs, offMs, count, brightness)` | blinking; off after `count` times (0 = forever) |
| `breathe(periodMs, low, high)` | brighter and dimmer smoothly between low and high, one breath per periodMs |
| `fadeTo(brightness, ms)` | fades from the current brightness to brightness |
| `heartbeat(periodMs, brightness)` | two short beats, a rest (a heartbeat) |
| `update()` | call it every loop |
| `kind()`, `isRunning()` | which effect runs now; false after a counted blink or a fade has ended |

- **The brightness is as the eye sees it** (0 ~ 255, see `brightness.h`), so breathing looks even.
- For one color (`SingleLED`, `LED2Pins`); the colors of an RGB LED change together with `set(r, g, b)`.
- Complete examples: `src/week_2_LED/tests/1_effects.cpp` (each effect in turn), `src/week_4_SSD1306_switch_buzzer_audio/tests/6_clicks.cpp` (button gestures change the effect).

## Wiring

**A bare LED always needs a resistor in series** (220 ~ 330 Ω): an LED does not limit its current, and straight on a pin it burns the LED or the pin. Modules that already have resistors (traffic light modules, some RGB modules) need none.

One LED:

| LED | UNO |
|---|---|
| long leg (+) | 220 Ω → a PWM pin, e.g. D3 |
| short leg (-) | GND |

RGB LED (4 legs, the longest is the common one):

| RGB LED | Common cathode | Common anode |
|---|---|---|
| R | 220 Ω → D3 | 220 Ω → D3 |
| the longest leg | GND | 5V |
| G | 220 Ω → D5 | 220 Ω → D5 |
| B | 220 Ω → D6 | 220 Ω → D6 |
| parameter | `COMMON_ANODE = false` (default) | `COMMON_ANODE = true` |

- **Common cathode or anode**: if it lights with the longest leg on GND and another leg through a resistor on 5V, it is common cathode; if it needs the opposite, common anode. Modules marked `-` (KY-016 and others) are common cathode.
- **PWM pins**: only D3, D5, D6, D9, D10, D11 of the UNO can set brightness; other pins are a compile error.
- **Module boards**: some RGB modules have resistors soldered on already and need no more.
A traffic light module (4 pins: R, Y, G, GND, resistors already on the board):

| Traffic light | UNO |
|---|---|
| R | D3 (directly, no resistor) |
| Y | D5 (an unused color stays unconnected, the program gives `LED::NoPwm`) |
| G | D6 |
| GND (or -) | GND |

- **Three separate LEDs work too**: the long legs of red, green and blue each through 220 Ω to D3, D5, D6, the short legs all on GND, is the same as a common-cathode RGB LED, with no change to the program. The difference: the light comes from three points and does not mix into one color up close; put them together under a sheet of white paper and they mix.

## Brightness and PWM

**PWM**: the pin switches on and off very fast (about 976 Hz on the UNO); the longer the share of "on" (the duty cycle), the brighter it looks. The eye cannot follow such fast blinking, so it sees the average brightness.

**Why convert**: the eye is more sensitive to changes in the dark: a duty cycle of 50% looks about 70% bright. Set by the duty cycle directly, the first half changes a lot and the second half barely shows.

| Brightness (`set`) | Duty cycle (`toDuty`) | Looks |
|---|---|---|
| 0 | 0 | off |
| 64 | 16 (6%) | about a quarter |
| 128 | 64 (25%) | about half |
| 255 | 255 (100%) | brightest |

- **Steps**: `levelToBrightness(level, levels)` spreads 0 ~ levels evenly over 0 ~ 255; with `toDuty`, each step looks about the same size.
- **The darkest steps**: the duty cycle is a whole number, so brightness 1 ~ 15 gives 0 ~ 1: off or very dim.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| pins | 3, 5, 6, 9, 10, 11 | compile error |
| brightness, duty cycle | 0 ~ 255 | — |
| level of `levelToBrightness` | 0 ~ levels | above levels it is 255; with levels = 0 it is 0 |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| no resistor | too much current: the LED or the pin burns | 220 ~ 330 Ω for each color |
| one resistor for all three colors (on the common leg) | the colors affect each other, red is brighter (the lowest drop) | one for each color |
| LEDs on D9 / D10 together with a passive buzzer (`UnoTone`) | both fight over Timer1: brightness and pitch both wrong | move the LEDs to D3, D5, D6, D11 |
| LEDs on D3 / D11 together with Arduino's `tone()` | they fight over Timer2 | do not use `tone()` (use `lib/buzzer`) |
| a common anode set as common cathode | bright and dark swapped: 0 is brightest | change `COMMON_ANODE` |
| one pin driving many LEDs | ≤ 20 mA a pin recommended | use a transistor |
