# DS18B20

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Wire` (1-Wire bus): `init()`, `reset()`, `write8()`, `read8()`… (`Uno::UnoOneWire<PIN>`) | `DS18B20::TO92`, `WaterproofProbe`, `KY001`: temperature (1/100 °C or float), resolution 9 ~ 12 bits, conversions without waiting, error codes |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

The DS18B20 digital temperature sensor (three pins: VCC, DQ, GND), `namespace DS18B20`, class `DS18B20::Thermometer`. The folder is named after the chip; the bare TO-92, the waterproof probe and the KY-001 module all hold the same DS18B20 with the same code, and `module/` only records how each is wired (they differ in having a pull-up resistor or not).

## Usage

```cpp
#include "Uno_one_wire.h"
#include "DS18B20.h"

constexpr uint8_t DQ_PIN = 2;                     // the sensor's DQ
using SensorWire = Uno::UnoOneWire<DQ_PIN>;
DS18B20::KY001<SensorWire> sensor;                // the board has a pull-up; a bare one: DS18B20::TO92

sensor.init(12);                                  // 12 bits: 0.0625 °C, 750 ms a conversion
int16_t centi;
if (sensor.readCentiCelsius(centi) == sensor.OK) {
    // centi = 2350 means 23.50 °C
}
```

To avoid blocking for 750 ms, do it in steps:

```cpp
sensor.startConversion();
// ……do other things, or keep asking sensor.isConversionDone()
int16_t raw;
if (sensor.readRaw(raw) == sensor.OK) {
    int16_t centi = sensor.toCentiCelsius(raw);
}
```

Complete examples: `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/2_temperature.cpp` (the temperature as text on the serial), `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/assignments/1_temperature_display.cpp` (shown on the screen).

## Wiring

| Sensor | UNO |
|---|---|
| VCC | 5V (3.0 ~ 5.5 V) |
| DQ | any digital pin, e.g. D2 |
| GND | GND |

| Part | Pins | Pull-up resistor | Use |
|---|---|---|---|
| bare DS18B20 (TO-92) | flat side facing you, legs down: GND, DQ, VDD | **add it yourself**: 4.7 kΩ (DQ to VCC) | `DS18B20::TO92` |
| waterproof probe | red VDD, black GND, yellow (or white, blue) DQ; colors depend on the seller, read the description | **add it yourself**, all the more with long wires | `DS18B20::WaterproofProbe` |
| KY-001 and other module boards | as printed on the board: S = DQ, middle = VCC, - = GND | already on the board | `DS18B20::KY001` |

- **DQ needs a pull-up resistor**: 1-Wire relies only on the pull-up to bring the line back HIGH.
- `Uno::UnoOneWire` also turns on the UNO's internal pull-up of about 35 kΩ by default; short wires (within a few tens of centimeters) may work without an external resistor, but this is not guaranteed.

## Resolution

| bits | A step | A conversion | `conversionMs()` |
|---|---|---|---|
| 9 | 0.5 °C | 93.75 ms | 94 |
| 10 | 0.25 °C | 187.5 ms | 188 |
| 11 | 0.125 °C | 375 ms | 375 |
| 12 (default) | 0.0625 °C | 750 ms | 750 |

- The resolution is kept in the sensor's RAM and goes back to 12 bits without power; `init()` sets it again.
- At lower resolutions, the unused low bits of raw are cleared to 0.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| `init(bits)`, `setResolution(bits)` | 9 ~ 12 | returns the error `ERROR_RESOLUTION` |
| measuring range | −55 ~ +125 °C (raw −880 ~ 2000) | the sensor's specification |
| accuracy | ±0.5 °C at −10 ~ +85 °C | the sensor's specification; a higher resolution is not more accurate |
| `toCentiCelsius` | −5500 ~ 12500 | — |
| number of sensors | one a line (Skip ROM, does not tell them apart) | not checked: two or more answer at once, a CRC error or wrong values |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| reading 85.00 °C | the value before any conversion after power-on (`POWER_ON_RAW`) | shows up when read right after power-on or when a conversion was cut by power loss: measure again |
| parasite power (VCC on GND) | not supported: a conversion needs a strong pull-up | connect VCC |
| very long wires | `ERROR_CRC` or `ERROR_NO_SENSOR` | use a smaller pull-up (e.g. 2.2 kΩ) |
| `UnoTone` on a pin other than D9 / D10 at the same time | the square wave pauses while interrupts are off, the sound wobbles | put the buzzer on D9 or D10 |
