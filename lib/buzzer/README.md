# buzzer

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| active: a `Pin`: `init()`, `write(high)`… (`Uno::UnoOutputPin<PIN>`) | `buzzer::ActiveBuzzer2Pins`, `ActiveBuzzer3Pins`: on, off (fixed pitch) |
| passive: a `Tone`: `init()`, `start(hz)`, `stop()`… (`Uno::UnoTone<PIN>`) | `buzzer::PassiveBuzzer2Pins`, `PassiveBuzzer3Pins`: any frequency, volume |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Two-pin (+, -) buzzers, `namespace buzzer`. Active and passive (the difference: see [Active and passive](#active-and-passive)); both only "make the sound": every module is the Sound that `audio::Player` of [audio](../audio/README.md) needs, and the player plays beeps and melodies on it.

## Usage

```cpp
#include "Uno_pin.h"
#include "Uno_tone.h"
#include "buzzer.h"
#include "audio.h"

constexpr uint8_t BEEPER_PIN = 8;                                         // the active one's +
constexpr uint8_t SPEAKER_PIN = 9;                                        // the passive one's +
using Beeper = buzzer::ActiveBuzzer2Pins<Uno::UnoOutputPin<BEEPER_PIN>>;  // active, two pins
using Speaker = buzzer::PassiveBuzzer2Pins<Uno::UnoTone<SPEAKER_PIN>>;    // passive, two pins
// 3-pin modules: buzzer::ActiveBuzzer3Pins<Uno::UnoOutputPin<BEEPER_PIN>>, buzzer::PassiveBuzzer3Pins<Uno::UnoTone<SPEAKER_PIN, true>>
audio::Player<Beeper> beeper;
audio::Player<Speaker> speaker;

void setup() {
    beeper.init();
    speaker.init();
    beeper.beeps(Beeper::HZ, 3, 100, 100);                   // three beeps (active ignores hz, as long as > 0)
    speaker.play<melody::ode_to_joy_flash_passive_medium>();             // a melody of lib/audio/output
}

void loop() {
    beeper.update();                                         // call it every loop, it never waits
    speaker.update();
}
```

Complete examples: `src/week_4_SSD1306_switch_buzzer_audio/tests/3_active_buzzer.cpp` (active), `src/week_4_SSD1306_switch_buzzer_audio/tests/4_passive_buzzer.cpp` (passive, measures the real frequency on the pin).

## Wiring

| Buzzer | UNO |
|---|---|
| + (the long leg, or the side marked + / S) | a digital pin: active e.g. D8; passive best on D9 or D10 |
| - | GND |

3-pin modules: **I/O → a digital pin, VCC → 5V, GND → GND**, no resistor.

**Connected straight to a pin**, the buzzer gets its power from the pin only:

| Buzzer | Current (5 V) | Wiring |
|---|---|---|
| active (5 V type) | about 20 ~ 30 mA | can connect directly (short beeps) |
| passive, magnetic (coil about 16 Ω) | directly it exceeds a pin's limit | **add a 100 Ω resistor in series** (about 30 mA, a bit quieter) |
| passive, piezo | a few mA | can connect directly |

≤ 20 mA a pin recommended, 40 mA absolute maximum, ≤ 200 mA for the whole chip.

**For louder sound, or 9 V / 12 V buzzers**: use an NPN transistor (e.g. S8050, 2N2222), never connect them directly:

```
external supply + ── buzzer + ── buzzer - ── transistor C
                │                │
                └── diode ◀──────┘     (1N4148, magnetic ones only: reversed across the buzzer)
UNO pin ── 1 kΩ ── transistor B
external supply - ── transistor E ── UNO GND (the two GNDs must be joined)
```

- Transistor on = sound, so `ACTIVE_HIGH` of `ActiveBuzzer` stays `true`.
- The diode takes the reverse voltage when the coil is switched off, protecting the transistor.

## Two pins and three pins

| Part | Looks | What drives the buzzer | Use |
|---|---|---|---|
| a 2-pin buzzer | black cylinder, two legs + and - | the pin directly | `ActiveBuzzer2Pins`, `PassiveBuzzer2Pins` |
| 3 pins with the middle unconnected (KY-012 active, KY-006 passive) | a small board marked S, - (nothing on the middle one) | the pin directly: S is + | the same (as two pins) |
| a 3-pin module: **VCC, I/O, GND** | a small board with a transistor on the back (S8550 / 9012) | the transistor, with current from VCC | `ActiveBuzzer3Pins`, `PassiveBuzzer3Pins` |

- **3-pin modules are louder**: the buzzer's current comes from VCC and the pin only switches the transistor, so no worry about the pin's current limit and no resistor.
- **3-pin modules sound on LOW** (a PNP transistor, often called "low level trigger"):
  - active: `ActiveBuzzer3Pins` defaults to `LOW_TRIGGER = true`. The few modules with an NPN sound on HIGH: change it to `false`.
  - passive: when quiet the pin must stay **HIGH**, or the transistor stays on and the coil keeps conducting. So use `Uno::UnoTone<PIN, true>` (HIGH when quiet); the wrong one is a compile error.
- **When unsure**: a board with the words VCC and I/O and a small black part with three legs on the back (the transistor) is a 3-pin module.

## Volume

`player.setVolume(volume)`: 0 = silent ~ 255 = loudest (255 after `init()`), also while playing.

| Buzzer | How | Adjustable |
|---|---|---|
| passive | the square wave's frequency stays, the share of "on" in each period changes (the duty cycle) | ✅ 0 ~ 255 |
| active | its own oscillator decides the volume | ❌ only 0 = silent, anything else = on (PWM makes it hiss) |
| samples (`audio::SamplePlayer`) | each sample times the volume | ✅ 0 ~ 255 |

**Volume and duty cycle**: the fundamental of a square wave is proportional to sin(π × duty cycle), largest at 50%. `UnoTone` uses duty cycle = asin(volume / 255) / π, so the volume is the size of the fundamental:

| Volume | Duty cycle | Level |
|---|---|---|
| 255 | 50% | 0 dB (loudest) |
| 128 | 16.7% | -6 dB |
| 64 | 8.1% | -12 dB |
| 32 | 4.0% | -18 dB |
| 16 | 2.0% | -24 dB |
| 1 | 0.13% | -48 dB |

- **Measured** (test 7 of `src/week_4_SSD1306_switch_buzzer_audio/tests/4_passive_buzzer.cpp`, 2 kHz): the duty cycle measured on D9 is the one set (within 0.1%), the frequency unchanged.
- **What the ear hears**: -6 dB is about "a bit quieter", -10 dB about "half as loud". Buzzers have some overtones too, so the tone gets sharper when very quiet.
- **The smallest step**: the duty cycle is a whole number of ticks. At 20 kHz a period is only 800 ticks, so the quietest steps get coarse; on pins other than D9 / D10 the shortest is about 2 µs (the interrupt itself).

### Limits

| Item | Limit | Why / what to do |
|---|---|---|
| loudest | volume 255 (duty cycle 50%) | a square wave's fundamental is largest at 50%: no program can be louder |
| louder | hardware: voltage, current, resonance | use a 3-pin module (the transistor drives it, the current comes from VCC); stay near the resonant frequency (commonly 2 ~ 4 kHz, loudest); a 12 V buzzer + a transistor |
| pin current | ≤ 20 mA recommended, 40 mA maximum | magnetic ones get 100 Ω in series (see [Wiring](#wiring)), so the volume straight from a pin is limited |
| loudest samples | samples use the full 0 ~ 255 | `convert.py` already brings the largest sample to full scale; for louder use `--gain` (clips the peaks: sounds louder but distorts) |
| active | not adjustable | only silent; for quieter add a resistor in series (e.g. 100 ~ 330 Ω: quieter; too large and it stops sounding) |

## Active and passive

| | Active | Passive |
|---|---|---|
| inside | an oscillator | only a coil (magnetic) or a piezo disc |
| on DC | sounds, at a fixed pitch (mostly about 2.3 kHz) | silent, only a "click" |
| what the board does | on, off | sends a square wave whose frequency is the pitch |
| can play | beeps, rhythms | scales, melodies, slides |
| telling them apart | on 5V it sounds continuously; the bottom is usually sealed, it is taller; the + leg is longer | on 5V it is silent; the circuit board shows at the bottom |
| use | `ActiveBuzzer2Pins` / `ActiveBuzzer3Pins` + `Uno::UnoOutputPin` | `PassiveBuzzer2Pins` / `PassiveBuzzer3Pins` + `Uno::UnoTone` |

The two differ only in "what `start(hz)` does" (active: pin HIGH; passive: start a timer); the timing of playing is all in `audio::Player`, so they share one folder.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| pin | 0 ~ 19; D0 / D1 compile warning | compile error |
| hz of passive | 16 ~ 20000 (`UnoTone`); pleasant at about 200 ~ 5000, loudest at the resonant frequency (commonly 2 ~ 4 kHz) | clamped |
| hz of active | any > 0 is the same | — |
| volume | passive 0 ~ 255, active only silent (see [Volume](#volume)) | above 255 impossible (it is 50% already) |
| notes at once | one note a Sound | — |

## Caveats

### What the wiring must not do

| Situation | Result | What to do |
|---|---|---|
| the buzzer's + on a pin **and** an external supply (5V, 9V, a battery) at once | the external supply flows into the pin, burning the pin or the whole chip; with the pin LOW it shorts the supply to GND | one buzzer, one source; for an external supply use a transistor (see Wiring) |
| a buzzer between a pin and 9 V / 12 V | with the pin HIGH it still gets more than 5 V and burns out | use a transistor, with the GNDs joined |
| the external supply's GND not joined with the UNO's GND | the transistor does nothing, or it sounds randomly | the GNDs must always be joined |
| a magnetic passive one directly, without a resistor | more current than a pin allows; the pin slowly dies | 100 Ω in series |
| a magnetic one driven by a transistor without a diode | the reverse voltage at switch-off kills the transistor | a reversed 1N4148 across the buzzer |
| an active one's + and - swapped | silent (it has polarity) | the long leg is + |
| a 3 V buzzer straight on a 5 V pin | louder, but it gets hot and lives shorter | add a resistor, or use a 5 V type |
| on D0 / D1 | the serial and uploads interfere, the buzzer sounds randomly | use another pin |
| on D13 / D11 (the pins of this project's OLED, M130T) | fights the screens for the pins | use another pin |

### What the program must not do

| Situation | Result | What to do |
|---|---|---|
| `analogWrite` / `pwmSet` (PWM) on the buzzer's pin | active: its oscillator is cut every period, the sound hisses and the volume does not change; passive: only a fixed 490 / 980 Hz, no scales | no PWM: `ActiveBuzzer` for active, `PassiveBuzzer` for passive |
| `analogWrite` / `pwmSet` on D9 / D10 (e.g. the BLK of `UnoST7789` on D9) together with a passive one | both change Timer1: frequency and brightness both wrong | move BLK to D3, D5, D6, D11, or no brightness control |
| Arduino's `tone()` / `noTone()`, `Servo` at the same time | fight over the timer and the pin | use only this library |
| `digitalWrite` on the buzzer's pin while playing | `audio::Player` does not know, its state no longer matches | control it only through `Player` |
| two passive ones sounding at once | there is one Timer1: the one `start`ed later changes the frequency and pin of the other | one passive at a time; or one passive + several active |
| `PassiveBuzzer…` driving an active buzzer | the square wave keeps cutting its oscillator, a messy sound | active with `ActiveBuzzer…` |
| `ActiveBuzzer…` driving a passive buzzer | only a "click" when switched on; a magnetic one heats up on long DC | passive with `PassiveBuzzer…` |
| a 3-pin module used as two pins (`…2Pins`) | active: on and off swapped, it sounds from power-on; passive: the pin is LOW when quiet, the transistor stays on and the coil keeps conducting | use `…3Pins`; passive with `Uno::UnoTone<PIN, true>` |
| `Uno_tone.h` included by two .cpp | the interrupt functions are defined twice, a link error | include it in one .cpp only |
| a passive one on a pin other than D9 / D10 together with `UnoOneWire` (DS18B20) or other code turning interrupts off | the square wave pauses while interrupts are off, the sound wobbles | put the passive one on D9 or D10 |
| `update()` called only once in a long while (e.g. the OLED's `display()`, 64 ms) | the notes' edges come late, but the rhythm does not drift; the passive square wave goes on (the timer runs) | normal; for precision, do fewer long things |
| the board going to sleep (power-down) | Timer1 stops, the passive one is silent | do not sleep while playing |
| wanting to adjust the volume | passive with `setVolume`; active only silent | for a quieter active one, add a resistor |
