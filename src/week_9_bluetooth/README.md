# week_9_bluetooth

English | [中文](讀我.md)

Tests of the HC-05, HC-06 (classic Bluetooth) and BT05 (BLE) modules ([lib/bluetooth](../../lib/bluetooth/README.md)): a round trip. The computer sends a line to the board over the USB serial, the board sends it over Bluetooth, and it comes back to the same computer through its Bluetooth serial port; the other way round too. The module is on a software UART (`Uno::UnoSoftSerial`), so D0 / D1 stay USB: upload and the USB serial work with the module connected.

Every `.cpp` is `.bak` now; keep the one to run:

```bash
python src/hide.py week_9_bluetooth/tests/1_HC_05     # keep only this one (drop its .bak), the others become .bak
pio run -t upload
python src/week_9_bluetooth/tests/round_trip.py --bt /dev/cu.<NAME>     # HC-05 / HC-06 (classic)
python src/week_9_bluetooth/tests/round_trip.py --ble <NAME>            # BT05 (BLE): pip install bleak
```

Which module is it? `tests/3_diagnose.cpp` first: `+UUID=FFE0` to `AT+UUID` (or the AT answers of `4_BT05.cpp`) means BLE.

| File | Contents | Hardware needed |
|---|---|---|
| `tests/1_HC_05.cpp` | the board: name and PIN through EN, then a bridge USB ↔ Bluetooth; STATE says whether lines can go out | HC-05 (EN, STATE wired) |
| `tests/2_HC_06.cpp` | the same without EN / KEY (the AT commands right after power on) and without STATE (lines always go out) | HC-06 (4 pins) |
| `tests/4_BT05.cpp` | the same bridge for a BT05-type BLE module (`+UUID=FFE0`) on a board whose EN is the power switch: name, PIN (6 digits), firmware, address on USB | BT05 (EN, STATE wired) |
| `tests/3_diagnose.cpp` | checks the wiring and the module on USB: whether EN is KEY or the power switch, the TXD line, STATE, `AT` at every baud (the raw answers) | the module, no computer Bluetooth |
| `tests/round_trip.py` | the computer: opens USB and Bluetooth (`--bt PORT`) or BLE (`--ble NAME`), sends lines, shows each one coming back with its time and whether it is the same | Python 3, `pip install pyserial` |

## Wiring

| Part | Pin | UNO | Which programs |
|---|---|---|---|
| HC-05 / HC-06 | VCC | 5V | all |
| | GND | GND | |
| | TXD | **`BT_RX_PIN`** | |
| | RXD | **`BT_TX_PIN`** through 1 kΩ, with 2 kΩ from RXD to GND | |
| HC-05 | EN | **`EN_PIN`** through the same divider | `tests/1_HC_05` |
| | STATE | **`STATE_PIN`**, directly | `tests/1_HC_05` |
| USB | D0, D1 | upload, the USB serial | all |

| Pin | The rule |
|---|---|
| `BT_RX_PIN` | any pin; it uses its group's pin change interrupt (D0 ~ D7, D8 ~ D13, A0 ~ A5): no infrared receiver (D2) or ultrasonic Echo (D8) in the same group; A0 ~ A5 is free of both |
| `BT_TX_PIN` | any pin, through the divider (the module takes 3.3 V) |
| `EN_PIN` | any pin, through the divider. What EN does depends on the board: KEY on most (`EN_IS_POWER = false`: LOW at power on, HIGH only during AT commands), the module's power switch on some (`EN_IS_POWER = true`: kept HIGH, LOW turns the module off). `tests/3_diagnose.cpp` tells which |
| `STATE_PIN` | any pin, directly (no divider: the module drives it with 3.3 V); 10 kΩ to GND if the board may leave it unconnected |
| `BT_BAUD` | the module's data baud; both modules come at 9600 (the software UART takes up to 38400) |

## Running

1. Upload (the module may stay connected).
2. Classic modules only (HC-05, HC-06): pair the module in the computer's Bluetooth settings: the name is `NAME` (a computer that paired before may show the old name: remove it and pair again), the PIN is `PAIRING_PIN`. Only once: the computer remembers it.
3. Close `pio device monitor` (one program per port), then:

