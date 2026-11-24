# week_6_ultrasonic

English | [中文](讀我.md)

Tests of the HC-SR04 ultrasonic ranging module ([lib/ultrasonic](../../lib/ultrasonic/README.md)), and assignments adding a switch ([lib/switch](../../lib/switch/README.md)) and a passive buzzer ([lib/buzzer](../../lib/buzzer/README.md)).

Every file is `.bak` now; keep the one to run:

```bash
python src/hide.py week_6_ultrasonic/tests/1_distance     # keep only this one (drop its .bak), the others become .bak
pio run -t upload
pio device monitor                      # at BAUD (= monitor_speed of platformio.ini)
```

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_distance.cpp` | measures the distance every `MEASURE_MS`, prints cm, Echo's µs and a bar | HC-SR04 |
| `tests/2_median.cpp` | each measurement printed next to the median of the last `MEDIAN_COUNT` (`ultrasonic::Median`) | HC-SR04 |
| `assignments/1_parking_sensor.cpp` | assignment 1: sounds while the switch is held and the distance is under `NEAR_MM`, the volume inversely proportional to the distance | switch module + HC-SR04 + passive buzzer |
| `assignments/2_theremin.cpp` | assignment 2: sounds at distances of `MIN_MM` ~ `MAX_MM`, the pitch (the logarithm of the frequency) linear in the distance | HC-SR04 + passive buzzer |

## Wiring

| Part | Pin | UNO | Which programs |
|---|---|---|---|
| HC-SR04 | VCC | 5V | all |
| | Trig | **`TRIG_PIN`** | |
| | Echo | **`ECHO_PIN`** | |
| | GND | GND | |
| switch module | OUT | **`SWITCH_PIN`** | `assignments/1_parking_sensor` |
| | VCC, GND | 5V, GND | |
| passive buzzer | + | 100 Ω → **`BUZZER_PIN`** | `assignments/1_parking_sensor`, `assignments/2_theremin` |
| | - | GND | |
| USB | D0, D1 | serial | all |

| Pin | The rule |
|---|---|
| `ECHO_PIN` | Echo uses a pin change interrupt, one per group (D0 ~ D7, D8 ~ D13, A0 ~ A5): not in the group of the infrared receiver's `IR_PIN` (week_5_NEC), so both can be used together |
| `TRIG_PIN` | Trig is only an output, any pin works; best one the parts of other weeks do not use |
| `SWITCH_PIN` | the same switch wiring as week_4_SSD1306_switch_buzzer_audio; the switch is only polled, no interrupt, so it may share a group with `IR_PIN` |
| `BUZZER_PIN` | D9 or D10: Timer1 drives the passive buzzer directly, the volume can be set (the width of the PWM), with no CPU taken |

# tests/1_distance.cpp: ultrasonic ranging

```
HC-SR04: Trig D7, Echo D8; sound at 346.5 m/s
Echo idle: LOW (ok)
 23.4 cm  echo  1351 us  |###########
100.2 cm  echo  5786 us  |##################################################>
too far / nothing (> 400 cm), or a soft / slanted surface
```

- One `#` is `BAR_CM_PER_CHAR` cm, up to `BAR_CM`; farther ends with `>`.
- `AIR_TENTHS_C`: the room's temperature (0.1 °C), for the speed of sound.
- **`no echo` all the time**: no power, or Trig and Echo swapped (the power-on line then says `Echo idle: HIGH`).
- **`too far` all the time, aimed at something far**: normal; aimed at cloth, sponge or a slanted wall too.
- Compare with a ruler: it should be within a few mm.

# tests/2_median.cpp: the median filter

```
HC-SR04 Trig D7, Echo D8, median of 5
raw   234 mm   median   233 mm  (5/5 good)
raw  1873 mm   median   234 mm  (5/5 good)      <- one wrong echo: raw jumped, median did not move
raw  too far   median   234 mm  (4/5 good)
```

- **raw**: this measurement; **median**: the median of the last `MEDIAN_COUNT` good measurements; **good**: how many of them were good.
- With fewer than half good, median prints `-` (nothing in front, or a wiring problem).
- When the hand moves fast, median catches up about `MEDIAN_COUNT / 2` measurements late: the bigger `MEDIAN_COUNT`, the steadier and the slower.
- `ONLY_CHANGES = true`: prints only when the median changed by 1 cm or more.

# assignments/1_parking_sensor.cpp: a parking sensor while the switch is held

While the switch is held and the thing in front is nearer than `NEAR_MM`, the buzzer sounds at `TONE_HZ`. The volume is inversely proportional to the distance:

```
volume = MAX_VOLUME × FULL_MM ÷ distance      (within FULL_MM, < 2 cm included, it is always MAX_VOLUME, the loudest)
```

- **Half the distance, twice the volume**: that is "inversely proportional"; at twice `FULL_MM` it is half of `MAX_VOLUME` (-6 dB). The volume is the pulse width of `UnoTone` (0 ~ 255), see [lib/buzzer](../../lib/buzzer/README.md#volume).
- **Switch released, `NEAR_MM` or farther, nothing measured (no echo)**: all silent.
- **Always measuring**: held or not, it measures every 60 ms, so pressing gives sound at once; the volume is updated every 60 ms, with no pops when it changes.
- The serial prints a line every `PRINT_MS`: `pressed   23.4 cm  volume 22  (sounding)`.
- The constants to change are in the settings at the top of the file: `NEAR_MM`, `FULL_MM`, `MAX_VOLUME`, `TONE_HZ`, `AIR_TENTHS_C`, and every pin.
- **The switch**: `SWITCH_ACTIVE_HIGH` is `false` for a module whose OUT is LOW when pressed (the module measured here), `true` for one that is HIGH when pressed.

# assignments/2_theremin.cpp: the pitch set by the distance

It sounds at distances between `MIN_MM` and `MAX_MM`, **the pitch linear in the distance**. The pitch is the logarithm of the frequency, so each equal move changes the pitch by the same interval (like moving evenly along a keyboard), not by the same Hz:

```
t  = (distance - MIN_MM) ÷ (MAX_MM - MIN_MM)          0 at MIN_MM, 1 at MAX_MM
hz = HZ_AT_MIN × (HZ_AT_MAX ÷ HZ_AT_MIN)^t        log(hz) goes linearly from log(HZ_AT_MIN) to log(HZ_AT_MAX)
```

| Parameter | Explanation |
|---|---|
| `MIN_MM` | the nearest distance that sounds; the HC-SR04 measures 20 ~ 4000 mm |
| `MAX_MM` | the farthest distance that sounds |
| `HZ_AT_MIN` | the frequency at `MIN_MM` |
| `HZ_AT_MAX` | the frequency at `MAX_MM`; higher than `HZ_AT_MIN` makes it higher the farther |
| `VOLUME` | 0 ~ 255 |

- **How far apart the notes are**: the range spans log2(the higher ÷ the lower of `HZ_AT_MIN`, `HZ_AT_MAX`) octaves, so an octave is (`MAX_MM` − `MIN_MM`) ÷ that many mm, a semitone a twelfth of it; the Hz are not evenly spaced, but they sound evenly spaced.
- **Outside the range or nothing measured**: silent.
- It measures every 60 ms, with no pops when the pitch changes; keep the hand steady: the HC-SR04 itself is a few mm off (about a quarter of a semitone).
- The serial prints a line every `PRINT_MS`: `23.4 cm  775 Hz`.
- Compile-time checks: `MIN_MM < MAX_MM`, within 20 ~ 4000 mm, frequencies within 31 ~ 20000 Hz; otherwise a compile error.
