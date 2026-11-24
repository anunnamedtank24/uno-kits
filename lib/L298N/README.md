# L298N

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| each IN and EN: a PWM pin (`Uno::UnoPwmPin<PIN>`), an output pin (`Uno::UnoOutputPin<PIN>`), or not on the UNO: `L298N::Ground` (on GND), `L298N::Supply` (on +5V; the EN jumper is `L298N::EnableJumper`) | `L298N::L298NBoard`: two H-bridges `Board::A`, `Board::B`, what they can do decided by the wiring (at compile time); `L298NBoardJumpers` (EN jumpers on), `L298NBoardParallel` (both channels in parallel as one) |
| the `Board::A`, `Board::B` above | `L298N::DCMotor` (speed, ramp, brake, dead zone), `Stepper` (stepper motors), `TwoWheelCar` (two-wheeled car, moves queued) |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

The L298N dual H-bridge motor driver, `namespace L298N`. It drives two DC motors, or one bipolar stepper.

## Usage

```cpp
#include "Uno_pin.h"
#include "L298N.h"

constexpr uint8_t ENA_PIN = 5;      // a PWM pin
constexpr uint8_t IN1_PIN = A0;
constexpr uint8_t IN2_PIN = A1;
constexpr uint8_t IN3_PIN = A2;
constexpr uint8_t IN4_PIN = A3;
constexpr uint8_t ENB_PIN = 6;      // a PWM pin

using Board = L298N::L298NBoard<Uno::UnoPwmPin<ENA_PIN>,
                                Uno::UnoOutputPin<IN1_PIN>, Uno::UnoOutputPin<IN2_PIN>,
                                Uno::UnoOutputPin<IN3_PIN>, Uno::UnoOutputPin<IN4_PIN>,
                                Uno::UnoPwmPin<ENB_PIN>>;
L298N::DCMotor<Board::A> left;      // OUT1, OUT2
L298N::DCMotor<Board::B> right;     // OUT3, OUT4

void setup() {
    left.init();
    right.init();
    left.setSpeed(200);             // forward, through the ramp (by default 300 ms from 0 to 255)
}

void loop() {
    left.update();                  // call it every loop, it never waits
    right.update();
}
```

A stepper:

```cpp
L298N::Stepper<Board::A, Board::B> motor;   // coil A on OUT1, OUT2, coil B on OUT3, OUT4

motor.init();
motor.setSpeed(200);                // steps / s
motor.move(400);                    // 400 steps forward; call motor.update() in loop
```

Complete examples: `src/week_7_L298N/tests/1_one_motor.cpp` (one), `2_two_motors.cpp` (two), `3_voltage.cpp` (how many volts the motor gets), `4_pwm.cpp` (what PWM is), `5_stepper.cpp` (stepper), `6_car.cpp` (two-wheeled car, jumpers on), `7_background_remote.cpp` (remote + background), `8_car_enable.cpp` (two-wheeled car, speed on ENA, ENB).

## Every wiring

Each channel has three inputs: IN1, IN2 (direction) and EN (on / off). Each can go to a PWM pin of the UNO, a plain pin, or not to the UNO at all but straight to GND / +5V; each position of `L298NBoard` takes the matching type, what can be done is decided at compile time, and what cannot be done is replaced by the closest thing (see the last column below).

| This input | Type | Takes no pin |
|---|---|---|
| a PWM pin (3, 5, 6, 9, 10, 11) | `Uno::UnoPwmPin<PIN>` | |
| a plain pin | `Uno::UnoOutputPin<PIN>` | |
| on GND (IN only) | `L298N::Ground` | ✅ |
| on +5V | `L298N::Supply`; the EN jumper on is the same thing: `L298N::EnableJumper` | ✅ |

Common wirings of one channel (the other channel may be the same or different):

| EN | IN1, IN2 | Pins | Can | Type |
|---|---|---|---|---|
| PWM pin | pin, pin | 3 | both ways, speed, brake, coast | `L298NBoard<UnoPwmPin, UnoOutputPin, UnoOutputPin, …>` |
| PWM pin | +5V, GND | 1 | one way, speed, coast (no brake) | `L298NBoard<UnoPwmPin, Supply, Ground, …>` |
| PWM pin | pin, GND | 2 | one way, speed, brake, coast | `L298NBoard<UnoPwmPin, UnoOutputPin, Ground, …>` |
| pin | pin, pin | 3 | both ways, full speed only, brake, coast | `L298NBoard<UnoOutputPin, UnoOutputPin, UnoOutputPin, …>` |
| jumper | PWM pin, PWM pin | 2 | both ways, speed, brake (no coast) | `L298NBoardJumpers<UnoPwmPin, UnoPwmPin, …>` |
| jumper | PWM pin, GND | 1 | one way, speed, brake | `L298NBoardJumpers<UnoPwmPin, Ground, …>` |
| jumper | pin, pin | 2 | both ways, full speed only, brake | `L298NBoardJumpers<UnoOutputPin, UnoOutputPin, …>` |

