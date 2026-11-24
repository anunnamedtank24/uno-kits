# NEC

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| receiving: `InterruptPin`: `init(onChange)`, `read()`, `nowUs()`… (`Uno::UnoInterruptPin<PIN>`) | `NEC::HX1838`: the codes received (which remote, which key, held or not), none missed |
| the remote's command table | `NEC::Remote17KeyNumbersTop`, `Remote17KeyArrowsTop`: codes turned into keys (`KEY_1`, `KEY_UP`…) |
| sending: `Carrier`: 38 kHz on, off (`Uno::UnoIrCarrier<3>`) | `NEC::IRLED`: an infrared LED as a remote |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

The NEC protocol of infrared remotes, `namespace NEC`. The receiver (`module/HX1838.h`) gets the remote's light, this library decodes it into "which remote, which key, held or not", and the remote's module (`module/remote_17_key.h`) turns that into keys with fixed meanings (`KEY_1`, `KEY_UP`…). It can also **send** the other way: an infrared LED as a remote (`transmitter.h`). The folder is named after the protocol: every NEC remote, receiver and sender uses the same protocol.

## Usage

```cpp
#include "Uno_interrupt_pin.h"   // include it in one .cpp only (it defines the interrupts)
#include "NEC.h"

using Remote = NEC::Remote17KeyNumbersTop;                    // your remote (see Remotes)
constexpr uint8_t IR_PIN = 2;                                 // the receiver's OUT (S)
using Receiver = NEC::HX1838<Uno::UnoInterruptPin<IR_PIN>>;

void setup() {
    Receiver::init();
}

void loop() {
    NEC::Code code;
    while (Receiver::read(code, 3)) {                          // 3: repeating starts only after holding about 0.3 s
        NEC::Key key = Remote::toKey(code);
        if (key == NEC::KEY_UP) {
            // just pressed (code.repeats == 0), or being held (code.repeats >= 3)
        }
    }
}
```

Complete examples: `src/week_5_NEC/tests/1_receiver.cpp` (prints the codes received to the serial, to find out which your remote is), `src/week_5_NEC/assignments/`.

## Sending

```cpp
#include "Uno_ir_carrier.h"
#include "NEC.h"

constexpr uint8_t IR_LED_PIN = 3;                     // infrared LED: D3 or D11 (38 kHz made by Timer2)
using Sender = NEC::IRLED<Uno::UnoIrCarrier<IR_LED_PIN>>;

Sender::init();
Sender::send(0x00, 0x45);                              // address, command
Sender::sendKey<NEC::Remote17KeyNumbersTop>(NEC::KEY_OK);   // by the key's name
Sender::hold(0x00, 0x1C, 1000);                        // held 1 second: one press, then a repeat code every 108 ms
```

| Function | What it does |
|---|---|
| `init()` | the LED off |
| `send(address, command)`, `send(code)` | one press: 67.5 ms (waits); an address > 255 uses extended NEC (16-bit addresses) |
| `sendKey<Remote>(key)` | a key of the remote; returns false when that remote has no such key |
| `sendRepeat()` | one repeat code ("still held"), to send 108 ms after the last one |
| `hold(address, command, ms)`, `holdKey<Remote>(key, ms)` | held for ms: a press + a repeat code every 108 ms |

| Wiring | Pin | UNO |
|---|---|---|
| bare infrared LED (940 nm, clear or light blue) | long leg | 220 Ω → **D3** (or D11) |
| | short leg | GND |
| KY-005 board | S | 220 Ω → **D3** (many boards have no resistor) |
| | middle, - | 5V (not connected on some boards), GND |

