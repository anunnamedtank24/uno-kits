# audio/source

English | [中文](讀我.md)

Tools that run on the computer. Needs Python 3, `pip install numpy pyserial`, and `brew install ffmpeg` (decodes sound files; without it, macOS's `afconvert` reads most formats too). All run inside `lib/audio`.

## Files

| File | What it does |
|---|---|
| `convert.py` | sound file → `output/*.h` (the data of `--target stream` is in the `#if 0` of the same file), and adds the new one to `output/selected.h` |
| `check.py` | checks every file of `output/`: the current format, an unchanged source (`convert.py` runs it after every conversion) |
| `stream_handler.py` | used by `stream.py` of [lib/stream](../../stream/README.md) to read sounds and answer the board's `'A'` (`StreamPlayer`, `StreamSamplePlayer`); not run on its own |

## Usage

```bash
cd lib/audio
python source/convert.py assets/twinkle_star.wav output
python source/convert.py ~/Music/song.mp3 output --start 12.5 --duration 8 --preview song.wav
python source/convert.py assets/sos.webm output --sound active
python source/convert.py assets/hello.aiff output --sound pcm --quality low --preview hello.wav
python source/convert.py assets/ode_to_joy.flac output --target stream --sound pcm --quality high
python source/check.py
cd ../.. && python lib/stream/source/stream.py     # streaming: it serves pictures, animations and sounds
```

## convert.py

### Files supported

| Kind | Extensions | Decoded by |
|---|---|---|
| lossless | `.wav` `.flac` `.aiff` `.aif` `.caf` | ffmpeg; without ffmpeg, macOS's `afconvert`; `.wav` reads even without both (integer PCM) |
| lossy | `.mp3` `.aac` `.m4a` `.m4r` (ringtones) `.mp4` `.mov` | ffmpeg or `afconvert` |
| lossy | `.ogg` `.oga` `.opus` `.webm` | ffmpeg only |

- Stereo is mixed into mono; video files give only their sound.
- Measured: the same scale saved in each format above gives the same 15 notes.

### Steps

A buzzer makes only one note at a time, so only "the most noticeable pitch at each moment" is looked for:

1. **Decoding**: to mono at 22050 Hz.
2. **Pitch**: a cell every 10 ms, the fundamental found with YIN (de Cheveigné & Kawahara, 2002), from 43 Hz to 5 kHz; cells not clear enough count as having no pitch.
3. **Smoothing**: each cell takes the median of the 5 cells before and after, removing occasional octave jumps.
4. **Silence**: cells more than `--silence` dB quieter than the loudest are rests.
5. **Cutting notes**: a change of pitch (rounded to a semitone) is a new note; the same pitch with the volume jumping up by `--onset` dB within 40 ms is a new note too (two equal notes in a row).
6. **Tidying**:
   - notes shorter than `--min-note` merge into the longer neighbor.
   - rests shorter than `--min-rest` go to the note before.
   - rests at the start and end are removed.
7. **Pitch**: transposed by `--transpose`; notes outside `--range` move by octaves into the range.
8. **Time**: divided by `--speed`; each note's length is "the rounded end minus the rounded start", so the total never drifts from rounding.

- **Chords, songs with accompaniment**: what you get is the strongest line at each moment, not always the main melody. Pick a passage where the main melody is alone or loudest (`--start`, `--duration`).
- **Listening first**: `--preview x.wav` synthesizes the result as the passive buzzer's square wave (with the 20 ms gaps of `GAP_MS`), to listen to on the computer without uploading.

### Parameters

| Parameter | Default | Explanation |
|---|---|---|
| `--target flash / stream` | `flash` | `flash`: the data is burned into Flash; `stream`: the data stays on the computer (in the `#if 0` of `output/x.h`) and `stream.py` streams it; both: data in Flash, streamable too |
| `--sound passive / active / pcm` | `passive` | who plays it: a passive buzzer (notes), an active buzzer (rhythm), samples (the real waveform) |
| `--quality low / medium / high` | `medium` | a preset, see [the formats and quality of audio](../README.md#formats-and-quality); the single parameters below override it |
| `--name` | `{file name}_{target}_{sound}_{quality}`; samples: `{file name}_{target}_pcm_{rate}_{encoding}` (files outside `assets/` get their folders in front; equal names, e.g. differing only by `--transpose`, get `_2`, `_3`, never replacing another; see [output/README.md](../output/README.md#names-and-sources)) | the file and struct name |
| `--board FILE` | `custom_board` of `platformio.ini` (UNO: `lib/Uno/board.json`) | the board's numbers: flash size and how constants are kept there (UNO: `PROGMEM`, `avr/pgmspace.h`), clock, drawing cycles… Not counted in `// options:`, recorded in `// board:` |
| `--preview F.wav` | — | saves a wav to listen to: notes and rhythms as square waves; samples as the values the board really plays |
| `--start S`, `--duration S` | 0, to the end | which part (seconds) |
| `--speed F` | 1 | > 1 faster, < 1 slower, the pitch unchanged; 0.1 ~ 10 (samples need ffmpeg) |
| `--min-note MS`, `--min-rest MS` | by `--quality` | shorter notes merge into a neighbor, shorter rests go to the note before (notes, rhythms) |
| `--quantize semitone / raw` | by `--quality` | snapped to semitones, or the frequencies measured (notes; `--quality low` only semitone) |
| `--transpose N` | 0 | transposed by N semitones (notes) |
| `--range LOW HIGH` | 200 4000 | Hz; notes outside move by octaves into it, at least an octave wide (notes) |
| `--hz` | the median pitch measured, else 2000 | the pitch of the rhythm on a passive buzzer (rhythms) |
| `--rate HZ` | by `--quality` | the sample rate, within the board's `pcmOut.minRate` ~ `maxRate` (UNO: 2000 ~ 32000); converted at the frequency the board can really make (`pcmOut` of `board.json`: whole ticks) |
| `--encoding u8 / u4 / adpcm` | by `--quality` | 8 bits, 4 bits, IMA ADPCM 4 bits a sample |
| `--gain DB` | 0 | dB added after the largest sample is brought to full scale; > 0 is louder, but the peaks are clipped |
| `--silence DB` | 35 | this much quieter than the loudest is a rest (notes, rhythms) |
| `--voicing F` | 0.2 | the threshold of YIN, 0.05 ~ 0.5 (notes, rhythms) |
| `--onset DB` | 6 | how many dB the volume must jump up to count as a new note; 0 = not used |

- The limits of `--range` and the range of `--hz` are the frequency range of the board's buzzer (`tone` of `board.json`, UNO: 16 ~ 20000 Hz).
- Parameters that do not belong to this `--sound` (e.g. `--rate` with `--sound passive`) are an error.

### The steps of samples

1. **Decoding**: ffmpeg turns it straight into mono at the board's real sample rate (8000 → 8000 Hz, 11025 → 11050 Hz, 22050 → 21978 Hz); `--speed` uses ffmpeg's atempo (the pitch unchanged).
2. **Volume**: the largest sample is brought to 0.98, then `--gain` is added.
3. **Encoding**: u8, u4, or IMA ADPCM. The ADPCM encoder decodes back with the board's decoder as it encodes, so errors never add up; at the end the whole is decoded once more to make sure it matches the board.
4. **Report**: signal / error (dB), bytes a second, the share clipped.

### Errors and warnings

| Situation | Handling |
|---|---|
| the file does not exist, the extension is not supported, a parameter out of range | error, no file made |
| decoding fails, shorter than 46 ms, not a single note found | error, with a hint on which parameter to change |
| over 65535 steps, or over the board's flash (`flash.bytes`, UNO: 32256 bytes), with `--target flash` | error: cut it shorter, a lower `--quality`, or `--target stream` |
| notes or rhythms over 25% of the board's flash, samples over 62.5% (UNO: 8064, 20160 bytes), with `--target flash` | warning: little Flash left for the program |
| samples of `--target stream` over the board's `serial.maxStreamBytesPerSecond` a second (UNO: 12000 bytes) | warning: streaming may not keep up (measured: 16000 is at the edge; `underruns()` counts) |
| a sample rate above 22050 / below 4000 | warning: the interrupt takes much CPU / the sound is dull |
| over 1% of the samples clipped | warning: lower `--gain` |
| notes shorter than 40 ms | warning: `GAP_MS` 20 keeps them silent half the time |
| `--range` below 100 Hz or above 8 kHz | warning |
| the output folder is not `lib/audio/output` | warning: `audio.h` includes only `output/selected.h` |

## check.py

- `selected.h` exists and has its include guard.
- Every `#include` is a file of `output/`, listed once.
- Every file has the current format (`// format: 2`), `struct melody::{file name}` and `getPath()`; those for the UNO (`--target flash`) also need their data: `getNotes()` (notes, rhythms) or `getSamples()` (samples).
- The source sound (the recorded path and sha256) has not changed; a source not found is only a warning.
- The `// board:` of every file is the board of `custom_board` in `platformio.ini` (the same hash of the numbers); when it differs, `python src/clean.py --rebuild`.
- A .h in `output/` that is not selected is only a warning.

## stream_handler.py

`stream.py` of [lib/stream](../../stream/README.md) (finds the board, lets go of the port while uploading, prints the board's `'M'` messages) hands it each `'A'`:

- **The board says which file**: the path is the struct's `getPath()` (relative to `lib/audio`); only `.h` files inside `lib/audio` are opened; those of `--target flash` and `--target stream` (data in `#if 0`) can both be streamed.
- **Changed files are read again** (by their modification time), no restart needed.
- **Prints**: the format and size of each sound, and the time and speed when sent.
- The protocol: see [audio's streaming](../INTERFACE.md#串流).