| Cannot do | Becomes |
|---|---|
| the other way (an IN on GND or +5V) | brake; coast when braking is not possible either |
| coast (EN on +5V or the jumper) | brake |
| brake (one IN on +5V, the other on GND) | coast |
| speed (no PWM pin) | full speed: any speed that is not 0 is 255 |

- **Which pin sets the speed**: EN when it is a PWM pin ("drive / coast" alternating, the smoothest, and coasting works); else the IN that goes HIGH ("drive / brake" alternating); else the IN that goes LOW, with the duty inverted.
- **Known at compile time**: `Board::A::CAN_FORWARD`, `CAN_BACKWARD`, `BOTH_WAYS`, `CAN_BRAKE`, `CAN_COAST`, `HAS_SPEED`, `PIN_COUNT` (how many pins it takes).
- **Compile errors**: EN on GND (the channel is always off), IN1 and IN2 both on GND or both on +5V (the motor never turns), none of the three on the UNO (nothing to control).
- The time (`nowMs`, `nowUs`) comes by itself from the first pin connected to the UNO.

### IN on GND (`L298N::Ground`)

```cpp
// IN2, IN4 on GND, only two PWM pins (jumpers on)
constexpr uint8_t IN1_PIN = 5;
constexpr uint8_t IN3_PIN = 3;
using Board = L298N::L298NBoardJumpers<Uno::UnoPwmPin<IN1_PIN>, L298N::Ground, Uno::UnoPwmPin<IN3_PIN>, L298N::Ground>;
```

| On GND | The motor can | The other way |
|---|---|---|
| IN2 (or IN4) | only the way where IN1 - IN2 is positive | brakes |
| IN1 (or IN3) | only the way where IN1 - IN2 is negative | brakes |
| both IN1 and IN2 | not turn | compile error |

- **Saves pins**: one pin less a motor. A car that only goes forward (no backing up, no spinning in place) needs only 2 PWM pins for two motors.
- **Two-wheeled car**: `forward`, `pivotLeft` / `pivotRight` and forward `arc`s all work; `backward` and `spin` brake the wheel that would have to go backward.

### IN on +5V (`L298N::Supply`)

```cpp
// IN1, IN3 on +5V, IN2, IN4 on GND, jumpers off: only one PWM pin (EN) a motor
constexpr uint8_t ENA_PIN = 5;
constexpr uint8_t ENB_PIN = 6;
using Board = L298N::L298NBoard<Uno::UnoPwmPin<ENA_PIN>, L298N::Supply, L298N::Ground,
                                L298N::Supply, L298N::Ground, Uno::UnoPwmPin<ENB_PIN>>;
```

- The direction is fixed, EN sets the speed and coasting works; but the two INs are never equal, so it **cannot brake** (every stop rolls out).
- One IN on +5V, the other on a pin of the UNO: that pin LOW is forward, HIGH is a brake (both ends on the supply); no backward.

### Both channels in parallel (`L298NBoardParallel`)

A bigger motor (over 2 A) can be driven by both channels together, up to about 3 A. Wired as the L298N datasheet says, half-bridge 1 with 4 and 2 with 3:

| Joined | To |
|---|---|
| OUT1 + OUT4 | one wire of the motor |
| OUT2 + OUT3 | the other wire of the motor |
| IN1 + IN4 | the `In1` pin |
| IN2 + IN3 | the `In2` pin |
| ENA + ENB | the `Enable` pin (or both jumpers on: `EnableJumper`) |

```cpp
using Board = L298N::L298NBoardParallel<Uno::UnoPwmPin<ENA_PIN>, Uno::UnoOutputPin<IN1_PIN>, Uno::UnoOutputPin<IN2_PIN>>;
L298N::DCMotor<Board::Motor> motor;
```

### Supply and the 5V-EN jumper

The last two parameters of `L298NBoard`, `L298NBoardJumpers`, `L298NBoardParallel` can give the supply, checked at compile time:

```cpp
constexpr uint16_t SUPPLY_MV = 7400;    // the voltage on +12V (e.g. two lithium cells); 0 = not checked
constexpr bool JUMPER_5V = true;        // the 5V-EN jumper on
using Board = L298N::L298NBoard<…, Uno::UnoPwmPin<ENB_PIN>, SUPPLY_MV, JUMPER_5V>;
```

| 5V-EN jumper | +5V terminal | When |
|---|---|---|
| **on** (`JUMPER_5V = true`, the default) | the board's regulator (78M05) **outputs** 5 V, which may feed the UNO's 5V pin (about 0.5 A at most) | VS about 7 ~ 12 V |
| **off** (`JUMPER_5V = false`) | 5 V **input**: connect the UNO's 5V; the chip's logic needs 5 V | VS above 12 V, or below about 7 V |