```bash
python src/week_9_bluetooth/tests/round_trip.py --list                       # the ports: USB and Bluetooth marked
python src/week_9_bluetooth/tests/round_trip.py --bt /dev/cu.UnoKits          # macOS; Windows: --bt COM5 (Outgoing)
python src/week_9_bluetooth/tests/round_trip.py --bt /dev/cu.UnoKits --count 100   # 100 lines by itself, a summary
```

`--usb` is needed only when more than one USB serial is plugged in.

**BLE (`4_BT05.cpp`)**: no pairing, and the module never appears in the Bluetooth settings or as `/dev/cu.*`; `round_trip.py --ble TanksUno` finds it by name (`pip install bleak` once; macOS asks once to allow Bluetooth for the terminal app). `python lib/bluetooth/source/ble.py --scan` lists what is around. A phone connected to the module stops it advertising: disconnect it there first.

## How it runs

```
  computer (round_trip.py)               USB serial         UNO (1_HC_05.cpp)              Bluetooth
  ────────────────────────               ──────────         ─────────────────              ─────────
  opens the USB port  ────────────────── DTR resets ──────▶ setup(): AT commands to the module
  [BOARD] [INFO] AT: OK …            ◀──────────────────── each result printed
  [BOARD] [INFO] ready …             ◀────────────────────
  opens the Bluetooth port ───────────────────────────────────────────────────────────────▶ connects, STATE HIGH
                                                            loop(): isConnected() true after CONNECTED_MS
  you type: hello
    "hello\n"  ─────────────────────────────────────────▶ readUsb(): bytes → a line
                                                            forwardLine(): packet 'T' "hello" ──▶ module ~~▶
  [BOARD] [TO BLUETOOTH] hello       ◀────────────────────  printed                                        │
  read_bt: Decoder → 'T' "hello"     ◀~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~┘
  [BACK] "hello" 52 ms, same

  you type: < hi
    packet 'T' "hi" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~▶ module ──▶ BT_RX_PIN
                                                            interrupt: edges → bytes → buffer
                                                            packets.update(): CRC ok → showPacket()
  [BOARD] [FROM BLUETOOTH] hi        ◀────────────────────  printed
  [BACK] "hi" 40 ms, same
```

**The computer (`tests/round_trip.py`)**:

1. Finds the board's USB port (the one USB serial that is not Bluetooth), opens it at `MONITOR_BAUD`. Opening it resets the board, so it waits for the board's AT results and `[INFO] ready`, printing each line as `[BOARD] …`.
2. Opens the Bluetooth port: on Bluetooth, opening is what connects. Waits a moment for the board's STATE filter.
3. Two threads read all the time:
   - **USB**: every line the board prints → `[BOARD] …`; a `[FROM BLUETOOTH] text` line ends a trip Bluetooth → USB.
   - **Bluetooth**: the bytes go into `Decoder.feed()` (START, LENGTH, COMMAND, DATA, CRC-8 of `lib/bluetooth/source/packet.py`); a packet `'T'` ends a trip USB → Bluetooth; broken packets are skipped with a `[WARNING]`.
4. The main thread waits for a line you type:
   - `hello` → written to USB as `hello\n`, its time kept.
   - `< hello` → written to Bluetooth as a packet `'T'` (`encode()`), its time kept.
5. When a line comes back, it is compared with the oldest one on its way in that direction:
   - `[BACK] "hello" 52 ms, same`: back, unchanged, the round trip took 52 ms.
   - `[ERROR] … different`: back, but changed (should never happen past the CRC).
   - `[ERROR] … not back in 2.0 s: lost`: never came back (not connected, a broken packet, the board not running).
6. `--count N`: sends N random lines one after another (each after the last is back or lost), then `[INFO] N sent: … same, … different, … lost` and the shortest / longest / average round trip.
7. Ctrl+C closes both ports: Bluetooth disconnects.

**The board (`tests/1_HC_05.cpp`)**:

