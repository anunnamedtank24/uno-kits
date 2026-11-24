# week_7_L298N

English | [中文](讀我.md)

Tests of the L298N motor driver ([lib/L298N](../../lib/L298N/README.md)): one and two DC motors, voltage control, PWM, a stepper, a two-wheeled car.

Every file is `.bak` now; keep the one to run:

```bash
python src/hide.py week_7_L298N/tests/1_one_motor     # keep only this one (drop its .bak), the others become .bak
pio run -t upload
pio device monitor                      # at BAUD (= monitor_speed of platformio.ini)
```

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_one_motor.cpp` | **one** motor: ramps, coasting compared with braking, reversing, a speed ladder | L298N + DC motor (OUT1, OUT2) |
| `tests/2_two_motors.cpp` | **two** motors (a two-wheeled car): each checked alone, straight, backward, turning, spinning in place | L298N + 2 DC motors |
| `tests/3_voltage.cpp` | **voltage control**: type `a 4.5` on the serial and motor A gets about 4.5 V | L298N + 1 ~ 2 DC motors |
| `tests/4_pwm.cpp` | **PWM**: duty cycle from small to large (where it starts turning), from large to small (where it stops), the start-up "kick", PWM braking | L298N + DC motor |
| `tests/5_stepper.cpp` | **stepper**: FULL, HALF, WAVE each a turn there and back, a speed ladder for the limit | L298N + bipolar stepper (4 wires) |
| `tests/6_car.cpp` | **two-wheeled car** (`TwoWheelCar`): straight, backward, spinning in place, pivoting, arcs, a square, a figure 8, a zigzag | L298N (**EN jumpers on**, IN1, IN3 on PWM pins, IN2, IN4 on GND) + 2 DC motors |
| `tests/7_background_remote.cpp` | **background remote**: the car runs in the background (`UnoBackground`), driven by the keyboard; moves without a time, `until`, stopping when the link is lost | L298N (**EN jumpers on**, IN1 ~ IN4 all on PWM pins) + 2 DC motors |
| `tests/8_car_enable.cpp` | **a two-wheeled car with speed on EN**: ENA, ENB on PWM pins: real coasting, slow speeds, every move | L298N (**EN jumpers removed**, wired as `tests/1_one_motor`) + 2 DC motors |

## Wiring

`tests/1_one_motor` ~ `tests/5_stepper` and `tests/8_car_enable` share the wiring (tests with one motor leave channel B empty); `tests/6_car`, `tests/7_background_remote` differ, see [tests/6_car](#tests6_carcpp-two-wheeled-car).

| Part | Pin | UNO / elsewhere |
|---|---|---|
| L298N | ENA (**jumper removed**) | **`ENA_PIN`** (a PWM pin) |
| | IN1 | **`IN1_PIN`** |
| | IN2 | **`IN2_PIN`** |
| | IN3 | **`IN3_PIN`** |
| | IN4 | **`IN4_PIN`** |
| | ENB (**jumper removed**) | **`ENB_PIN`** (a PWM pin) |
| | +12V | the battery / adapter's + (6 ~ 12 V, depending on the motors) |
| | GND | the battery's - **and** the UNO's GND |
| | +5V | nothing (5V-EN jumper on) |
| | OUT1, OUT2 | motor A (stepper: coil A) |
| | OUT3, OUT4 | motor B (stepper: coil B) |
| USB | D0, D1 | serial |

| Pin | The rule |
|---|---|
| `ENA_PIN`, `ENB_PIN` | PWM pins that clash with neither the buzzer (D9 / D10, Timer1) nor the OLED's `SDA_PIN` (D11 in the other weeks) |
| `IN1_PIN` ~ `IN4_PIN` | any pins; the analog pins used as digital pins keep the digital ones free for the parts of other weeks |

- **Never power the motors from the UNO's 5V**: motors draw a big current when starting, which resets the UNO or burns its USB.
- **The first test**: lift the car, wheels in the air.

# tests/1_one_motor.cpp: one motor

```
1 forward 255: ramps up in 1 s
2 coast: rolls out slowly
3 forward 255 again
4 brake: stops at once (compare with 2)
5 backward 160
5 forward 160: through 0 by the ramp, no jolt
6 speed ladder: 40 (turning? if not: the dead zone, see tests/4_pwm)
...
```

- **Compare 2 and 4**: coasting stops slowly, braking stops at once.
- **5**: set straight from backward to forward, the ramp first goes down to 0 and then up, never reversing abruptly.
- **6**: at speeds 40, 80 it may only hum: that is the dead zone, made up for by `MIN_DUTY` (see `tests/4_pwm`).
- Turning the wrong way: swap the motor's two wires, or `REVERSED = true`.

# tests/2_two_motors.cpp: two motors

A is the left wheel, B the right. Watch 1 and 2 first: only one wheel turns, forward, to check which is which.

| Step | Left | Right | The car |
|---|---|---|---|
| 3 | 200 | 200 | straight |
| 4 | -200 | -200 | backward |
| 5 | 200 | 100 | arc to the right |
| 6 | 100 | 200 | arc to the left |
| 7 | 200 | brake | turns around the right wheel |
| 8 | 200 | -200 | spins right in place |
| 9 | -200 | 200 | spins left in place |
| 10 | 255 | 255 | are the two motors equally fast (usually a little different: trim with the speeds) |

- The two motors of a car face each other, so usually one must be reversed: `LEFT_REVERSED` or `RIGHT_REVERSED = true`.

# tests/3_voltage.cpp: voltage control

Type a line in `pio device monitor` and press Enter:

| Input | What it does |
|---|---|
| `a 4.5` | motor A forward, about 4.5 V |
| `a -3` | motor A backward, about 3 V |
| `b 6` | motor B, about 6 V |
| `x 5` | both 5 V |
| `a 0` | motor A coasts |
| `s` | both brake |
| `?` | prints the settings |

```
> a 4.5
A: duty 115 / 255 (45 %) -> about 4.51 V
```

- **The L298N cannot really lower the voltage**: it switches the supply on and off (PWM), and on average the motor gets `duty cycle × (supply - drop)`.
- Set the settings to yours: `SUPPLY_V` (the supply, measured with a multimeter; a 9 V battery often has only about 7 V left with motors on it), `DROP_V` (what the L298N takes, about 2 V), `MOTOR_MAX_V` (the motor's rated voltage; anything above is limited).
- The DC voltage measured with a multimeter on OUT1, OUT2 is about the value printed (PWM makes some multimeters less accurate).

# tests/4_pwm.cpp: PWM

Only motor A, four parts repeated forever:

1. **Duty cycle 0 → 255** (`SWEEP_STEP` more every `SWEEP_MS`): note the duty cycle where it **starts turning**.
2. **255 → 0**: note the duty cycle where it **stops**: lower than in 1 (a motor already turning keeps turning more easily).
3. **A kick**: started straight at a low duty cycle (`LOW_DUTY`, set a bit below the value of 1) it may only hum; 255 for `KICK_MS` first, then down, and it starts and keeps turning.
4. **PWM braking**: from full speed, `brake()` with each of `BRAKE_STRENGTHS` in turn (0 = coasting): the stronger, the shorter the stopping distance.

- The value of 1 is what `DCMotor::setMinDuty` should be set to (`MIN_DUTY` of `tests/1_one_motor`, `tests/2_two_motors`).
- **Humming**: the UNO's PWM is 976 Hz, within hearing: normal.

# tests/5_stepper.cpp: stepper

A bipolar stepper (4 wires): find the two coils with a multimeter (two wires with a few Ω are a pair); coil A on OUT1, OUT2, coil B on OUT3, OUT4.

```
FULL (2 coils, most torque): a turn forward, then back (200 steps at 100 steps/s)
  at 200
  at 0