| Situation | Result |
|---|---|
| `SUPPLY_MV` above 35000 | compile error: the board's capacitors are 35 V parts |
| jumper on, `SUPPLY_MV` above 12000 | compile warning: the regulator gets too hot |
| jumper on, `SUPPLY_MV` below 7000 | compile warning: the regulator cannot make 5 V |
| `SUPPLY_MV` below 5000 | compile warning: after the drop of about 2 V the motors get almost nothing |

`Board::FIVE_V_IS_OUTPUT`: whether +5V is an output (the jumper on).

## Wiring

| L298N | UNO | Explanation |
|---|---|---|
| ENA | **D5** | Timer0 PWM, 976 Hz (jumper removed) |
| IN1 | **A0** | used as a digital pin (`UnoOutputPin<14>`) |
| IN2 | **A1** | `UnoOutputPin<15>` |
| IN3 | **A2** | `UnoOutputPin<16>` |
| IN4 | **A3** | `UnoOutputPin<17>` |
| ENB | **D6** | Timer0 PWM (jumper removed) |
| GND | GND | joined with the battery's - |
| +12V | — | the battery's + (6 ~ 12 V) |
| +5V | nothing, or feeding the UNO's 5V | 5V-EN jumper on |

- **Why D5, D6**: D9, D10 are kept for the buzzer (Timer1), D3, D11 are Timer2 (D11 is taken by the OLED). Timer0 of D5, D6 also runs `millis()`, but it is only read, its frequency is never changed, so that is fine.
- **Why the INs on A0 ~ A3**: analog pins also work as digital pins, so D2 (infrared), D4 (HW-006), D7, D8 (HC-SR04), D9 (buzzer), D11, D13 (OLED) all stay free to be connected together later.

## Names

| Name | What it is |
|---|---|
| **L298N** | the chip: ST's dual H-bridge (Multiwatt15 package, 15 pins), on the board's big heat sink. Named after the chip, so the folder is `L298N/` |
| **H-bridge** | 4 switches around the motor in an H shape; closing them crosswise lets the current flow either way, so the motor turns forward or backward |
| **the red L298N module** | the board you actually buy (often printed HW-095, not always), in `module/L298N_board.h` |
| not this library | the small blue "L298N mini" boards are really other chips such as the MX1508 / L9110S, with different pins and usage |

## The board's pins

```
        ┌──────────────────────────────────────┐
 OUT1 ──┤                                      ├── OUT4
 OUT2 ──┤             L298N chip               ├── OUT3
        │             (heat sink)              │
        │  [5V-EN jumper]                       │
        │  +12V   GND   +5V      ENA IN1 IN2 IN3 IN4 ENB
        └───┬──────┬─────┬────────┬───┬───┬───┬───┬───┬──┘
          screw terminals (power)   header pins (jumpers on ENA, ENB)
```

(The layout differs between makers; the above is only a common one. **Wire by what is printed on the board.**)

| Pin | Kind | Function | Goes to |
|---|---|---|---|
| **OUT1, OUT2** | screw terminal | output of channel A | the two wires of motor A (stepper: coil A) |
| **OUT3, OUT4** | screw terminal | output of channel B | the two wires of motor B (stepper: coil B) |
| **+12V** (VS) | screw terminal | the motors' supply, **5 ~ 35 V** (named 12V, but 12 V is not required) | the battery / adapter's +. **Not the UNO's 5V** |
| **GND** | screw terminal | common ground | the battery's - **and the UNO's GND, both** |
| **+5V** | screw terminal | 5V-EN jumper **on**, VS ≤ 12 V: the board's regulator (78M05) **outputs** 5 V | may feed the UNO's 5V pin (about 0.5 A at most; fine with the UNO on USB at the same time) |
| | | 5V-EN jumper **off** (always off when VS > 12 V): 5 V **input** | brought from the UNO's 5V; the chip's logic needs 5 V |
| **ENA** | header pin | channel A on / off, speed by PWM | jumper on: always on; removed: a PWM pin (e.g. **D5**) or a plain pin (on / off only) |
| **IN1, IN2** | header pin | direction of channel A | pins (e.g. **A0, A1**; PWM pins work too), or GND / +5V |
| **IN3, IN4** | header pin | direction of channel B | pins (e.g. **A2, A3**), or GND / +5V |
| **ENB** | header pin | channel B on / off, speed by PWM | jumper on: always on; removed: a PWM pin (e.g. **D6**) or a plain pin |

