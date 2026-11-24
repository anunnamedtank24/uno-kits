# audio/output

English | [中文](讀我.md)

The sounds made by `source/convert.py` and by macros in programs, `namespace melody`. Never edit the files here by hand: to change one, convert it again.

## Files

```
output/
├── selected.h    written automatically: lists every .h here, audio.h includes it
├── registry.h    written automatically: the types of MELODY("path", "options") in the current program
└── *.h           the converted sounds: made by convert.py by hand or by MELODY(...), all here, in the same format
```

## selected.h, registry.h

- Both are written by `src/tools/assets.py` before every build (`convert.py` also adds a new one to `selected.h`); nothing to edit by hand.
- `selected.h` lists **every** one: the unused ones take no Flash (the data is in arrays inside `inline` functions, with the board's flash attribute (`board.json`; UNO: `PROGMEM`); never called, never generated; measured: the same Flash).
- `registry.h` has only the `MELODY(...)` of the current program.
- `python source/check.py` checks that every file has the current format and an unchanged source; `convert.py` checks once after every conversion.

## The current sounds

| File | Source | Format | Content | Data |
|---|---|---|---|---|
| `twinkle_star_flash_passive_medium.h` | twinkle_star.wav | notes Hz + ms | the first line of Twinkle Twinkle Little Star (French folk song, public domain), 14 notes, C5 ~ A5, 6.4 s | 56 B |
| `twinkle_star_flash_passive_low.h` | twinkle_star.wav | notes compact | the same, half the size | 28 B |
| `twinkle_star_flash_active_medium.h` | twinkle_star.wav | rhythm | the rhythm of the same (active buzzer), 30 ms between the sounds | 54 B |
| `twinkle_star_stream_passive_medium.h` | twinkle_star.wav | notes Hz + ms, computer | for streaming (the data in `#if 0`) | 56 B |
| `ode_to_joy_flash_passive_medium.h` | ode_to_joy.flac | notes Hz + ms | the first line of Ode to Joy (Beethoven, public domain), 14 notes, 6.9 s | 56 B |
| `ode_to_joy_stream_pcm_16000_u8.h` | ode_to_joy.flac | samples 16 kHz 8 bits, computer | the real waveform of the same, 7.5 s | 119 KB (in `#if 0`) |
| `scale_flash_passive_medium.h` | scale.aiff | notes Hz + ms | the C major scale C5 → C6 → C5, for checking the tuning | 60 B |
| `scale_flash_passive_high.h` | scale.aiff | notes, the frequencies measured | the same, frequencies not snapped to semitones (e.g. 1048 Hz) | 60 B |
| `startup_flash_passive_medium.h` | startup.mp3 | notes | start-up: C E G C upward, 0.5 s | 16 B |
| `success_flash_passive_medium.h` | success.m4a | notes | success: a fourth upward, 0.3 s | 8 B |
| `error_flash_passive_medium.h` | error.ogg | notes | error: two low notes downward, 0.6 s | 8 B |
| `sos_flash_active_medium.h` | sos.webm | rhythm | Morse code SOS (... --- ...), 3.3 s | 34 B |
| `hello_flash_pcm_8000_adpcm.h` | hello.aiff | samples 8 kHz ADPCM | the voice of macOS `say`: "Hello! I am an Arduino.", 2.5 s | 10 KB |
| `mine_4_assets_audio_pokemon_8_2_la_disc_4_16_7b5dc7_flash_passive_medium.h` | `~/Mine/4_assets/audio/Pokemon/8-2 LA/Disc 4/16 戦い：ウォロ.aiff` (`MELODY(...)`, `src/week_4_SSD1306_switch_buzzer_audio/tests/7_play_file.cpp`) | notes Hz + ms | the melody found in 279 seconds of orchestra: 283 notes, 220 ~ 1760 Hz | 2 KB |

The sound files of `assets/`: the first seven were synthesized with numpy (fundamental plus overtones, attack and decay, a little noise, stereo), then saved by ffmpeg in seven formats; hello.aiff is `say -o assets/hello.aiff "Hello! I am an Arduino."`.

## The generated struct

```cpp
namespace melody {
    struct ode_to_joy_flash_passive_medium {
        static constexpr uint8_t target;       // audio::TARGET_FLASH (1), TARGET_STREAM (2), or both
        static constexpr uint8_t kind;         // audio::KIND_NOTES, KIND_RHYTHM, KIND_PCM
        static constexpr uint8_t encoding;     // audio::NOTES_HZ_MS … PCM_ADPCM (lib/audio/format.h)
        static constexpr uint32_t count;       // steps (notes, rhythms) or samples
        static constexpr uint32_t durationMs;
        static constexpr uint16_t rate;        // the sample rate (samples only)
        static constexpr uint16_t rhythmHz;    // the pitch of a rhythm on a passive buzzer
        static constexpr uint32_t dataSize;
        static inline const char *getPath();          // the path where stream.py finds the data, relative to lib/audio
        static inline const uint16_t *getNotes();     // notes, rhythms (--target flash)
        static inline const uint8_t *getSamples();    // samples (--target flash)
        static inline const uint16_t *getSteps();     // PCM_ADPCM only: the 89 steps of IMA ADPCM (the decoder's table, with --target stream too)
    };
}
```

**The start of a file**: `// options: …` (the arguments when converted; `src/tools/assets.py` finds converted ones by it and `source:`), `// format: 2` (the format version), `// board: … sha256 …` (the board it was converted for: `custom_board` of `platformio.ini`; when the hash of the numbers differs, `check.py` errors and `src/tools/assets.py` converts it again before compiling), `// source: … sha256 …` (the source sound and its hash), `// settings:` (every setting and result: target, sound, quality, encoding, the cut, detection parameters, sample rate, signal / error, data size).

## Format (format 2)

The bytes of each encoding are at the top of [lib/audio/format.h](../format.h):

| Encoding | A step / sample | Content |
|---|---|---|
| `NOTES_HZ_MS` (0) | 2 uint16 | Hz (0 = rest), ms; the same format as hand-written melodies of `note.h` |
| `NOTES_COMPACT` (1) | 1 uint16 | high 8 bits a MIDI note (0 = rest, 12 ~ 119), low 8 bits a count of 10 ms (1 ~ 255) |
| `RHYTHM_MS` (2) | 1 uint16 | bit 15 = on, bits 0 ~ 14 = ms (1 ~ 32767) |
| `PCM_U8` (3) | 1 byte | 0 ~ 255, 128 = silence |
| `PCM_U4` (4) | half a byte | the low 4 bits first; 0 ~ 15 → 0 ~ 255 (× 17) |
| `PCM_ADPCM` (5) | half a byte | IMA ADPCM, the low 4 bits first, starting at 0, index 0 |

- **A step too long** is split into several steps (the same note again): Hz + ms at most 65535 ms a step, compact 2550 ms, rhythm 32767 ms.
- **format 1** (only `{hz, ms}`, named `*_semitone`) is no longer used: `check.py` asks for converting again.

## Names and sources

The start of every .h says which file and which options it was converted from:

```
// options: --fit 120x80 --palette 16
// source: assets/sunset.jpg sha256 1413…
```

`MELODY("path", "options")` in a program finds a converted file by these two lines: the same file with the same options (the order and the form `--fit=…` do not matter) is used as it is, only otherwise is it converted. So converted by hand or by a macro, each is converted only once.

| Rule | Explanation |
|---|---|
| Names | the rule of `convert.py`: `{file name}_{settings}`, the struct named the same (`melody::name`) |
| Files outside `assets/` | the folders go in front of the file name (relative to the project; outside the project, to the home folder), e.g. `~/Music/Game/title.mp3` → `music_game_title_…`, so files with the same name in different folders never mix |
| Characters that are not letters or digits (e.g. Japanese) | left out, with a 6-digit hash added instead, e.g. `16 戦い：ウォロ.aiff` → `…_16_7b5dc7_…` |
| Equal names (the name does not hold every option, or another source) | `_2`, `_3`… added, never replacing another file |
| The source changed | a warning when compiling; `python src/tools/assets.py --force` converts it again (replacing the one with the same name) |
| The source moved or deleted | it still compiles as long as the output is there (the old path in the program still finds it) |
| Unwanted ones | `python src/tools/assets.py --unused` lists those no program (`.bak` included) uses; delete them yourself |
