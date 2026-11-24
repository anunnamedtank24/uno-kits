# Uno

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| Arduino's `millis()`, `micros()` (Arduino UNO, ATmega328P) | the UNO version of the interfaces other libraries need: Bus (`UnoST7789`, `UnoI2C`), Link (`UnoSerial`), Wire (`UnoOneWire`), Pin, Pwm, InterruptPin, AnalogPin (`UnoInputPin`, `UnoPwmPin`, `UnoInterruptPin`, `UnoAnalogPin`), Tone (`UnoTone`), Carrier (`UnoIrCarrier`), SampleOut (`UnoPcmOut`), background tasks (`UnoBackground`); a wrong pin is stopped when compiling |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

The register layer of the Arduino UNO (ATmega328P), `namespace Uno`, the "board" of the four categories. This is the lowest level: the interfaces other libraries need (the "Needs" column) have their UNO versions here; the functions of each are in [the interfaces provided](INTERFACE.md#提供的介面). For another board, write the same functions for that board.

## Usage

Include the file of each interface you use; `Uno.h` never includes the interfaces the other way round:

```cpp
#include "Uno_ST7789.h"   // ST7789 screens
#include "Uno_I2C.h"      // I2C devices (OLED)
#include "Uno_serial.h"   // streaming pictures, videos from the computer
// #include "Uno_serial_buffer.h"  // or instead of Uno_serial.h: UnoSerial<BAUD, RX_SIZE, TX_SIZE> with buffers, e.g. Bluetooth (one .cpp only)
// #include "Uno_soft_serial.h"    // a UART on any two pins (software), e.g. Bluetooth while D0 / D1 stay USB (one .cpp only)
#include "Uno_one_wire.h" // 1-Wire sensors
#include "Uno_pin.h"      // switches, active buzzers, LEDs, motor drivers
#include "Uno_analog_pin.h"  // analog sensors (ADC)
#include "Uno_interrupt_pin.h" // infrared receivers, ultrasonic (include it in one .cpp only)
#include "Uno_background.h"    // update() in the background (include it in one .cpp only)
#include "Uno_tone.h"     // passive buzzers (include it in one .cpp only)
#include "Uno_pcm_out.h"  // samples (PCM): voices, real waveforms (include it in one .cpp only)

constexpr uint8_t DC_PIN = 8;
constexpr uint8_t RES_PIN = 10;
constexpr uint8_t BLK_PIN = 9;
using Bus = Uno::UnoST7789<DC_PIN, RES_PIN, Uno::NONE, BLK_PIN>;    // no CS
```

Complete examples: every program in `src/` (the parts section); `src/week_7_L298N/tests/7_background_remote.cpp` (`UnoBackground`), `src/week_1_analog/tests/1_sensors.cpp` (`UnoAnalogPin`, `UnoChip`).

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| pins (template) | 0 ~ 19, or `Uno::NONE` (optional pins only) | compile error; D0 / D1 compile warning |
| pins of `UnoPwmPin` | 3, 5, 6, 9, 10, 11 | compile error |
| pins of `UnoAnalogPin` | 14 ~ 19 (A0 ~ A5) | compile error |
| pins of `UnoPcmOut` | 9, 10 | compile error |
| pins of `UnoIrCarrier` | 3, 11 | compile error |
| `MODE` | 0 ~ 3 | compile error |
| `BAUD` | 2000 ~ 2000000, off by ≤ 5% | compile error; off by > 2% compile warning |
| `KHZ` of `UnoI2C` | 1 ~ 400 | compile error |
| `init(blk)`, `setBacklight(blk)` | 0 ~ 255; when BLK is not a PWM pin only 0 = off, anything else = on | — |
| `read8(byte, timeoutMs)` | 0 ~ 65535 ms | — |
| `UnoTone::start(hz)` | 16 ~ 20000, 0 = stop | clamped |
| `UnoPcmOut::start(rate, next)` | 1954 ~ 32000 Hz | clamped |
| `readAnalogAverage(count)` | 1 ~ 64 | clamped |
| `UnoBackground::add` | at most 6 functions | returns false |
| `readFlash8/16(p)` | must point to `PROGMEM` data | not checked: wrong data is read |
| `delayMs(ms)` | 0 ~ 65535 | — |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| the computer opens the serial port | the board resets (DTR), the bootloader waits about 1 second | start the program on the computer first, then upload |
| `Uno_tone.h`, `Uno_interrupt_pin.h`, `Uno_pcm_out.h`, `Uno_background.h` included by two .cpp | the interrupt functions are defined twice, a link error | include them in one .cpp only (this project has one .cpp at a time) |
| two `UnoInterruptPin` in the same group of pins (D0 ~ D7, D8 ~ D13, A0 ~ A5) | the one `init`ed later replaces the other | put them in different groups |
| `UnoTone` together with `pwmSet` on D9 / D10 (e.g. the BLK of `UnoST7789` on D9) | both change Timer1: frequency and brightness both wrong | move BLK to D3, D5, D6, D11, or `Uno::NONE` |
| `UnoTone` on a pin other than D9 / D10, together with `UnoOneWire` | the square wave pauses while interrupts are off, the sound wobbles | put the buzzer on D9 or D10 (no interrupts) |
| a reference voltage other than `REFERENCE_EXTERNAL`, with a voltage on the AREF pin | the internal reference and the external voltage are shorted, may burn out | when AREF has something connected, always use `REFERENCE_EXTERNAL` |
| a background function too slow (over 1 ms) | the next round is skipped (`skipped()` goes up) | put only non-waiting functions like `update()` in the background |
| a pin driving something power-hungry | ≤ 20 mA a pin recommended (40 mA absolute maximum), ≤ 200 mA for the whole chip | drive it through a transistor |