HALF (smoother, twice the steps): ...
speed ladder (FULL): the last speed that turns is the limit
  50 steps/s = 15 rpm
  ...
```

- **Shaking, not turning**: one coil is reversed: swap OUT3 and OUT4.
- **Speed ladder**: the last speed still turning is the limit at this supply; beyond it, it loses steps (hums without turning).
- **Very hot**: lower `RUN_DUTY`. The L298N does not limit current: a low-resistance stepper (e.g. a 1.5 Ω NEMA 17) on 12 V draws too much, see [lib/L298N](../../lib/L298N/README.md#steppers-take-care).
- `STEPS_PER_TURN`: 200 for a 1.8° motor; about 2048 for the 28BYJ-48.

# tests/6_car.cpp: two-wheeled car

**Wired differently from the other tests**: only IN, OUT1 ~ OUT4, +12V, GND, with **the ENA, ENB jumpers on**. The speed comes from PWM on the IN pins instead, so every IN on the UNO must be a PWM pin.

| L298N | Two INs on GND (forward only) | All four connected (backward and spinning possible) |
|---|---|---|
| IN1 | a PWM pin (`IN1_PIN`) | a PWM pin (`IN1_PIN`) |
| IN2 | GND (`IN2_PIN = GROUND`) | a PWM pin (`IN2_PIN`) |
| IN3 | a PWM pin (`IN3_PIN`) | a PWM pin (`IN3_PIN`) |
| IN4 | GND (`IN4_PIN = GROUND`) | a PWM pin (`IN4_PIN`) |
| ENA, ENB | nothing (jumpers on) | the same |
| OUT1, OUT2 | right motor | the same |
| OUT3, OUT4 | left motor | the same |
| +12V | the battery's + | the same |
| GND | the battery's - **and** the UNO's GND | the same |

- **Which PWM pins**: the UNO has only 6 (3, 5, 6, 9, 10, 11); D9, D10 (Timer1) are kept for the buzzer, so four INs need D11 too, the OLED's SDA in the other weeks: then the OLED cannot be connected too.

The wiring is written as a type, all in one line, e.g.:

```cpp
//                                    IN1 / IN2     IN1 - IN2 > 0    IN3 - IN4 > 0
using Car = L298N::TwoWheelCar<Board, L298N::RIGHT, L298N::FORWARD, L298N::FORWARD>;
```

- **IN1 - IN2 positive** (IN1 HIGH, IN2 LOW): here the right motor goes forward.
- **IN3 - IN4 positive**: here the left motor goes forward.
- A wheel turning the wrong way: change that pair's `FORWARD` to `BACKWARD`. Left and right swapped: `RIGHT` to `LEFT`.
- **INs on GND**: IN2, IN4 straight on GND, using only two PWM pins (`IN2_PIN = GROUND`, `IN4_PIN = GROUND`). The wheels only go forward: backward and spinning in place (moves 2, 3, 4, 9) become braking. For every move, wire four pins as in the right column above and change the four `IN*_PIN`.

| Move | Contents |
|---|---|
| 1, 2 | straight, backward 1.5 seconds: **lift it first to check both wheels go forward, both backward** |
| 3, 4 | spin right, left 90° in place |
| 5, 6 | pivoting: the right wheel still / the left wheel still |
| 7, 8 | arcs: left fast right slow (to the right), right fast left slow (to the left), half a circle each |
| 9 | a square: 4 × (straight 1 second, right 90°), queued at once |
| 10 | a figure 8: a circle to the right, a circle to the left |
| 11 | a zigzag: the two wheels' speeds swapped every 0.6 seconds |
| 12 | straight, then coasting (with the jumpers on it becomes braking) |

The parameters in the settings, to be measured on the floor:

| Parameter | Explanation |
|---|---|
| `SPEED` | the speed of every move |
| `SPIN_90_MS` | how many ms a 90° spin in place takes: adjust after seeing how far 3 turns, so the square closes |
| `ARC_360_MS` | how many ms a full circle of the arc takes: so the figure 8 closes |
| `ARC_DIFFERENCE` | the speed difference of the two wheels in an arc: the bigger, the sharper the turn |
| `LEFT_TRIM`, `RIGHT_TRIM` | it drifts going straight: lower the faster side (%) |
| `MIN_DUTY` | the motors' dead zone (`tests/4_pwm`) |

# tests/7_background_remote.cpp: background remote

Wired as the right column of the wiring table of `tests/6_car` (EN jumpers on, IN1 ~ IN4 all on PWM pins): backward and spinning in place are needed. Open `pio device monitor` and press keys (no Enter needed):

| Key | Move |
|---|---|
| `w` / `s` | forward / backward |
| `a` / `d` | spin left / right in place |
| `q` / `e` | arc to the left / right |
| space, `x` | stop |
| `f` | forward, until it has gone `UNTIL_MS` (`until`) |
| `t` | forward `TIMED_FORWARD_MS`, then right `TIMED_SPIN_MS` (with times, queued) |

- **Background**: `Uno::UnoBackground::add([] { car.update(); })`; `loop()` only reads the keyboard, prints the speeds, and has a `delay(PRINT_MS)`, yet the car still ramps and changes moves.
- **Moves without a time**: `w`, `a` and the like go on and on.
- **Stopping when the link is lost** (`setFailsafeMs(FAILSAFE_MS)`): it brakes by itself `FAILSAFE_MS` after the key is released. While a key is held the terminal keeps resending it, so it keeps going, like a remote.

# tests/8_car_enable.cpp: a two-wheeled car with speed on EN

Wired like `tests/1_one_motor` ([Wiring](#wiring)): **remove the ENA, ENB jumpers**, ENA, ENB on PWM pins (`ENA_PIN`, `ENB_PIN`), IN1 ~ IN4 on `IN1_PIN` ~ `IN4_PIN`. The same car as `tests/6_car` (`TwoWheelCar`), only the speed comes from EN, so:

- **It really coasts**: with EN LOW the outputs are open. Compare move 3 (coasting) with 4 (braking): 3 stops slowly, 4 at once; with the jumpers of `tests/6_car` both are braking.
- **Smoother at slow speeds**: PWM on EN alternates "drive / coast", PWM on IN alternates "drive / brake".
- **Takes 6 pins** (`tests/6_car` takes 4).

| Move | Contents |
|---|---|
| 1, 2 | straight, backward 1.5 seconds: **lift it first to check both wheels go forward, both backward** |
| 3 | straight 1 second, then coasting 2 seconds: stops slowly |
| 4 | straight 1 second, then braking 2 seconds: stops at once |
| 5 | straight at a slow speed (`SLOW_SPEED`): not turning, raise `SLOW_SPEED` or `MIN_DUTY` |
| 6, 7 | spin right, left 90° in place |
| 8 | pivoting: the right wheel braked, the left turning |
| 9 | an arc to the right, then to the left |

- At power-on it prints what this wiring can do: `both ways, coasts, brakes`.
- **Forward only, with only 2 pins**: IN1, IN3 on +5V, IN2, IN4 on GND, and `Board` changed to the `L298NBoard<…, L298N::Supply, L298N::Ground, …>` in the comment. Then it cannot brake (every stop coasts), and backward and spinning in place are impossible too (power-on prints `forward only, coasts, cannot brake`).
- `SUPPLY_MV`: the battery's voltage (mV); when given, the 5V-EN jumper is checked when compiling (see [the supply and the 5V-EN jumper of lib/L298N](../../lib/L298N/README.md#supply-and-the-5v-en-jumper)).
