# bluetooth

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| `Link`: a UART (`Uno::UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>`: any two pins, D0 / D1 stay USB; or `Uno::UnoSerial<BAUD, RX_SIZE, TX_SIZE>` of `Uno_serial_buffer.h` on D0 / D1); `Key` (EN as KEY) or `Power` (EN as the power switch): an output pin (`Uno::UnoOutputPin<PIN>`); `State`: an input pin for STATE (`Uno::UnoInputPin<PIN>`); any pin may be `bluetooth::NoPin` | `bluetooth::HC05`: sending, receiving, name, PIN, baud, role by AT commands through EN; `bluetooth::HC06`: the same without EN / KEY; `bluetooth::BT05`: a BLE module (FFE0 / FFE1) with the same UART side; `bluetooth::Port`: any module, also a Link itself; `bluetooth::Packet`: packets with a CRC-8 over the byte stream |
| the computer: Python 3, `pip install pyserial` (BLE: also `pip install bleak`) | `source/packet.py`: sends and receives the same packets over the Bluetooth serial port or BLE; `source/ble.py`: a BLE module used like a serial port |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Bluetooth serial modules (SPP), `namespace bluetooth`. The module does all of Bluetooth by itself (being found, pairing, the connection); the board sees only a UART, and the computer only a serial port. Whatever the board writes to the module's RXD comes out of the computer's serial port, and the other way round. The format of the data is the program's own: `Packet` is one.

```
the board  ──TXD/RXD──  module  ~~ radio (RFCOMM / SPP) ~~  computer: a serial port (/dev/cu.HC-05, COM5)
```

## Usage

```cpp
#include "Uno_pin.h"
#include "Uno_soft_serial.h"
#include "bluetooth.h"

constexpr uint8_t BT_RX_PIN = A0;           // <- the module's TXD
constexpr uint8_t BT_TX_PIN = A1;           // -> the module's RXD (through a divider)
constexpr uint8_t KEY_PIN = 12;
constexpr uint8_t STATE_PIN = 4;
constexpr uint32_t BAUD = 9600;             // the module's data baud
constexpr uint16_t RX_BUFFER = 64;          // received bytes kept by the interrupt while loop() is busy

using Link = Uno::UnoSoftSerial<BT_RX_PIN, BT_TX_PIN, BAUD, RX_BUFFER>;
using Bluetooth = bluetooth::HC05<Link, Uno::UnoOutputPin<KEY_PIN>, Uno::UnoInputPin<STATE_PIN>>;

void setup() {
    Bluetooth::init();                      // the UART, EN LOW (data mode), STATE
    Bluetooth::setName("UnoKits");          // AT commands: EN HIGH, the command, the answer, EN LOW
    Bluetooth::setPin("1234");
}

void loop() {
    if (Bluetooth::isConnected() && Bluetooth::available()) {
        uint8_t byte;
        Bluetooth::read8(byte, 0);
        Bluetooth::write8(byte);            // sends it back
    }
}
```

