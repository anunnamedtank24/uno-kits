# ultrasonic

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| `Trigger`: an output pin (`Uno::UnoOutputPin<PIN>`); `Echo`: an input pin with an interrupt (`Uno::UnoInterruptPin<PIN>`) | `ultrasonic::HCSR04`: distance (mm), measuring without waiting, result codes, temperature correction; `ultrasonic::Median`: a steadier distance |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Ultrasonic ranging modules, `namespace ultrasonic`.

## Usage

```cpp
#include "Uno_pin.h"
#include "Uno_interrupt_pin.h"
#include "ultrasonic.h"

constexpr uint8_t TRIG_PIN = 7;
constexpr uint8_t ECHO_PIN = 8;
using Ranger = ultrasonic::HCSR04<Uno::UnoOutputPin<TRIG_PIN>, Uno::UnoInterruptPin<ECHO_PIN>>;

void setup() {
    Ranger::init();
    Ranger::setTemperature(250);      // 25.0 °C (optional, 20.0 °C by default)
}

void loop() {
    if (Ranger::update()) {           // call it every loop, it never waits; true when a measurement is done
        if (Ranger::result() == ultrasonic::RESULT_OK) {
            uint16_t mm = Ranger::distanceMm();
        }
    }
}
```

Everything is static (an interrupt has no object to call), so use `Ranger::` without declaring a variable; one Ranger per Echo pin. Complete example: `src/week_6_ultrasonic/tests/1_distance.cpp`.

## Median: a steadier distance

Now and then one wrong echo (reflected off a slope, missed) makes the distance jump. `Median` takes the median of the last N good measurements, so one or two wrong ones do not change the result.

```cpp
using Sensor = ultrasonic::HCSR04<Uno::UnoOutputPin<TRIG_PIN>, Uno::UnoInterruptPin<ECHO_PIN>>;
ultrasonic::Median<Sensor, 5> distance;

distance.init();                        // inits the sensor too
// in loop:
if (distance.update() && distance.isValid()) {
    uint16_t mm = distance.distanceMm();
}
```

| Function | What it does |
|---|---|
| `init()` | inits the sensor too |
| `update()` | calls the sensor's `update()`; returns true when there is a new measurement |
| `distanceMm()` | the median of the last N good measurements |
| `goodCount()` | how many of the last N were `RESULT_OK` |
| `isValid()` | at least half are good (otherwise there may be nothing in front) |

- `N` is an odd number 3 ~ 15 (default 5); the bigger the steadier, but N / 2 measurements slower (N = 5 about 150 ms).

## Wiring

| HC-SR04 | UNO | Explanation |
|---|---|---|
| VCC | 5V | the HC-SR04 needs 5 V (the HC-SR04P takes 3 ~ 5.5 V) |
| Trig | any digital pin, e.g. **D7** | `Uno::UnoOutputPin<TRIG_PIN>` |
| Echo | any digital pin, e.g. **D8** | `Uno::UnoInterruptPin<ECHO_PIN>`; outputs 5 V, the UNO takes it directly |
| GND | GND | |

- **Echo is measured by an interrupt**: a group can have only one pin change interrupt (D0 ~ D7, D8 ~ D13, A0 ~ A5); the infrared receiver is on D2, so Echo goes on D8 and both can be used together.
- Keep the front clear: the nearest thing in a cone of about 15° in front of the transducers is the distance measured.

## Names

| Name | What it is |
|---|---|
| **HC-SR04** | the module's model (the part), in `module/HC_SR04.h` |
| chip | none in common: boards by different makers use different chips (EM78P153, RCWL-9206, CS100A…), but the pins and usage are all the same |
| **Trig / Echo** | the usage (the way of communicating): a pulse on Trig starts a measurement, and how long Echo is HIGH is the sound's round trip |

By the rule "`lib/` is named after the chip or the way of communicating", there is no common chip here, and Trig / Echo is no protocol with a name, so like `switch`, `buzzer`, `LED` it is named after the kind of part: `ultrasonic/`. Other Trig / Echo modules (the JSN-SR04T waterproof probe, the US-100) will go into `module/`.

## How it works

```
Trig  ──┐▔▔┐────────────────────────────────────    HIGH for ≥ 10 µs: start one measurement
Sound      ))))))))                    ((((((((      8 pulses of 40 kHz ultrasound (about 0.2 ms), bouncing back off something
Echo  ───────────┐▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┐──────────    HIGH when sent, LOW when the echo is heard
                 └─── round-trip time ──┘
```

- **Distance** = Echo's HIGH time × the speed of sound ÷ 2 (there and back). At 20 °C sound travels 343 m/s: **every 58 µs is 1 cm**.
- **No echo**: Echo goes back LOW by itself after about 38 ms (some copies take longer, see `RESULT_BUSY`).
- **At least 60 ms between two**: the last ultrasound may still bounce around the room, and measuring too soon takes an old echo for a new one.

## Results

| Result | Meaning | Common causes |
|---|---|---|
| `RESULT_OK` | measured: `distanceMm()` is the distance | |
| `RESULT_TOO_NEAR` | nearer than `MIN_MM` (2 cm) | the echo came back while the module was still sending |
| `RESULT_TOO_FAR` | no echo by `MAX_MM` (400 cm) | nothing in front; cloth or sponge absorbs the sound; a slanted surface reflects the sound elsewhere |
| `RESULT_NO_ECHO` | Echo never went HIGH | no power, Trig and Echo swapped, a bad connection |
| `RESULT_BUSY` | Echo was still HIGH when measuring should start | some copies keep Echo HIGH a long time without an echo; skipped this time, tried again at the next interval |
| `RESULT_NONE` | nothing measured yet | |

## Accuracy

| Source of error | Size | What to do |
|---|---|---|
| `nowUs()` in steps of 4 µs | about 0.7 mm | — |
| interrupt latency (another interrupt running, e.g. `millis`) | a few µs, about 1 mm | — |
| air temperature | about 1.7 % every 10 °C (1.7 cm at 1 m) | `setTemperature`, which can come from a DS18B20 |
| the module itself | about 3 mm; the smaller and more slanted the surface, the worse | take the median of several |

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| pins of Trigger, Echo | 0 ~ 19; D0 / D1 compile warning (USB serial) | compile error |
| `MIN_MM < MAX_MM`, `MAX_MM` ≤ 10000 | | compile error |
| `setIntervalMs` | 60 ~ 65535 ms | below 60 taken as 60 |
| `setTemperature` | -400 ~ 800 (-40 ~ 80 °C) | clamped |
| `N` of `Median` | an odd number 3 ~ 15 | compile error |
| another `UnoInterruptPin` in the same group (D0 ~ D7, D8 ~ D13, A0 ~ A5) | | the one `init`ed later replaces the other |