- **Why only D3 or D11**: Timer2 puts the 38 kHz straight out of these two pins, without the CPU. The PWM of D3 / D11 and `UnoPcmOut` cannot be used at the same time.
- **Range**: 220 Ω is about 15 mA, 1 ~ 2 m; 100 Ω about 30 mA (a pin's limit is 40 mA, no smaller). For 5 ~ 10 m like a real remote, drive about 100 mA through an NPN transistor (2N2222, S8050).
- **The light is invisible**: look through a phone camera and you will see the LED blink.
- **Sending waits**: a press takes 67.5 ms, during which `loop()` does nothing; interrupts go on (`millis`, serial).
- Example: `src/week_5_NEC/tests/3_transmitter.cpp` (pressing keys on the serial sends them, and the receiver gets them back for checking).

## Wiring

| Receiver | UNO |
|---|---|
| OUT (S on modules, or the left leg with the chip facing you) | any digital pin, e.g. D2 |
| GND (-) | GND |
| VCC (+) | 5V |

- **The chip's own pins**: with the dome of an HX1838 / VS1838B facing you and the legs down, from left to right: **OUT, GND, VCC** (2.7 ~ 5.5 V).
- **Soldered on a small board (KY-022 and others)**: the board reroutes the pins and the order is often different (the middle is often +); **wire by what is printed on the board** (S = OUT, + = VCC, - = GND).
- **No resistor**: the output is already 0 / 5 V.
- **The remote**: the 17-key one takes a CR2025 coin cell; the first time, pull out the clear strip next to the battery.

## Names

Sellers often mix several names together; they are really things on different levels:

| Name | What it is | This library |
|---|---|---|
| **HX1838**, **VS1838B** | the receiver "chip": black, with a dome in front, 3 pins. The two are the same thing by different makers: they receive 38 kHz infrared and output the demodulated signal | `module/HX1838.h` (both names, the same code) |
| **KY-022** | a "module": the chip above soldered on a small board with an LED and a resistor, S, +, - marked | the same |
| **NEC** | the "protocol": how a remote shows 0 and 1 with lengths of light (see [the NEC protocol](INTERFACE.md#nec-協定)) | this folder, `Receiver` |
| 17-key remote | the "remote": each key sends a command code. **Different remotes have different codes** | `module/remote_17_key.h` |

So the folder is `NEC` (the protocol), not `infrared` (too vague) and not `HX1838` (another receiver runs the same code); receivers and remotes are parts you buy, in `module/`.

**8 pins SDA, SCK, MOSI, MISO, IRQ, GND, RST, 3.3V are not an infrared receiver**: that is the **RC522 (MFRC522) RFID reader module**: it reads cards and key fobs by 13.56 MHz radio, connects by SPI, and takes only 3.3V. It has nothing to do with infrared; if used later it will be another library (see [lib/README.md](../README.md#where-to-add-things-later)).

## Remotes

**The codes are not universal**: each remote decides what it sends. Even those called "17 keys" come in two layouts with different commands:

| | `Remote17KeyNumbersTop` | `Remote17KeyArrowsTop` |
|---|---|---|
| layout | 1 ~ 9, * 0 # on top, arrows and OK below | arrows and OK on top, 1 ~ 9, * 0 # below |
| address | 0x00 | 0x00 |
| 1 2 3 | 0x45 0x46 0x47 | 0x16 0x19 0x0D |
| 4 5 6 | 0x44 0x40 0x43 | 0x0C 0x18 0x5E |
| 7 8 9 | 0x07 0x15 0x09 | 0x08 0x1C 0x5A |
| * 0 # | 0x16 0x19 0x0D | 0x42 0x52 0x4A |
| ▲ ◀ OK ▶ ▼ | 0x18 0x08 0x1C 0x5A 0x52 | 0x46 0x44 0x40 0x43 0x15 |

Both follow the codes commonly found online; **check your remote first with `src/week_5_NEC/tests/1_receiver.cpp`**: press a few keys, see which column's names match what you pressed, and use that one in the `using Remote = …` at the top of the assignments. If neither matches, add a file in `module/` with the commands `src/week_5_NEC/tests/1_receiver.cpp` prints.

**Another remote changes one line**: programs use only meanings like `KEY_1`, `KEY_UP`, never the codes. TV and air conditioner remotes work too, as long as they are NEC: add a struct with `ADDRESS` and `toKey` in `module/`.

## Repeats

| Situation | What is sent |
|---|---|
| a quick press | one full code; held over about 0.1 second, one more repeat code (the second code comes 108 ms after the press) |
| held | one full code, then a repeat code every 108 ms (about 9 a second), stopping on release |
| two quick presses | two full codes |

- **It depends on the remote**: NEC remotes (both 17-key ones included) send repeat codes while held; some other protocols keep resending the full code, or send nothing.
- **A normal press often has 1 repeat too**: so for "one press, one step" use `read(code, 3)`: repeating starts only after holding about 0.3 s, like a computer keyboard.
- **Keys that must not repeat** (OK, clear): skip when `code.repeats > 0`.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| pin | 0 ~ 19; D0 / D1 compile warning | compile error |
| timing tolerance | about ±30% | not received (noise or another protocol) |
| check | the command and its inverse must match | not received, `badCount()` + 1 |
| protocol | NEC, extended NEC | not received (Sony, RC5, long codes of air conditioners…) |
| interval of `read` | each code waits in the queue, at most `QUEUE` | when full, new ones are dropped; held keys keep sending repeats, and reading too seldom loses a few repeats |
| `QUEUE` | 1 ~ 255 | 0: compile error |
| distance, angle | about 5 ~ 8 m, roughly aimed at the receiver | nothing received |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| `Uno_interrupt_pin.h` included by two .cpp | the interrupt functions are defined twice, a link error | include it in one .cpp only |
| two `UnoInterruptPin` in the same group of pins | the one `init`ed later replaces the other | move one to another group (D0 ~ D7, D8 ~ D13, A0 ~ A5) |
| sunlight, fluorescent lamps, a phone camera shining on the receiver | noise now and then, `badCount()` goes up | normal; noise never becomes a key |
| together with the DS18B20 (`UnoOneWire`) | interrupts are off about 70 µs every bit, the measured lengths are a bit off | within the tolerance: they can be used together |
| nothing received | — | look at the front of the remote through a phone camera: pressing shows a purple light; if not, change the battery or pull the insulating strip; does `src/week_5_NEC/tests/1_receiver.cpp` print anything |
| the remote's keys do not match | the wrong `Remote` | check with `src/week_5_NEC/tests/1_receiver.cpp` |
