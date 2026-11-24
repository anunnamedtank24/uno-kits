# week_2_LED

English | [中文](讀我.md)

Tests of LEDs ([lib/LED](../../lib/LED/README.md)): how brightness is converted, every effect (blinking, breathing, fading, heartbeat).

Every file is `.bak` now; keep the one to run:

```bash
python src/hide.py week_2_LED/tests/1_effects     # keep only this one (drop its .bak), the others become .bak
pio run -t upload
pio device monitor                      # at BAUD (= monitor_speed of platformio.ini)
```

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_effects.cpp` | one LED: brightness steps, blinking, breathing, fading, heartbeat, one after another | LED + 220 Ω |

Other programs using LEDs: `week_4_SSD1306_switch_buzzer_audio/tests/6_clicks` (button gestures change the effect), `week_5_NEC/assignments/1_rgb_remote` (a remote sets red, green, blue).

## Wiring

| LED | UNO |
|---|---|
| long leg (+) | 220 Ω → **`LED_PIN`** |
| short leg (-) | GND |

- **Always a resistor in series**: an LED does not limit its current, and straight on a pin it burns the LED or the pin.
- **`LED_PIN` must be a PWM pin** (3, 5, 6, 9, 10, 11) for brightness.

# tests/1_effects.cpp: every effect of an LED

```
LED on D6
test 0: steady steps
  steady   0
  steady  31
  ...
test 1: blink
  done
test 2: breathe
test 3: fade
test 4: heartbeat
```

| Test | What you should see |
|---|---|
| 0 brightness steps | 0 ~ 255 in `STEPS` steps, each looking a bit brighter than the one before, and evenly |
| 1 blinking | blinks `BLINK_TIMES` times, then off |
| 2 breathing | brighter and dimmer smoothly, one breath per `BREATHE_MS` |
| 3 fading | from dark to brightest within `FADE_MS`, then back to dark |
| 4 heartbeat | two short beats, a rest, repeated |

- **The brightness is as the eye sees it**: 0 ~ 255 looks evenly brighter, so 128 looks "half as bright", while the PWM duty cycle is really only 25 % (duty = brightness² ÷ 255 = 64).
- **Effects never wait**: `effect.blink(...)` returns at once, and `effect.update()` must be called all the time (which is what `run(ms)` does here).
- **Running only some**: `ONLY_TESTS`, e.g. `(1UL << 2)` runs only breathing.
