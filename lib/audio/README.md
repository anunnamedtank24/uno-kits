# audio

English | [中文](讀我.md)

| Needs | Gives |
|---|---|
| playing notes, rhythms: a `Sound` (the modules of [buzzer](../buzzer/README.md)) | `audio::Player`: beeps, melodies, slides, never waiting while playing |
| playing samples: `SampleOut` (`Uno::UnoPcmOut<9>`) | `audio::SamplePlayer`: real waveforms (speech, music) |
| streaming: also a `Stream` (`stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`) | `audio::StreamPlayer`, `StreamSamplePlayer`: the sound stays on the computer and is sent while playing |
| sound files (wav, mp3, m4a…) | `melody::*`: sounds made by `MELODY("file")` or `source/convert.py` |

"Needs" are interfaces (duck typing); in parentheses the ready implementations of this project, and another board only replaces them. The details for developers (prerequisites, interface requirements, every function, parameters, resources) are in [INTERFACE.md](INTERFACE.md).

Sounds and players, `namespace audio` (players, formats) and `namespace melody` (the sounds made). `source/convert.py` turns a sound file (wav, flac, mp3, m4a, ogg, webm…) into one of three exchange formats, depending on **who plays it** (`--sound`) and **the quality** (`--quality`); it can be burned into Flash (`--target flash`) or stay on the computer and be streamed over the serial while playing (`--target stream`).

## Usage

```cpp
#include "Uno_tone.h"        // notes, rhythms (Timer1)
#include "Uno_pcm_out.h"     // samples (Timer1 + Timer2)
#include "Uno_serial.h"      // streaming
#include "stream.h"
#include "buzzer.h"
#include "audio.h"

constexpr uint8_t BUZZER_PIN = 9;                              // the passive buzzer's + (samples only on D9, D10)
constexpr uint32_t BAUD = 1000000;                             // the same as --baud of stream.py
constexpr uint8_t NOTE_CHUNK = 32;                             // notes: bytes pulled at a time
constexpr uint8_t PCM_CHUNK = 255;                             // samples: the bigger the faster (16 kHz needs 255)

using Speaker = buzzer::PassiveBuzzer2Pins<Uno::UnoTone<BUZZER_PIN>>;
using Link = Uno::UnoSerial<BAUD>;
using NoteLink = stream::Puller<Link, NOTE_CHUNK>;
using PcmLink = stream::Puller<Link, PCM_CHUNK>;
audio::Player<Speaker> speaker;
audio::StreamPlayer<Speaker, NoteLink> streamSpeaker;
using Samples = audio::SamplePlayer<Uno::UnoPcmOut<BUZZER_PIN>>;
using StreamSamples = audio::StreamSamplePlayer<Uno::UnoPcmOut<BUZZER_PIN>, PcmLink>;

void setup() {
    speaker.init();
    speaker.play<melody::ode_to_joy_flash_passive_medium>();             // notes from Flash
    // streamSpeaker.open<melody::twinkle_star_stream_passive_medium>();  // notes from the computer
    // Samples::play<melody::hello_flash_pcm_8000_adpcm>();              // samples from Flash
    // StreamSamples::open<melody::ode_to_joy_stream_pcm_16000_u8>();     // samples from the computer
}

void loop() {
    speaker.update();                                                  // call it every loop, it never waits
}
```

```bash
cd lib/audio
python source/convert.py assets/hello.aiff output --sound pcm --quality low --preview hello.wav
python source/convert.py assets/ode_to_joy.flac output --target stream --sound pcm --quality high
cd ../.. && python lib/stream/source/stream.py     # keep it running while streaming; it lets go of the port by itself while uploading (lib/stream)
```

Complete examples: `src/week_5_NEC/tests/2_sound_formats.cpp` (each format played once from Flash and once from the computer, the measurements sent to `stream.py` to print), `src/week_4_SSD1306_switch_buzzer_audio/tests/3_active_buzzer.cpp` (active), `src/week_4_SSD1306_switch_buzzer_audio/tests/4_passive_buzzer.cpp` (passive).