1. `setup()`: the USB serial (with buffers: `Uno_serial_buffer.h`); `packets.init()` starts the software UART (TX HIGH, RX interrupt), EN LOW, STATE; the AT commands (`check`, `setName`, `setPin`, `version`), each result on USB; then `[INFO] ready`.
2. `loop()`, every time round, never waiting:
   - `isConnected()`: STATE filtered; when it changes, `[INFO] connected` / `disconnected`.
   - `readUsb()`: takes the bytes the USB interrupt has kept; `\n` ends a line (`\r` dropped, longer than `LINE_MAX` cut). `forwardLine()`: not connected → `[ERROR] not connected`; else `packets.send('T', line)` over Bluetooth (about 1 ms a byte), `[TO BLUETOOTH] line` on USB.
   - `packets.update()`: takes the bytes the software UART's interrupt has kept; a whole packet with the right CRC → `showPacket()`: `'T'` → `[FROM BLUETOOTH] text` on USB, anything else → its command and length.
   - `printErrors()`: a `[WARNING]` line only when dropped packets or broken / lost bytes changed.
3. Meanwhile, in the background: the USB interrupt keeps the bytes from the computer (`USB_RX_BUFFER`) while the board is busy sending over Bluetooth; every edge on `BT_RX_PIN` runs the software UART's interrupt (a few µs), which counts bits from the time between edges and keeps each whole byte (`BT_RX_BUFFER`).

`tests/2_HC_06.cpp` is the same without EN (the AT commands only work before anything connects, about a second each) and without STATE (no `connected` lines; lines go out connected or not).

## What it shows

```
[BOARD] [INFO] HC-05: module TXD -> A0, RXD <- A1, EN <- D12, STATE -> D4, 9600 baud
[BOARD] [INFO] AT: OK
[BOARD] [INFO] AT+NAME: OK
[BOARD] [INFO] AT+PSWD: OK
[BOARD] [INFO] AT+VERSION?: OK
[BOARD] [INFO] firmware 2.0-20100601
[BOARD] [INFO] ready: pair "UnoKits", open its serial port, then type lines here
[INFO] opening /dev/cu.UnoKits (this connects Bluetooth)
[BOARD] [INFO] connected
[SUCCESS] both open: type a line (USB -> Bluetooth), or "< line" (Bluetooth -> USB); Ctrl+C ends
hello
[BOARD] [TO BLUETOOTH] hello
[BACK] "hello" 52 ms, same
< hi
[BOARD] [FROM BLUETOOTH] hi
[BACK] "hi" 40 ms, same
```

| Line | From | Meaning |
|---|---|---|
| `[BOARD] …` | the board over USB | its log: AT results, `connected`, what it forwards |
| `[BOARD] [TO BLUETOOTH] hello` | the board | the line came in on USB and went out as packet `'T'` |
| `[BOARD] [FROM BLUETOOTH] hi` | the board | a packet `'T'` came in over Bluetooth with the right CRC |
| `[BACK] "…" N ms, same` | `round_trip.py` | the line arrived at the other end, unchanged, N ms after it was sent |
| `[ERROR] … different` / `lost` | `round_trip.py` | changed / never arrived |
| `[BOARD] [WARNING] packets dropped …, bytes broken …, bytes lost …` | the board | dropped: packets broken on the way; broken: noise on the wire to `BT_RX_PIN` or a wrong `BT_BAUD`; lost: a buffer too small for how long `loop()` is busy |

- **The round trip time**: mostly sending at `BT_BAUD` (about 1 ms a byte, the packet's 4 extra bytes too) and the Bluetooth link itself (tens of ms).
- **`AT: no answer`**: EN not wired or not through the divider, TXD / RXD swapped (module TXD → `BT_RX_PIN`), or `BT_BAUD` not the module's baud.
- **`AT+VERSION?: refused` but the rest OK**: some firmware does not answer `AT+VERSION?` in this AT mode; harmless.
- **`[ERROR] not connected (STATE LOW)`** on the board (HC-05): the Bluetooth port is not open, or STATE not wired.
- **Everything `lost` on an HC-06**: no STATE to tell, so lines go out even when nothing is connected: open the Bluetooth port first.
- **`pio device monitor` instead of `round_trip.py`**: works too (type a line, it goes out over Bluetooth), but nothing compares or times what comes back.
- **HC-06**: the new PIN takes effect only after the module has been without power for a while; a copy with firmware "3.0-…" wants `\r\n`: use `Module::command(...)`.