Everything is static, so use `Bluetooth::` without declaring a variable. The library does not care which UART it gets: see [Which pins](#which-pins-a-software-uart-or-d0--d1). Every AT function returns `OK` (0) or an error code (see [Results](#results)). Complete examples: `src/week_9_bluetooth/tests/1_HC_05.cpp`, `2_HC_06.cpp` (a bridge USB ↔ Bluetooth), with `round_trip.py` on the computer.

## Which pins: a software UART or D0 / D1

| | `Uno::UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>` (`Uno_soft_serial.h`) | `Uno::UnoSerial<BAUD, RX_SIZE, TX_SIZE>` (`Uno_serial_buffer.h`) |
|---|---|---|
| pins | any two; RX uses its group's pin change interrupt | D0 / D1 only: the hardware UART, shared with USB |
| upload, `pio device monitor` | work with the module connected | unplug the module's TXD to upload; no monitor |
| speed | 1200 ~ 38400 baud (warning above 19200) | any |
| sending | waits about 1 ms a byte at 9600 | a send buffer: returns at once |
| best for | trying things out, printing on USB meanwhile (`src/week_9_bluetooth`) | the finished thing on a battery, no USB |

Only `using Link = …` changes; `HC05`, `HC06`, `Packet` stay as they are. Without any buffer (`Uno::UnoSerial<BAUD>` of `Uno_serial.h`) it works too, but the UNO then keeps only 2 received bytes: `loop()` must never be busy for long.

## HC-06: without EN / KEY

```cpp
using Bluetooth = bluetooth::HC06<Link>;                                   // 4 pins
using Bluetooth = bluetooth::HC06<Link, Uno::UnoInputPin<STATE_PIN>>;      // 6 pins: STATE too
```

- **AT commands only while nothing is connected**: once a computer connects, every byte is data. Send them in `setup()`, right after power on.
- **About a second each**: the HC-06 takes a pause as the end of a command, so `HC06` waits `GAP_MS` between two commands.
- **With STATE wired**, the AT functions return `ERROR_CONNECTED` while connected, without sending anything; without it they cannot tell, and the commands go to the computer as text.

The same functions as `HC05`, except `setRole()` and `restart()` (the HC-06 has no such commands).

## BT05: BLE (the HM-10 family)

```cpp
constexpr uint8_t EN_PIN = 12;              // EN: this board's power switch
using Bluetooth = bluetooth::BT05<Link, Uno::UnoInputPin<STATE_PIN>, Uno::UnoOutputPin<EN_PIN>>;
```

Some boards that look like an HC-05 carry a BLE module instead (it answers `AT+UUID` with `+UUID=FFE0`). The UART side is the same, so `Port`, `Packet` and the wiring stay; the radio side is not:

| | HC-05 / HC-06 (classic, SPP) | BT05 (BLE) |
|---|---|---|
| on the computer | pair once, then a serial port (`/dev/cu.NAME`, `COM5`) | no serial port: connect by name with GATT, service FFE0, characteristic FFE1 (`source/ble.py`, `pip install bleak`) |
| pairing | needed (PIN) | not needed |
| iPhone | no (no SPP on iOS) | yes (any BLE terminal app) |
| a write | any length | at most 20 bytes (`ble.py` cuts and paces it) |
| AT commands | `module/HC_05.h`, `HC_06.h` | `module/BT05.h`: `AT+NAMExxx`, `AT+PIN` with 6 digits, `AT+BAUDn`, `AT+VERSION`, `AT+LADDR` |

- **AT commands only while nothing is connected**, like the HC-06; with STATE wired the AT functions return `ERROR_CONNECTED` meanwhile.
- **Right after a new name** the module ignores commands for a while (it sets up its advertisement): `BT05` sends a command again when it got no answer at all (`RETRY_MS`, `TRIES`).
- **EN as a power switch**: on the board measured, EN switches the module on (HIGH) and off (LOW), and it keeps its last level when not driven. `Power` keeps it HIGH from `init()`; `setPower(false)` turns the module off. An HC-05 board wired the same way: `bluetooth::HC05PowerSwitch<Link, En, State>`, or `HC05ByEn<Link, En, State, EN_IS_POWER>::Type` to pick by a bool. `src/week_9_bluetooth/tests/3_diagnose.cpp` tells which one a board is.

## Any module: Port

```cpp
using Bluetooth = bluetooth::Port<Link>;                                       // only TXD / RXD
using Bluetooth = bluetooth::Port<Link, bluetooth::NoPin, Uno::UnoInputPin<STATE_PIN>>;
```

`Port` is the UART with the two optional pins and nothing of a particular module: sending, receiving, `isConnected()`, `setCommandMode()`. An HC-05 whose EN is not wired, or another SPP module, works with it. `Port` (and so `HC05`, `HC06`) is a Link itself: `stream::Puller<Bluetooth, CHUNK>` pulls files over Bluetooth too.

## Packets

Bluetooth serial gives only bytes: a packet may arrive in pieces, or two may stick together. `Packet` adds where a packet starts, how long it is and a CRC:

```
┌──────┬────────┬─────────┬───────────────┬──────┐
│ 0xAA │ LENGTH │ COMMAND │ DATA (LENGTH) │ CRC  │
└──────┴────────┴─────────┴───────────────┴──────┘
 START   0 ~ 255  any byte  numbers: low     CRC-8 (polynomial 0x07, start 0)
                  ('L')     byte first       of LENGTH, COMMAND, every DATA byte
```

```cpp
bluetooth::Packet<Bluetooth, PACKET_MAX_LENGTH> packets;

packets.init();                                 // Bluetooth::init() too
// in loop:
if (packets.update()) {                         // call it all the time, it never waits
    if (packets.command() == 'M') {
        int16_t left = packets.readInt16(0);    // DATA bytes 0, 1
        int16_t right = packets.readInt16(2);   // DATA bytes 2, 3
    }
}
packets.sendText('S', "ready");                 // to the computer
```

| Function | What it does |
|---|---|
| `update()` | reads what has arrived; true when a whole packet with the right CRC is in |
| `command()`, `length()`, `data()` | the packet, good until the next `update()` |
| `readUint8(i)`, `readInt8(i)`, `readUint16(i)`, `readInt16(i)` | numbers from DATA byte `i` (two bytes low first); 0 beyond the length |
| `send(command, data, length)`, `sendText(command, text)` | sends a packet |
| `dropped()` | packets dropped: wrong CRC, too long, or cut off |

- A broken packet is dropped and the next `0xAA` starts again: a lost byte loses one packet, never the following ones.
- `Packet` takes any Link: `Packet<Uno::UnoSerial<BAUD>>` sends the same packets over USB.

### The computer: packet.py

```bash
pip install pyserial
python lib/bluetooth/source/packet.py --list                  # the serial ports, Bluetooth ones marked
python lib/bluetooth/source/packet.py --port /dev/cu.UnoKits  # then type packets
python lib/bluetooth/source/packet.py --ble TanksUno          # BLE (BT05): by its name, pip install bleak
python lib/bluetooth/source/ble.py --scan                     # the BLE devices around, FFE0 ones marked
```

macOS asks once to allow Bluetooth for the terminal app (Terminal, iTerm, VS Code); without it no BLE device is ever found.

```
L 1                  'L' with one byte 1
M h:200 h:-100       'M' with two int16 (low byte first, like readInt16)
E s:hello            'E' with the bytes of "hello"
[BOARD] 'S' 5 bytes: 72 65 61 64 79  "ready"
```

| Typed | Bytes |
|---|---|
| `200`, `-5` | one byte (0 ~ 255, -128 ~ -1) |
| `h:-100`, `H:500` | two bytes, int16 / uint16, low first |
| `s:text` | the text's bytes |

In your own Python, `encode(command, data)`, `Decoder().feed(bytes)` and `crc8(bytes)` of `packet.py` make and read the same packets.

## Against noise

Errors come from several places; each layer catches its own:

| Layer | What goes wrong | What catches it |
|---|---|---|
| wires (TXD / RXD) | motors, long wires, a floating RXD: bits flip | the divider holds RXD; common GND; 100 nF + 10 µF at the module's VCC; short wires away from motor wires |
| UART | a broken byte (the stop bit not HIGH) | `UnoSerial` drops it, `brokenCount()` |
| UART | `loop()` busy, bytes not read in time | the receive buffer (`RX_SIZE`): the interrupt keeps them; still too many: `lostCount()` |
| radio | interference on 2.4 GHz | Bluetooth itself: every radio packet has its own CRC and is sent again until it arrives (nothing to do) |
| packets | a byte lost or changed, two packets stuck together | `Packet`: START, LENGTH, CRC-8 (every 1 ~ 2 bit error, every burst up to 8 bits), `GAP_MS`; dropped, `dropped()`; the next START begins again |
| the program | a packet dropped | answer every command (like `L` → `L`): the computer sends again when no answer comes |

So over Bluetooth the radio itself is safe; what breaks is almost always the wire between the board and the module, or the board reading too late.

## STATE: is anything connected

```cpp
if (Bluetooth::isConnected()) { … }      // filtered: call it often
bool level = Bluetooth::readState();     // the pin as it is now
```

| | |
|---|---|
| what it is | an output of the module: the HC-05's pin 32 (LED2), LOW until paired, HIGH while a computer / phone is connected; on 6-pin HC-06 boards the same, or a copy of the LED |
| wiring | straight to any pin, **no divider** (it is an output of the module, 3.3 V into the UNO); a 10 kΩ from the pin to GND keeps it LOW when the board leaves STATE unconnected |
| the level | 3.3 V is HIGH for the UNO (HIGH from 3.0 V), with only 0.3 V to spare: noise can flip it |
| the filter | `isConnected()` turns true only after STATE has stayed HIGH for `CONNECTED_MS`, false after LOW for `DISCONNECTED_MS`: a glitch, noise or a STATE blinking with the LED never counts |
| the cost | connecting is noticed `CONNECTED_MS` late, disconnecting `DISCONNECTED_MS` late; false for `CONNECTED_MS` after `init()` |
| what uses it | the program (`1_HC_05.cpp` forwards lines only while it is true); `HC06` refuses AT commands while connected (it watches STATE during its `GAP_MS` wait) |
| without it | `NoPin`: `isConnected()` is a compile error; data still works, the board just cannot tell |

## Wiring

| HC-05 | UNO | Explanation |
|---|---|---|
| VCC | 5V | the board's regulator takes 3.6 ~ 6 V and makes the module's 3.3 V |
| GND | GND | |
| TXD | **`BT_RX_PIN`** (or D0) | 3.3 V out: the UNO reads it as HIGH directly |
| RXD | **`BT_TX_PIN`** (or D1) through a divider | the module takes 3.3 V: TX ─ 1 kΩ ─┬─ RXD, ┬─ 2 kΩ ─ GND |
| EN | any pin (`EN_PIN`) through a divider | KEY on most boards (HIGH = AT commands), the power switch on some (HIGH = on); HC-06: not wired |
| STATE | any pin (`STATE_PIN`), directly (no divider), or not wired | HIGH while connected; see [STATE](#state-is-anything-connected) |

```
BT_TX_PIN ──[1 kΩ]──┬── RXD        5 V × 2 / (1 + 2) = 3.3 V
                    └──[2 kΩ]── GND
```

- **On D0 / D1 instead** (the hardware UART, shared with USB): the module's TXD fights the USB chip on D0, so **unplug the module's TXD (or VCC) while uploading**, and `pio device monitor` cannot be used at the same time.
- **Power**: about 30 ~ 40 mA while waiting or pairing, about 8 ~ 10 mA connected: from the 5V pin, never from a pin.
- **No pull-up on RXD**: the divider holds it; a floating RXD picks up noise as data.
- **Near motors**: 100 nF + 10 µF between the module's VCC and GND, its GND to the UNO's GND by the shortest wire, TXD / RXD away from the motor wires.

## Names

| Name | What it is |
|---|---|
| **SPP** (Serial Port Profile) | the way of communicating: Bluetooth pretending to be a serial cable (over RFCOMM); the computer gets a serial port |
| **HC-05** | a module that can be master or slave, AT commands through EN (KEY, the module's pin 34), in `module/HC_05.h` |
| **HC-06** | a slave-only module, AT commands only while unconnected, in `module/HC_06.h` |
| **EN / KEY** | the same pin by two names: HIGH = AT command mode; on the HC-06 it does nothing; on some boards EN is the module's power switch instead |
| **BLE**, **BT05** | Bluetooth low energy, another radio than SPP: GATT service FFE0, characteristic FFE1; `module/BT05.h` (the HM-10 family of copies) |
| **STATE** | HIGH while connected (the module's pin 32 on the HC-05) |
| **AT commands** | text commands to the module itself (`AT+NAME=…`): name, PIN, baud, role |

By the rule "`lib/` is named after the chip or the way of communicating": the chips differ between makers (CSR BC417 and copies), but every one of them speaks SPP over a UART, so the folder is named after the way of communicating; other SPP modules (JDY-31) will go into `module/`. BLE modules with a transparent UART (BT05, HM-10) are here too: the board's side is the same UART and the same packets; only the computer's side differs (`source/ble.py`).

## How it works

| Step | Classic Bluetooth (HC-05, HC-06) |
|---|---|
| 1. being found | unconnected, the module listens for "who is there?" (inquiry) and answers with its address, then its name |
| 2. pairing | the computer asks for the PIN; both make a link key from the PIN, the addresses and a random number (the PIN itself is never sent) and keep it: no PIN next time |
| 3. connecting | opening the serial port connects (RFCOMM); STATE goes HIGH, the LED changes; closing it disconnects |
| 4. data | bytes both ways, 8N1 at the data baud on the UART side |

- **The baud** is only the speed between the board and the module (the UART); the radio has its own speed. The program's `BAUD` and the module's data baud must be the same; the baud the computer sets for a Bluetooth serial port does nothing.
- **HC-05, two AT modes**: EN HIGH while powering on: 38400 baud, every command; EN HIGH after powering on: the data baud, most commands. `HC05` uses the second, so the UNO's UART never changes its speed; it even works while connected.

## The AT commands

| Function | HC-05 | HC-06 |
|---|---|---|
| `check()` | `AT` | `AT` |
| `setName(name)` | `AT+NAME=name` | `AT+NAMEname` |
| `setPin(pin)` | `AT+PSWD=1234`, then `AT+PSWD="1234"` (firmware 3.0) | `AT+PIN1234` |
| `setBaud(baud)` | `AT+UART=baud,0,0` (after `restart()`) | `AT+BAUDn` |
| `setRole(role)` | `AT+ROLE=n`: `ROLE_SLAVE`, `ROLE_MASTER`, `ROLE_SLAVE_LOOP` | — |
| `version(reply, size)` | `AT+VERSION?` | `AT+VERSION` |
| `restart()` | `AT+RESET` | — |
| `command(text, reply, size)` | any command, `\r\n` added | any command, sent as it is |
| ends with | `\r\n` | a pause (at most one command a second) |

- **Saved in the module**: the name, PIN and baud stay after power off.
- **After `setBaud`**: the module talks at the new baud from its next start; change the program's `BAUD` the same and upload again, or nothing will understand each other.
- **The name** may show the old one on a computer that paired before: remove the device there and pair again.

## Results

| Result | Meaning | Common causes |
|---|---|---|
| `OK` | the module took it | |
| `ERROR_NO_ANSWER` | no answer, or one not understood, in time | the program's `BAUD` is not the module's; TXD / RXD swapped; EN not wired (HC-05); a connected HC-06 |
| `ERROR_REFUSED` | the module answered with an error | a command this firmware does not know |
| `ERROR_BAD_VALUE` | the value is not allowed: not sent | a name too long or empty, a PIN not 4 digits, a baud not in the list |
| `ERROR_CONNECTED` | HC-06 with STATE: connected, not sent | disconnect first, or send the commands right after power on |

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| name | 1 ~ `MAX_NAME_LENGTH` printable characters | returns an error (`ERROR_BAD_VALUE`) |
| PIN | exactly 4 digits | returns an error (`ERROR_BAD_VALUE`) |
| `setBaud` of HC-05 | 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400 | returns an error (`ERROR_BAD_VALUE`) |
| `setBaud` of HC-06 | 1200, 2400 and the HC-05's list | returns an error (`ERROR_BAD_VALUE`) |
| `setRole` | `ROLE_SLAVE`, `ROLE_MASTER`, `ROLE_SLAVE_LOOP` | returns an error (`ERROR_BAD_VALUE`) |
| `Key` of `HC05` | a pin | compile error |
| `isConnected()` without `State`, `setCommandMode()` without `Key` | | compile error |
| `MAX_LENGTH` of `Packet` | 1 ~ 255 | compile error |
| `RX_SIZE`, `TX_SIZE` of `UnoSerial` | 0, or a power of 2 4 ~ 256; > 0 needs `Uno_serial_buffer.h` | compile error |
| a received packet's LENGTH | 0 ~ `MAX_LENGTH` | dropped (`dropped()`) |
| a reply of `version`, `command` | `size` - 1 characters | clipped |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| uploading with the module's TXD on D0 | the upload fails | unplug TXD (or VCC) while uploading, or use `UnoSoftSerial` on other pins |
| `UnoSoftSerial`'s RX pin in the group of another interrupt pin (infrared receiver D2, Echo D8) | the one `init`ed later takes the group | RX in a group of its own (e.g. A0 ~ A5) |
| `loop()` busy for long (drawing a screen, `delay`) without buffers | bytes lost: the UNO keeps only 2 (about 2 ms at 9600 baud); `lostCount()`, `dropped()` grow | `Uno_serial_buffer.h` with `RX_SIZE`; with buffers, longer than `RX_SIZE` bytes' time still overflows |
| `brokenCount()` grows | noise on the wire to the board's RX, or the wrong `BAUD` | see [Against noise](#against-noise) |
| the HC-05's LED blinks slowly at power on | EN was HIGH while powering on: the 38400 AT mode, no data | EN LOW (or not wired) at power on; `restart()` always ends with EN LOW |
| HC-06 reset while connected | the AT commands go to the computer as text | wire STATE, or ignore them on the computer |
| a new HC-06 PIN does not work | it takes effect only after the module has been without power for a while | unplug it for a while |
| an HC-06 copy answers nothing to `check()` | firmware "3.0-…" wants `\r\n` like an HC-05 | `command("AT\r\n")`, `command("AT+NAME=x\r\n")` |
| iPhone | no SPP on iOS: HC-05 / HC-06 cannot be used | a BLE module: `BT05` |
| a BT05 never shows up in the computer's Bluetooth settings | normal: BLE, no pairing, no serial port | `ble.py --scan`, then `--ble NAME` |
| `ble.py` finds nothing on macOS | Bluetooth not allowed for the terminal app | System Settings → Privacy & Security → Bluetooth |
| sending from the computer before the board is ready | lost | wait until the board says it is ready (`round_trip.py` waits for `[INFO] ready` and the connection) |