- **The grounds must be joined**: the HIGH / LOW of IN and EN are measured against GND; without a common ground the L298N cannot read the UNO's signals.
- **Jumpers**: the jumpers on ENA, ENB connect EN to 5 V. Remove them for speed control or coasting and connect pins of the UNO; keep the removed caps.
- **Current sensing (SENSE A, SENSE B)**: the chip has it, but this board connects them straight to GND, so the current cannot be measured.
- **Protection diodes**: the board has 8, so the kickback voltage when a motor is switched off does not damage the chip.
- **Logic levels**: HIGH on IN and EN needs only 2.3 V or more, so 3.3 V and 5 V boards both work.

## Truth table

| EN | IN1 | IN2 | OUT1 | OUT2 | Motor | Function |
|---|---|---|---|---|---|---|
| HIGH | HIGH | LOW | + | - | **forward** | `drive(> 0)` |
| HIGH | LOW | HIGH | - | + | **backward** | `drive(< 0)` |
| HIGH | LOW | LOW | - | - | **brake**: both ends shorted, the motor's own generated voltage holds it back, it stops fast | `brake()` |
| HIGH | HIGH | HIGH | + | + | brake (the same) | `brake()` (with an IN on +5V) |
| LOW | any | any | open | open | **coast**: nothing connected, it slows down by itself | `coast()`, `drive(0)` |
| PWM | HIGH | LOW | | | forward, speed = duty cycle (drive / coast) | `drive(duty)` (EN is a PWM pin) |
| HIGH | PWM | LOW | | | forward, speed = duty cycle (drive / brake) | `drive(duty)` (IN1 is a PWM pin) |

Channel B is the same: ENB, IN3, IN4, OUT3, OUT4.

## Ratings and limits

| Item | Value | Explanation |
|---|---|---|
| motor supply VS | 5 ~ 35 V (the board's capacitors are often 35 V parts; ≤ 24 V recommended) | below about 7 V, little is left after the drop |
| current a channel | **2 A** continuous (with the heat sink), 3 A peak; both channels in parallel about 3 A | the current is highest when the motor is stuck (stalled): check the motor's stall current |
| total power | 25 W (the chip) | very hot means it is over |
| **voltage drop** | about 1.5 V (0.3 A) ~ 2.5 V (1 A), two transistors together | with a 12 V supply the motor gets at most about 10 V; the rest becomes heat |
| PWM frequency | the chip goes up to about 40 kHz; the UNO's PWM pins about 976 Hz (D5, D6) / 490 Hz | 976 Hz is audible, the motor hums, normal |
| logic supply | 5 V, about 36 mA | the board's regulator or the UNO's 5V |
| missing features | no current limiting, no current measuring, no protection except overheating | take special care with steppers, see below |

## Steppers: take care

- **Telling the coils apart**: measure the resistance with a multimeter: two wires with a few Ω are one coil, no connection means different coils. Coil A goes to OUT1, OUT2, coil B to OUT3, OUT4. Wired the other way it only turns backward or shakes, nothing breaks: swap OUT3 and OUT4.
- **The L298N does not limit current**: the coil current = supply voltage ÷ coil resistance. A common NEMA 17 (1.5 Ω, rated 1.7 A) on 12 V would want 8 A, overheating both the L298N and the motor.
  - Use a motor rated near the supply voltage (e.g. 12 V, about 30 Ω).
  - Or lower the average current with `setRunDuty`: about duty ÷ 255 × voltage ÷ resistance.
  - For low-resistance steppers, use a driver that limits current (A4988, DRV8825).
- **A stepper needs both directions**: both channels must turn both ways (no IN on GND / +5V).
- **28BYJ-48** (the small 5-wire stepper): leaving out the red middle wire, the other 4 work as a bipolar motor on the L298N, about 2048 steps a turn (FULL), at most about 500 steps / s.
- **No acceleration**: `setSpeed` uses that speed from the start. To go fast, start slow and raise the speed step by step (the speed ladder of `src/week_7_L298N/tests/5_stepper.cpp` does this).

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| `drive`, `setSpeed` | -255 ~ 255 | clamped |
| `setMinDuty` | 0 ~ 254 | above 254 taken as 254 |
| `setRampMs` | 0 ~ 65535 ms | — |
| `Stepper::setSpeed` | 1 ~ 2000 steps / s | clamped |
| position of `Stepper` | ±2.1 billion steps | wraps around after that |
| pins of `UnoPwmPin` | PWM pins (3, 5, 6, 9, 10, 11) | compile error |
| wiring | EN never on GND; IN1, IN2 never both on GND or both on +5V; at least one on the UNO | compile error |
| `SUPPLY_MV` | 0 ~ 35000; 7000 ~ 12000 recommended with the jumper on | compile error; compile warning |
| interval of `update()` | the more often the better | steppers: slower when longer than a step's interval (`delay` stops it) |
