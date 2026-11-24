# week_5_NEC

English | [中文](讀我.md)

Tests of the infrared remote ([lib/NEC](../../lib/NEC/README.md)), and four assignments driven by the remote: a traffic light module and an LED ([lib/LED](../../lib/LED/README.md)), a square on the OLED, a staff and a buzzer, a piano.

Every file is `.bak` now; keep the one to run:

```bash
python src/hide.py week_5_NEC/tests/1_receiver     # keep only this one (drop its .bak), the others become .bak
pio run -t upload
```

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_receiver.cpp` | prints the codes the remote sends to the serial, to find out which kind your remote is | infrared receiver + remote |
| `tests/2_sound_formats.cpp` | each sound format (notes, rhythm, samples) played once from Flash and once from the computer, the measurements printed by `stream.py` | passive buzzer |
| `tests/3_transmitter.cpp` | infrared **sending**: press keys on the serial, and the UNO sends them as a 17-key remote; the receiver gets them back for checking | infrared LED + receiver |
| `assignments/1_rgb_remote.cpp` | assignment 1: number keys set red, green, blue (red and green of a traffic light module + a blue LED) | receiver + traffic light module + LED |
| `assignments/2_moving_square.cpp` | assignment 2: the arrow keys move a small square on the OLED by 1 pixel | receiver + OLED |
| `assignments/3_staff.cpp` | assignment 3: half notes written on a staff, OK plays them | receiver + OLED + passive buzzer |
| `assignments/4_piano.cpp` | assignment 4: the remote as a piano (0 ~ 9 = A3 ~ C5), recording what is played, OK plays it again | receiver + passive buzzer |

## Wiring

| Part | Pin | UNO | Which programs |
|---|---|---|---|
| infrared receiver (HX1838 / VS1838B / KY-022) | OUT (S) | **`IR_PIN`** | all but `tests/2_sound_formats` |
| | GND (-) | GND | |
| | VCC (+) | 5V | |
| traffic light module (R, Y, G, GND) | R | **`RED_PIN`** (resistors on the module, connect directly) | `assignments/1_rgb_remote` |
| | G | **`GREEN_PIN`** | |
| | Y | nothing | |
| | GND | GND | |
| blue LED (2 legs) | long leg (+) | 220 Ω → **`BLUE_PIN`** | `assignments/1_rgb_remote` |
| | short leg (-) | GND | |
| OLED (128×64 I2C) | SCL | **`SCL_PIN`** | `assignments/2_moving_square`, `assignments/3_staff` |
| | SDA | **`SDA_PIN`** | |
| | VCC, GND | 5V, GND | |
| passive buzzer | + | 100 Ω → **`BUZZER_PIN`** | `tests/2_sound_formats`, `assignments/3_staff`, `assignments/4_piano` |
| | - | GND | |
| infrared LED (940 nm) | long leg (+) | 220 Ω → **`IR_LED_PIN`** | `tests/3_transmitter` |
| | short leg (-) | GND | |
| USB | D0, D1 | serial | `tests/1_receiver`, `tests/2_sound_formats` (`stream.py`), `tests/3_transmitter`, `assignments/1_rgb_remote`, `assignments/4_piano` (print their state) |

The pins are the `*_PIN` of each program's settings. To keep every part connected at once, they follow these rules (the infrared LED may share a pin with R of the traffic light, as long as both are not connected at once):

| Pin | The rule |
|---|---|
| `IR_PIN` | any pin can use a pin change interrupt; but only one interrupt pin per group (D0 ~ D7, D8 ~ D13, A0 ~ A5), so not in a group another part uses for one |
| `RED_PIN`, `GREEN_PIN`, `BLUE_PIN` | PWM pins not using Timer1 (D9, D10 are kept for the buzzer), and not `SDA_PIN` |
| `IR_LED_PIN` | D3 or D11: Timer2 makes the 38 kHz there |
| `BUZZER_PIN` | D9 or D10: Timer1 toggles the passive buzzer directly, with no CPU taken, so the sound does not break during `display()` (and `tests/2_sound_formats` needs D9 / D10 for PCM) |
| `SCL_PIN`, `SDA_PIN` | any two pins, the same OLED wiring as week_4_SSD1306_switch_buzzer_audio |

The board with 8 pins (SDA, SCK, MOSI, MISO, IRQ, GND, RST, 3.3V) is the **RC522 RFID reader module**, not an infrared receiver, and not used this week (see [lib/NEC](../../lib/NEC/README.md#names)). An infrared receiver has only 3 pins, with a black dome in front: the chip itself, with the dome facing you and the legs down, is OUT, GND, VCC; soldered on a small board (KY-022 and others), wire by what is printed on the board: the order is often different.

# tests/1_receiver.cpp: infrared receiving

A line for every code received:

```
infrared receiver on D2: press keys on the remote
address 0x00  command 0x45  press     numbers top: 1      arrows top: *
address 0x00  command 0x45  repeat 1  numbers top: 1      arrows top: *
```

- **Run this first**: 17-key remotes come in two layouts with different commands (see [lib/NEC](../../lib/NEC/README.md#remotes)). Press a few keys; whichever column, `numbers top` or `arrows top`, matches what you pressed, set the `using Remote = …` in the settings of the three assignments to that one (now `NEC::Remote17KeyNumbersTop`; the other is `NEC::Remote17KeyArrowsTop`).
- **Neither column matches**: write down the `command`s printed and add a remote following `lib/NEC/module/remote_17_key.h`.
- **Held**: first a `press` line, then a `repeat` line every 108 ms.
- **Diagnosing when nothing comes**:
  - power-on prints `{IR_PIN} idle: HIGH (ok)` or `LOW`. Without light it should be HIGH; LOW means the receiver has no power, a pin is wrong, or OUT is not on `IR_PIN`.
  - each burst of pulses is printed as `pulses 67: L9010 H4470 L580 …` (L = light, H = no light, in µs). A good NEC key starts with L about 9000, H about 4500.
  - when there are changes but nothing decodes, every `CHECK_MS` it prints `{IR_PIN} changed N times, no NEC code`. One NEC key changes about 68 times; a single digit is noise.
  - `IR_PIN` has the internal pull-up on (`IR_INTERNAL_PULLUP`): with the receiver badly connected, it is a quiet HIGH instead of floating noise.
- The serial monitor: `pio device monitor` (at `BAUD`, = `monitor_speed` of platformio.ini).

# tests/2_sound_formats.cpp: each sound format

A passive buzzer (+ through 100 Ω to `BUZZER_PIN`). Start `stream.py` on the computer first (`python lib/stream/source/stream.py`); the board's reports are printed as `[BOARD] …`.

| Test | Format | What you should hear |
|---|---|---|
| 0 | notes from Flash, Hz + ms | Twinkle Twinkle Little Star, in tune |
| 1 | notes from Flash, compact (half the size) | the same Twinkle Twinkle |
| 2 | a rhythm from Flash (for active ones), played on the passive one | SOS, at one fixed pitch |
| 3 | samples from Flash, 8 kHz ADPCM | the voice "Hello! I am an Arduino."; prints the real sample rate and how many samples played in the first second |
| 4 | notes from the computer | Twinkle Twinkle (needs `stream.py`) |
| 5 | samples from the computer, 16 kHz 8 bits | the real sound of Ode to Joy; prints how long it took and how many times the buffer ran empty (underruns) |

When done, it pauses `AGAIN_MS` and starts over. Without `stream.py`, 4 and 5 print error 4 (which nobody sees either) and go on.

# tests/3_transmitter.cpp: infrared sending

| Part | Pin | UNO |
|---|---|---|
| infrared LED (or KY-005) | long leg (S) | 220 Ω → **`IR_LED_PIN`** |
| | short leg (-) | GND |
| infrared receiver (optional) | OUT, GND, VCC | **`IR_PIN`**, GND, 5V |

Open `pio device monitor` and press keys (no Enter needed): `0` ~ `9`, `*`, `#`, `w` `a` `s` `d` (arrows), `k` (OK), `h` (OK held for `HOLD_MS`).

