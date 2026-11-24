# week_8_TCRT5000

English | [中文](讀我.md)

Tests of the HW-006 line tracking board (TCRT5000, [lib/TCRT5000](../../lib/TCRT5000/README.md)).

Every file is `.bak` now; keep the one to run:

```bash
python src/hide.py week_8_TCRT5000/tests/1_line_tracking     # keep only this one (drop its .bak), the others become .bak
pio run -t upload
pio device monitor                      # at BAUD (= monitor_speed of platformio.ini)
```

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_line_tracking.cpp` | reflecting / dark printed to the serial | HW-006 |

## Wiring

| Part | Pin | UNO |
|---|---|---|
| HW-006 | VCC | 5V |
| | GND | GND |
| | OUT | **`OUT_PIN`** |
| USB | D0, D1 | serial |

| Pin | The rule |
|---|---|
| `OUT_PIN` | it only reads high and low, any pin works; best one that clashes with none of the parts of other weeks (L298N, HC-SR04, infrared, OLED, buzzer) |

# tests/1_line_tracking.cpp: the HW-006 line tracking board

```
HW-006 on D4: white / near = reflecting, black / far = dark
reflecting  (OUT LOW,  change 0)
dark        (OUT HIGH, change 1)    was reflecting 1250 ms
still dark        (OUT HIGH, change 1)
```

1. The front of the module facing down, 1 ~ 2 cm over white paper: it should be `reflecting`, with the signal LED on the HW-006 on.
2. Moved over black tape or ink (or lifted away): it turns `dark`, the signal LED off.
3. Nothing changes: turn the blue potentiometer until white turns it on and the black line turns it off.
4. **The other way round** (white paper is `dark`): this board's OUT is HIGH when reflecting: `ACTIVE_LOW` must be `false` (`true` for the usual board).
5. Every `STATUS_MS` a line `still …`, so the current level shows even when nothing changes.
