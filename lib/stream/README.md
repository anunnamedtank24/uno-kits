# stream

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| a `Link`: `init()`, `available()`, `read8(byte, timeoutMs)`, `write8()`, `clearInput()`, `readFlash8()` (`Uno::UnoSerial<BAUD>`) | `stream::Puller<Link, CHUNK>`: asks the computer for a file and pulls it back piece by piece; the `Stream` parameter of `image::StreamImage`, `video::StreamPlayer`, `audio::StreamPlayer`, `audio::StreamSamplePlayer` |
| the computer: Python 3, `pip install pyserial` | `source/stream.py`: one program sending pictures, animations and sounds, whatever the board asks for |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

The protocol that sends files on the computer to the board, `namespace stream`. The board side only "asks" and "pulls"; the content of a file (a picture's pixels, an animation's frames, a sound's notes) is read by image, video, audio themselves, so:

- **image, video, audio do not include this library**: `Stream` is their template parameter (duck typing), like `Screen`, so they still depend on no other library.
- **The computer runs a single `stream.py`**: there used to be one for each of the three libraries, and only one could run at a time (the same serial port); now one program handles them all.
- **No slower**: `Puller` is all templates and inline; compiled, it is the same as the code that used to be in each class.

## Usage

**The computer**: just keep it running, no arguments. It finds the board by itself, and lets go of the port while uploading.

```bash
pip install pyserial
python lib/stream/source/stream.py
```

**The UNO**:

```cpp
#include "Uno_serial.h"
#include "stream.h"
#include "image.h"

constexpr uint32_t BAUD = 1000000;          // the same as --baud of stream.py
constexpr uint8_t STREAM_CHUNK = 64;        // bytes received at a time (RAM)

using Link = Uno::UnoSerial<BAUD>;
using StreamLink = stream::Puller<Link, STREAM_CHUNK>;
image::StreamImage<Screen, StreamLink> pictures(screen);   // the color format follows Screen

uint8_t result = pictures.draw<picture::sunset_stream_240x240_palette_565>(0, 0);
```

- **Name**: never call it `Stream`, which is Arduino's own class (`Arduino.h` already has it), so the examples all use `StreamLink`.
- **CHUNKs may differ**: one program may have several `Puller`s, e.g. 32 for notes and 255 for samples (`src/week_5_NEC/tests/2_sound_formats.cpp`); they share the same serial.

Complete examples: `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/3_pictures.cpp` (pictures), `tests[19]` of `tests/1_screen.cpp` (animation), `src/week_5_NEC/tests/2_sound_formats.cpp` (sounds).

## stream.py

```bash
python lib/stream/source/stream.py                                # finds the board by itself
python lib/stream/source/stream.py --port /dev/cu.usbmodem1101    # a given port: COM3 on Windows, /dev/ttyACM0 on Linux
python lib/stream/source/stream.py --baud 1000000                 # the same as the program's BAUD (default 1000000)
python lib/stream/source/stream.py --tile 8                       # animations: the grid size for finding changed areas (default 8)
```

- **Works on any system** (macOS, Windows, Linux): finds the board by the ID of its USB chip (Arduino, CH340, FTDI, CP210x, PL2303); without a board it waits, and reconnects by itself after unplugging, replugging or a board reset.
- **One program for everything**: at start it loads every `lib/*/source/stream_handler.py` and hands each command to the right one (`'I'` pictures, `'A'` sounds, `'O'` and others animations). A new kind only needs a `stream_handler.py`; `stream.py` stays as it is.
- **Safe**: each kind opens only `.h` files inside its own folder (`lib/image`, `lib/video`, `lib/audio`).
- **Changed files are read again** (by their modification time), no restart needed.
- **Close the monitor first**: PlatformIO's serial monitor also takes the port.
- **Lets go of the port while uploading**: another thread checks every 0.3 seconds whether avrdude or `pio run -t upload` is running; if so it closes the port at once and reconnects after the upload, so `stream.py` can stay running.
- **Opening the port resets the board**: opening a port always touches DTR, which resets the UNO; software cannot avoid it.
  - starting `stream.py` while the board runs a program starts the program over.
  - **start `stream.py` first**, then upload or press reset. With `stream.py` kept running, only the first time resets.
  - to avoid it completely in hardware, put a 10 µF capacitor between RESET and GND; but then every upload needs a manual press of reset.

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| `CHUNK` | 1 ~ 255 | compile error; each player has its own limit too (`video::StreamPlayer` ≥ 16, `audio::StreamPlayer` ≥ 4, `StreamSamplePlayer` ≤ BUFFER / 2) |
| path | at most 255 characters | cut |
| text of `message` | at most 255 characters, in RAM | cut |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| `stream.py` not running | `ERROR_NO_ANSWER` after 3 seconds | start `stream.py` first, then upload |
| `pio device monitor` open | `stream.py` cannot open the port | close the monitor while streaming; for messages use `message()` |
| Arduino's `Serial` at the same time | it uses D0 / D1 too | they cannot be used together |
| a type named `Stream` | clashes with Arduino's `Stream` class | give it another name, e.g. `StreamLink` |
| streaming two kinds at once (e.g. an animation playing while drawing a streamed picture) | the answers interleave and the board may read wrong | stream one kind at a time |
