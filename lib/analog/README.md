# analog

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| `AnalogPin`: `init()`, `readAnalog()`… (`Uno::UnoAnalogPin<A0>`); the joystick's button also needs a `Pin` (`Uno::UnoInputPin<PIN, true>`) | `analog::Potentiometer`, `Photoresistor`, `KY023` (joystick), `KY013` (temperature), `KY037` (sound): smoothed values, percent or any range, thresholds like a switch, temperature, loudness and claps |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Analog sensors, `namespace analog`. Parts whose output is "a voltage that keeps changing", read as 0 ~ 1023 by the UNO's ADC.

## Usage

```cpp
#include "Uno_analog_pin.h"
#include "Uno_pin.h"
#include "analog.h"

constexpr uint8_t KNOB_PIN = A0;                  // the knob's middle leg
constexpr uint8_t STICK_X_PIN = A2;               // joystick VRx
constexpr uint8_t STICK_Y_PIN = A3;               // joystick VRy
constexpr uint8_t STICK_BUTTON_PIN = 4;           // joystick SW

analog::Potentiometer<Uno::UnoAnalogPin<KNOB_PIN>> knob;
analog::KY023<Uno::UnoAnalogPin<STICK_X_PIN>, Uno::UnoAnalogPin<STICK_Y_PIN>, Uno::UnoInputPin<STICK_BUTTON_PIN, true>> stick;

void setup() {
    knob.init();
    stick.init();                 // do not touch the joystick now: it remembers the center
}

void loop() {
    knob.update();                // call it every loop (reads once, about 112 µs)
    stick.update();
    uint8_t percent = knob.percent();     // 0 ~ 100
    int32_t angle = knob.map(0, 180);     // 0 ~ 180
    int8_t x = stick.x();                 // -100 ~ 100
}
```

Complete example: `src/week_1_analog/tests/1_sensors.cpp` (five sensors plus the chip's own 5V and temperature).

## Wiring

| Part | Pin | UNO |
|---|---|---|
| knob (3 legs) | the two sides | 5V, GND (swapped only reverses the direction) |
| | the middle (the wiper) | A0 ~ A5 |
| photoresistor module (KY-018 and others, 3 pins) | S / AO | A0 ~ A5 |
| | + / VCC, - / GND | 5V, GND |
| bare photoresistor | one side on 5V, the other on A0 ~ A5, plus 10 kΩ from there to GND | `BRIGHTER_IS_HIGHER = true` |
| joystick KY-023 (5 pins) | VRx, VRy | two of A0 ~ A5 |
| | SW | any digital pin (pressed connects to GND; with the internal pull-up: `Uno::UnoInputPin<PIN, true>`) |
| | +5V, GND | 5V, GND |
| NTC module KY-013 (3 pins) | S | A0 ~ A5 |
| | middle, - | 5V, GND |
| sound module KY-037 / KY-038 (4 pins) | AO | A0 ~ A5 |
| | DO | any digital pin (louder than set by the potentiometer: read it with `Switch3Pins` of `lib/switch`) |
| | +, G | 5V, GND |

## Names

These parts have no common chip and no protocol (it is just a voltage), so like `switch`, `buzzer`, `LED` they are named after the kind of part: `analog/`. Analog parts with a chip (e.g. the 4-pin TCRT5000 board) go into the chip's library (`TCRT5000/module/TCRT5000_4_pins.h`), also with AnalogPin.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| pins of AnalogPin | A0 ~ A5 (14 ~ 19) | compile error (`UnoAnalogPin`) |
| `SMOOTHING` | 0 ~ 6 | compile error |
| `setDeadZone` | 0 ~ 90 % | above 90 taken as 90 |
| `setRange` with both equal | | `map` returns outLow |
| readings | 0 ~ 1023 | — |
