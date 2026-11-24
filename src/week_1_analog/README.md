# week_1_analog

English | [中文](讀我.md)

Tests of the analog sensors ([lib/analog](../../lib/analog/README.md)): knob, photoresistor, joystick, NTC thermistor, sound module, plus the 5V and temperature the UNO chip measures itself.

Every file is `.bak` now; keep the one to run:

```bash
python src/hide.py week_1_analog/tests/1_sensors     # keep only this one (drop its .bak), the others become .bak
pio run -t upload
pio device monitor                      # at BAUD (= monitor_speed of platformio.ini)
```

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_sensors.cpp` | five analog sensors and the chip's 5V and temperature: a line every `PRINT_MS`, events printed at once | as many as you have |

## Wiring

| Part | Pins | UNO |
|---|---|---|
| knob | sides, middle | 5V, GND; **`KNOB_PIN`** |
| photoresistor module | S, +, - | **`LIGHT_PIN`**, 5V, GND |
| joystick KY-023 | VRx, VRy, SW, +5V, GND | **`STICK_X_PIN`**, **`STICK_Y_PIN`**, **`STICK_BUTTON_PIN`**, 5V, GND |
| NTC module KY-013 | S, middle, - | **`NTC_PIN`**, 5V, GND |
| sound module KY-037 / KY-038 | AO, +, G | **`SOUND_PIN`**, 5V, GND |

- Each may be left unconnected: those not connected only read noise.
- The pins are the `*_PIN` of the settings; analog ones only on A0 ~ A5.

# tests/1_sensors.cpp: analog sensors

A line every `PRINT_MS`, and events printed at once:

```
Vcc 4987 mV, chip 31.2 C
knob 42%  light 63%  stick 0,0  24.8 C  sound 3
joystick -> up
knob turned: 55 %
clap!
dark: night light on
```

- **Joystick**: do not touch it at power-on: it remembers the center.
- **Photoresistor**: `BRIGHTER_IS_HIGHER` is `false` for a module (the brighter, the lower the reading), `true` for a bare photoresistor.
- **Night light**: `DARK_ON`, `DARK_OFF`: prints `dark` when it gets dark, `light again` when it is bright again (tune the values for the room).
- **Sound**: clapping at the microphone prints `clap!`; `sound` is the largest swing since the last line.
- **Vcc, chip**: the real 5V and chip temperature measured by `Uno::UnoChip` (the chip is warmer than the room: for reference only).