```
sent 5 (command 0x40)
  heard 5 (address 0, command 0x40)
sent OK (command 0x1C) held 1 s
  heard OK (address 0, command 0x1C)
  heard OK (address 0, command 0x1C, repeat 1)
```

- **Receiving itself**: with the LED aimed at the receiver, or reflected off a wall, `heard` is what the receiver got, which confirms the sending is right.
- **`IR_LED_PIN` is D3 or D11**: Timer2 puts the 38 kHz straight out of those two pins only.
- **Controlling other things**: it sends the codes of the 17-key remote (address 0), which another UNO running an assignment receives too; for a TV, change to that TV's address and commands (receive its remote's codes with `tests/1_receiver` first).

# assignments/1_rgb_remote.cpp: red, green, blue by remote

| Key | Red R | Green G | Blue B |
|---|---|---|---|
| -= 1 | 1 | 4 | 7 |
| = 0 | 2 | 5 | 8 |
| += 1 | 3 | 6 | 9 |

`0`: R, G, B all back to 0.

- **How big a step is**: R, G, B are whole numbers 0 ~ `LEVELS`, so `LEVELS` presses of `+= 1` go from off to brightest. With 0 ~ 255, one `+= 1` would not even show.
- **Looks even**: each step is first turned into the eye's brightness (`LED::levelToBrightness`), then into the PWM duty cycle (`LED::toDuty`, squared). With the duty cycle directly, the first steps change a lot and the later ones barely at all.
- **Held**: 1, 3, 4, 6, 7, 9 held past `REPEAT_DELAY` repeat codes go about 9 steps a second (one per repeat code, 108 ms); 2, 5, 8, 0 do not repeat when held.
- Each change is also printed to the serial: `R 3  G 0  B 10`.

# assignments/2_moving_square.cpp: a square moved by the arrow keys