## Using the files directly (recommended)

A program writes `MELODY("path", "options")` and it is converted before compiling: no need to run `convert.py` first, and the file need not be in `assets/`:

```cpp
#include "audio.h"
using Chime = MELODY("sounds/chime.m4a", "--sound passive --quality high");
speaker.play<Chime>();
```

| Item | Explanation |
|---|---|
| Path | absolute, `~/…`, relative to the .cpp that writes it; if not found, the project folder, then `lib/audio/assets/` |
| Options | one string, the options of `source/convert.py`: `--target`, `--sound`, `--quality`, `--start`, `--duration`…; all left out, the converter's defaults (`--sound passive --quality medium`) |
| Where it goes | `lib/audio/output/`, the same as running `convert.py` by hand (`src/tools/assets.py` looks before every build: the same file with the same options already converted is used, only otherwise is it converted) |
| Type | `MELODY(...)` is that `melody::` struct, used like one converted by hand |
| Limits | both arguments must be string constants (no variables) |

The full explanation: [lib/README.md](../README.md#fonts-pictures-videos-sounds-use-the-files-directly); example: `src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp` (all four kinds). "Usage" above is the way of converting by hand, which works just as well.

## Hand-written melodies

Without converting a sound file, a melody can also be written directly:

```cpp
#include <avr/pgmspace.h>
using namespace audio;

const uint16_t MY_SONG[] PROGMEM = {
    noteHz(E, 5), beatMs(120), noteHz(G, 5), beatMs(120, 1, 2), REST, 100, noteHz(C, 6), 600,
};
speaker.play(MY_SONG, sizeof(MY_SONG) / sizeof(MY_SONG[0]) / 2);
```


## Wiring

| What it plays | Wiring |
|---|---|
| notes, rhythms | the buzzer's wiring, see [buzzer](../buzzer/README.md#wiring) |
| samples (`Uno::UnoPcmOut`) | **D9 or D10** → 100 Ω → the passive buzzer's + / a small speaker; - to GND. 3-pin passive modules (PNP): I/O on D9, with `Uno::UnoPcmOut<9, true>` |

- **How samples make sound**: Timer1 sends a 62.5 kHz PWM on D9 / D10 whose duty cycle is the sample value; 62.5 kHz is far beyond hearing, and the buzzer or speaker can only follow the average, so the average becomes the sound.
- **Small speakers (8 Ω)**: never straight on a pin (too much current): 100 ~ 220 Ω in series, or a transistor; another 10 µF capacitor in series blocks the DC (the middle value of the samples is half the voltage).
- **A buzzer playing samples**: magnetic ones are loudest at their 2 ~ 4 kHz resonance, the bass is very quiet and the tone sharp, but voices are clear enough.

## Formats and quality

| `--sound` | Who plays it | Format (`kind`) | `--quality low` | `medium` (default) | `high` |
|---|---|---|---|---|---|
| `passive` | a passive buzzer (`Player`, `StreamPlayer`) | notes: { pitch, ms } | `NOTES_COMPACT`: semitone number + units of 10 ms, **2 B a note**, exact semitones only | `NOTES_HZ_MS`: Hz + ms, 4 B a note, snapped to semitones | `NOTES_HZ_MS`, the frequencies measured (slides and vibrato become many small steps), shortest 20 ms |
| `active` | an active buzzer (the same) | rhythm: { on / off, ms }, `RHYTHM_MS`, 2 B a step | shortest sound 80 ms, rest 60 ms | 50 / 40 ms | 20 / 20 ms |
| `pcm` | a passive buzzer or small speaker, PWM (`SamplePlayer`, `StreamSamplePlayer`) | samples: the real waveform (voices, the tone of instruments) | 8000 Hz, IMA ADPCM (4 bits): 4 KB a second | 11025 Hz, 8 bits: 11 KB a second | 16000 Hz, 8 bits: 16 KB a second |

- **Single parameters override `--quality`**: notes have `--quantize`, `--min-note`, `--min-rest`; rhythms have `--hz` (the pitch when played on a passive one); samples have `--rate` (2000 ~ 32000), `--encoding u8 / u4 / adpcm`, `--gain`.
- **The three encodings of samples** (signal / error measured on the same voice at 8 kHz):

| `--encoding` | A sample | Signal / error | Explanation |
|---|---|---|---|
| `u8` | 8 bits | 35.5 dB | the clearest |
| `adpcm` | 4 bits | 17.5 dB | stores only "the difference from the one before": the same size as u4 but much clearer; decoding takes about 100 cycles |
| `u4` | 4 bits | 9.2 dB | very coarse, with clear noise |

- **Rhythms**: anything sounding counts, with a pitch or not (drums and claps come through too); two sounds in a row get a 30 ms stop between them automatically, or the active buzzer would join them into one.
- **The wrong player**: samples given to `Player`, notes given to `SamplePlayer`, `--target stream` given to a Flash player are all **compile errors**; notes given to an active buzzer (`Sound::HAS_PITCH` is false) are a **compile warning** (only the rhythm is heard).

## Flash or stream

| | `--target flash` | `--target stream` | `--target flash stream` |
|---|---|---|---|
| where the data is | `output/x.h`, burned into Flash | in the `#if 0` of `output/x.h`, staying on the computer; the board compiles only the length and path in front | `output/x.h` |
| played by | `Player`, `SamplePlayer` | `StreamPlayer`, `StreamSamplePlayer` + `stream.py` | both |
| length | limited by Flash: samples at 8 kHz ADPCM about 6 seconds, 16 kHz 8 bits about 1.5 seconds | unlimited | limited by Flash |

## Input limits

| Item | Range | Beyond it |
|---|---|---|
| `hz` | the range the Sound can make (`UnoTone`: 16 ~ 20000) | clamped (by the Sound); 0 = silent |
| `ms`, `onMs`, `offMs` | 0 ~ 65535 | — |
| `count` of `beeps` | 1 ~ 255 | 0: silent |
| steps of `Player` | 1 ~ 65535 (Flash is a limit too) | compile error (`play<Melody>`); `nullptr` or 0: silent |
| sample rate | 2000 ~ 32000 (`UnoPcmOut`: 1954 ~ 32000) | `convert.py` error; clamped on the board |
| the wrong player, `--target stream` given to a Flash player | — | compile error |
| notes given to an active buzzer | — | compile warning |
| `BUFFER`, `CHUNK` | see the parameters | compile error |
| the octave of `noteHz` | 0 ~ 8 | returns 0 (a rest) |
| interval of `update()` | the more often the better | notes: the edges come late, but the rhythm does not drift; streamed samples: underruns when the buffer runs out |
| the `notes` pointer | must point to an array in Flash (UNO: `PROGMEM`) | not checked: plays garbled notes |

## Caveats

| Situation | Result | What to do |
|---|---|---|
| chords, two voices | notes play only one at a time | pick the main melody when converting; for the original sound use `--sound pcm` |
| an active buzzer playing notes | only the rhythm is heard (compile warning) | convert to a rhythm with `--sound active` |
| very short notes (< 40 ms) | `GAP_MS` keeps them silent half the time | a smaller `GAP_MS`, or a bigger `--min-note` |
| samples together with `UnoTone` or PWM on D3 / D9 / D10 / D11 (LEDs) | they fight over Timer1 / Timer2 | one at a time; move the LEDs to D5 / D6 |
| the program doing something long while streaming samples (the OLED's `display()`, 64 ms) | the buffer runs out: an underrun, clicks | a bigger `BUFFER` (8 kHz 8 bits: 64 ms needs 512 B), or a lower sample rate, or ADPCM |
| `pio device monitor` or Arduino's `Serial` open at the same time | they fight over the same serial port | keep both closed while streaming; `stream.py` itself handles pictures, animations and sounds together |
| touching the pin directly while playing (`digitalWrite`, `analogWrite`) | the player does not know, its state no longer matches | control it only through the player |