- A filled `SIZE` × `SIZE` square, in the middle at first.
- Up, down, left, right move it by 1 pixel, stopping at the edges; `OK` brings it back to the middle.
- **Held**: after `REPEAT_DELAY` repeat codes it moves about 9 pixels a second (NEC repeat codes come every 108 ms).
- **Drawing**: erases the old one, draws the new one; `display()` sends only the changed bytes (about 1 ms), not the whole screen again.

# assignments/3_staff.cpp: a staff

A treble clef, a staff and 7 half notes; a triangle points at the one being written.

| Key | Action |
|---|---|
| up / down | one step higher / lower (C D E F G A B), with the accidental removed |
| `*` | a semitone lower, with a ♭ drawn in front of the note; a ♯ turns back to natural |
| `#` | a semitone higher, with a ♯ drawn in front of the note; a ♭ turns back to natural |
| `OK` | on to the next; OK on the last one plays them all, and afterwards the triangle goes back to the first, ready to change again |

- **Range**: "middle C5 to B4" I take as **middle C (C4, 262 Hz) to B4 (494 Hz)**, that is, the octave starting at C4. In scientific pitch notation middle C is C4; C5 is higher than B4 and cannot be the start of "to B4". Middle C has a short ledger line below the staff; B4 is on the third line.
- **At first**: all 7 are middle C.
- **The extremes of ♭, ♯**: C♭4 plays B3 (247 Hz), B♯4 plays C5 (523 Hz).
- **Playing**: at `BPM`, a half note = 2 beats; the last `GAP_MS` of each note are silent, so two equal notes are heard as two. While playing, the triangle follows along and keys are ignored.
- **Why 7**: each note needs room for an accidental (5 px) and a stem; 16 px a note fits exactly 7; with 8 the accidentals would touch the stem of the note before. Changing `NOTE_COUNT` to something that does not fit is a compile error.
- **The treble clef**: a 15×40 black-and-white picture (`picture::treble_clef_flash_15x40_palette_2`), drawn from 𝄞 of the macOS font Apple Symbols, its curl lined up with the G4 line (see [lib/image/output](../../lib/image/output/README.md#the-current-pictures)). It is drawn before the five lines, so the lines go through it.
- **The picture checked on the computer**: the code drawing the staff was compiled on the computer and drawn into a fake 128×64 screen, to check where the lines, accidentals, notes and ledger lines go.

# assignments/4_piano.cpp: a remote piano

Wiring: infrared receiver (S → `IR_PIN`) + passive buzzer (+ through 100 Ω → **`BUZZER_PIN`**, - → GND).

| Key | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|---|
| Note | A3 | B3 | C4 | D4 | E4 | F4 | G4 | A4 | B4 | C5 |
| Hz | 220 | 247 | 262 | 294 | 330 | 349 | 392 | 440 | 494 | 523 |

- **Sounds as long as held**, stops on release; another key changes the note.
- **Recording**: starts at the first press, recording how long each note sounded and the time between two notes with nothing pressed (ms). The time after the last note until OK is not recorded.
- **OK**: plays back what was just played (keys ignored while playing back), then waits again; the next press starts a new recording, and so on.
- **At most `MAX_STEPS` steps** (notes and rests each count as a step); when full, what is played is not recorded and the serial prints `full`.
- **Serial**: each note and its length are printed (`pio device monitor`), e.g. `C4 412 ms`.

**A remote has no "released" signal**: while held it sends a repeat code every 108 ms, and released counts only when the repeat codes stop (150 ms after the last one). So:

| Situation | Result |
|---|---|
| a quick press | sounds about 150 ~ 250 ms, never shorter |
| held | about 0.1 second longer than the finger pressed |
| just pressed | sounds about 70 ms late (a remote takes 67.5 ms to send a code) |
| playing back | what is recorded is "the length heard", so the playback sounds the same as when you played |

- **Volume**: the setting `VOLUME`.
- Already uploaded to the board, it prints `remote piano: …` at power-on; the playing is for you to try with the remote.

## Resources

| Program | Flash | RAM |
|---|---|---|
| `tests/1_receiver` | 2.6 KB | 390 B (the strings of the key names) |
| `assignments/1_rgb_remote` | 2.7 KB | 150 B |
| `assignments/2_moving_square` | 3.6 KB | 1164 B (the OLED's screen 1 KB) |
| `assignments/3_staff` | 8.3 KB | 1277 B |
| `assignments/4_piano` | 4.0 KB | 856 B |

## Measured

- `tests/1_receiver`: the first receiver had been wired wrong once and afterwards kept OUT pulled LOW (the LED always on, D2 read LOW); after changing to another one, it received.
- The remote, the LED, the square and the staff **have not been tried on the board with a remote yet** (I cannot press a remote). The decoding was tested on the computer: 256 commands, ±15% timing error, holding, noise and broken halves all decoded right (see [lib/NEC](../../lib/NEC/INTERFACE.md#nec-協定)).
