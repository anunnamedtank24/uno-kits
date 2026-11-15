import sys
sys.dont_write_bytecode = True # importing check.py must not leave __pycache__

import argparse
import math
import re
import shutil
import subprocess
import tempfile
import wave
from dataclasses import dataclass
from pathlib import Path
import numpy as np

import check


"""
Func: convert an audio file to .h (c header): one melody, a flash array of pairs { hz, ms } (hz 0 = rest)
      for audio::Player (with an active buzzer: its rhythm; with a passive buzzer: its notes)
Usage:
    python source/convert.py assets/twinkle_star.wav output
    python source/convert.py assets/song.mp3 output --start 12.5 --duration 8 --range 400 3000 --preview song.wav
Output (struct melody::{name}, name = {file}_{semitone|raw} or --name):
    output/{name}.h, added to output/selected.h, then selected.h is checked (source/check.py)
    every setting is written at the top of the file, whatever the name says
Files:
    .wav .flac .aiff .aif .mp3 .aac .m4a .m4r .mp4 .mov .caf .ogg .oga .opus .webm (anything ffmpeg reads)
    decoded by ffmpeg (brew install ffmpeg); without it macOS afconvert (no ogg / opus / webm),
    without both only .wav; stereo is mixed to mono
How:
    one pitch at a time (a buzzer plays one note): the strongest pitch of every 10 ms (YIN),
    quiet parts are rests, a sudden rise in loudness starts a new note (repeated notes),
    then notes too short are merged into their neighbors
    a chord or a song with drums and voice gives the loudest line, not always the tune:
    cut a part (--start / --duration) where the melody is alone or on top
Pitch:
    --quantize semitone     the nearest note of the scale (default): sounds in tune
    --quantize raw          the measured hz (median of the note): slides, vibrato kept as steps
    --transpose N           semitones up (+) / down (-)
    --range LOW HIGH        notes outside are moved by octaves into it (default 200 4000 Hz:
                            small buzzers are weak below, piercing above)
Time:
    --start S --duration S  only this part of the file, seconds
    --speed F               faster (> 1) / slower (< 1), pitch unchanged
    --min-note MS           shorter notes are merged into a neighbor (default 60)
    --min-rest MS           shorter rests are given to the note before (default 40)
Detection:
    --silence DB            frames this many dB under the loudest are rests (default 35)
    --voicing F             YIN threshold, 0.05 ~ 0.5: lower = only clean pitches (default 0.2)
    --onset DB              a rise of this many dB within 40 ms starts a new note, 0 = off (default 6)
Check:
    --preview FILE.wav      the result as the square wave a passive buzzer plays, to listen on the Mac
"""


SUFFIXES = (".wav", ".flac", ".aiff", ".aif", ".mp3", ".aac", ".m4a", ".m4r", ".mp4", ".mov", ".caf",
            ".ogg", ".oga", ".opus", ".webm")
AFCONVERT_SUFFIXES = (".wav", ".flac", ".aiff", ".aif", ".mp3", ".aac", ".m4a", ".m4r", ".mp4", ".mov", ".caf")

SAMPLE_RATE = 22050 # analysed at this rate: enough for pitches up to ~5 kHz
FRAME = 1024 # samples per YIN frame (46 ms): pitches down to 2 * SAMPLE_RATE / FRAME = 43 Hz
HOP = 220 # samples between frames (~10 ms)
MIN_PITCH_HZ = 50.0
MAX_PITCH_HZ = 5000.0
MEDIAN_FRAMES = 5 # pitch smoothing: odd
ONSET_FRAMES = 4 # --onset: the rise is measured over this many frames

# the board: Uno::UnoTone plays 16 ~ 20000 Hz, audio::Player counts notes in uint16_t, ms is uint16_t
MIN_HZ = 16
MAX_HZ = 20000
MAX_NOTES = 65535
MAX_MS = 65535
FLASH_BYTES = 32256 # UNO flash for the program
RECOMMENDED_MAX_BYTES = 8192 # more: little room left for the program
NOTE_NAMES = ("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B")


@dataclass
class Settings:
    # everything one conversion needs, after the arguments are resolved
    start: float
    duration: float | None
    quantize: str # "semitone" or "raw"
    transpose: int
    low_hz: float
    high_hz: float
    speed: float
    min_note_ms: float
    min_rest_ms: float
    silence_db: float
    voicing: float
    onset_db: float


@dataclass
class Segment:
    # frames [start, end) of one note (midi set) or rest (midi None)
    start: int
    end: int
    midi: float | None
    onset: bool # starts with a rise in loudness: never merged into the note before


def indent(str_: str, tab_count: int = 1) -> str:
    added = " "*4*tab_count
    return "\n".join(added + line if line else line for line in str_.split("\n"))


def get_midi(hz: float) -> float:
    return 69 + 12*math.log2(hz/440)


def get_hz(midi: float) -> float:
    return 440*2**((midi - 69)/12)


def get_note_name(hz: int) -> str:
    midi = round(get_midi(hz))
    return f"{NOTE_NAMES[midi % 12]}{midi//12 - 1}"


# ---------- decoding ----------

def read_wav(path: Path) -> np.ndarray:
    # 16 / 24 / 32-bit integer pcm wav without ffmpeg; mono, original rate, resampled after
    with wave.open(str(path), "rb") as wav_file:
        channels = wav_file.getnchannels()
        width = wav_file.getsampwidth()
        rate = wav_file.getframerate()
        raw = wav_file.readframes(wav_file.getnframes())
    if width == 3:
        bytes_ = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
        values = (bytes_[:, 0].astype(np.int32) | (bytes_[:, 1].astype(np.int32) << 8) | (bytes_[:, 2].astype(np.int32) << 16))
        values = np.where(values >= 1 << 23, values - (1 << 24), values).astype(np.float32)/(1 << 23)
    elif width in (2, 4):
        dtype = np.int16 if width == 2 else np.int32
        values = np.frombuffer(raw, dtype=dtype).astype(np.float32)/np.iinfo(dtype).max
    elif width == 1:
        values = (np.frombuffer(raw, dtype=np.uint8).astype(np.float32) - 128)/128
    else:
        raise ValueError(f"{path.name}: {8*width}-bit wav is not supported without ffmpeg")
    samples = values.reshape(-1, channels).mean(axis=1)
    if rate != SAMPLE_RATE:
        times = np.arange(0, len(samples)/rate, 1/SAMPLE_RATE)
        samples = np.interp(times, np.arange(len(samples))/rate, samples).astype(np.float32)
    return samples


def decode(path: Path, start: float, duration: float | None) -> np.ndarray:
    # mono float32 samples at SAMPLE_RATE, only [start, start + duration)
    if shutil.which("ffmpeg"):
        command = ["ffmpeg", "-v", "error", "-ss", str(start)]
        if duration is not None:
            command += ["-t", str(duration)]
        command += ["-i", str(path), "-vn", "-ac", "1", "-ar", str(SAMPLE_RATE), "-f", "f32le", "-"]
        result = subprocess.run(command, capture_output=True)
        if result.returncode != 0:
            raise ValueError(f"ffmpeg cannot read {path.name}: {result.stderr.decode(errors='replace').strip()}")
        samples = np.frombuffer(result.stdout, dtype=np.float32)
    else:
        if shutil.which("afconvert") and path.suffix.lower() in AFCONVERT_SUFFIXES:
            with tempfile.TemporaryDirectory() as temp_dir:
                wav_path = Path(temp_dir)/"audio.wav"
                result = subprocess.run(["afconvert", "-f", "WAVE", "-d", f"LEI16@{SAMPLE_RATE}", "-c", "1",
                                         str(path), str(wav_path)], capture_output=True)
                if result.returncode != 0:
                    raise ValueError(f"afconvert cannot read {path.name}: {result.stderr.decode(errors='replace').strip()}")
                samples = read_wav(wav_path)
        elif path.suffix.lower() == ".wav":
            samples = read_wav(path)
        else:
            raise ValueError(f"{path.suffix} needs ffmpeg (brew install ffmpeg)")
        first = int(start*SAMPLE_RATE)
        last = None if duration is None else first + int(duration*SAMPLE_RATE)
        samples = samples[first:last]
    if len(samples) < FRAME:
        raise ValueError(f"{path.name}: less than {1000*FRAME/SAMPLE_RATE:.0f} ms of audio (check --start / --duration)")
    return samples


# ---------- pitch (YIN, de Cheveigné & Kawahara 2002) ----------

def get_frames(samples: np.ndarray) -> np.ndarray:
    count = 1 + (len(samples) - FRAME)//HOP
    indexes = np.arange(FRAME)[None, :] + HOP*np.arange(count)[:, None]
    return samples[indexes].astype(np.float64)


def get_loudness_db(frames: np.ndarray) -> np.ndarray:
    rms = np.sqrt(np.mean(frames**2, axis=1))
    return 20*np.log10(np.maximum(rms, 1e-9))


def get_pitches(frames: np.ndarray, voicing: float) -> np.ndarray:
    # hz per frame, nan where no clear pitch
    window = FRAME//2
    min_lag = int(SAMPLE_RATE/MAX_PITCH_HZ)
    max_lag = min(window - 1, int(SAMPLE_RATE/MIN_PITCH_HZ) + 1)
    pitches = np.full(len(frames), np.nan)
    for start in range(0, len(frames), 512):
        block = frames[start:start + 512]
        # difference d(lag) = sum (x[j] - x[j + lag])^2, j < window, from correlations (FFT)
        spectrum = np.fft.rfft(block, 2*FRAME)
        head_spectrum = np.fft.rfft(block[:, :window], 2*FRAME)
        correlation = np.fft.irfft(spectrum*np.conj(head_spectrum), 2*FRAME)[:, :window]
        squares = np.concatenate([np.zeros((len(block), 1)), np.cumsum(block**2, axis=1)], axis=1)
        lags = np.arange(window)
        energy = squares[:, lags + window] - squares[:, lags]
        difference = np.maximum(squares[:, window][:, None] + energy - 2*correlation, 0)
        # cumulative mean normalized difference
        running = np.cumsum(difference[:, 1:], axis=1)
        normalized = np.ones_like(difference)
        normalized[:, 1:] = difference[:, 1:]*lags[1:]/np.maximum(running, 1e-12)
        for row in range(len(block)):
            values = normalized[row]
            below = np.nonzero(values[min_lag:max_lag] < voicing)[0]
            if len(below) == 0:
                continue
            lag = min_lag + below[0]
            while lag + 1 < max_lag and values[lag + 1] < values[lag]:
                lag += 1
            # parabola through the three points around the dip
            if 1 <= lag < window - 1:
                left, middle, right = values[lag - 1], values[lag], values[lag + 1]
                curve = left - 2*middle + right
                shift = 0.5*(left - right)/curve if curve > 0 else 0.0
            else:
                shift = 0.0
            pitches[start + row] = SAMPLE_RATE/(lag + shift)
    return pitches


def get_smoothed_midis(pitches: np.ndarray) -> np.ndarray:
    # median of the voiced frames around each voiced frame: removes single wrong frames (octave jumps)
    midis = np.where(np.isnan(pitches), np.nan, 69 + 12*np.log2(np.where(np.isnan(pitches), 440, pitches)/440))
    half = MEDIAN_FRAMES//2
    smoothed = midis.copy()
    for index in np.nonzero(~np.isnan(midis))[0]:
        window = midis[max(0, index - half):index + half + 1]
        smoothed[index] = np.median(window[~np.isnan(window)])
    return smoothed


# ---------- notes ----------

def get_segments(midis: np.ndarray, loudness: np.ndarray, settings: Settings) -> list[Segment]:
    loud = loudness >= loudness.max() - settings.silence_db
    voiced = loud & ~np.isnan(midis)
    labels = [round(midi) if voiced[index] else None for index, midi in enumerate(midis)]
    onsets = np.zeros(len(midis), dtype=bool)
    if settings.onset_db > 0:
        for index in range(ONSET_FRAMES, len(midis)):
            rise = loudness[index] - loudness[index - ONSET_FRAMES:index].min()
            # a frame of a rise is an onset only once: the first frame of it
            onsets[index] = voiced[index] and rise >= settings.onset_db and not onsets[index - 1]

    segments = []
    for index, label in enumerate(labels):
        if segments and segments[-1].midi == label and not onsets[index]:
            segments[-1].end = index + 1
        else:
            segments.append(Segment(index, index + 1, label, bool(onsets[index])))
    # raw: the measured pitch of the note (median), the rounded one only split the notes
    if settings.quantize == "raw":
        for segment in segments:
            if segment.midi is not None:
                segment.midi = float(np.median(midis[segment.start:segment.end]))
    return segments


def get_frame_ms(frames: int) -> float:
    return 1000*frames*HOP/SAMPLE_RATE


def get_cleaned(segments: list[Segment], settings: Settings) -> list[Segment]:
    # 1. notes shorter than --min-note: the shortest first, into the longer neighbor note (or a rest)
    segments = [Segment(s.start, s.end, s.midi, s.onset) for s in segments]
    while True:
        shorts = [i for i, s in enumerate(segments) if s.midi is not None and get_frame_ms(s.end - s.start) < settings.min_note_ms]
        if not shorts:
            break
        index = min(shorts, key=lambda i: segments[i].end - segments[i].start)
        neighbors = [i for i in (index - 1, index + 1) if 0 <= i < len(segments)]
        if neighbors:
            target = max(neighbors, key=lambda i: (segments[i].midi is not None, segments[i].end - segments[i].start))
            segments[index].midi = segments[target].midi
            # the two must join: the later one loses its onset (the joined note starts with the earlier)
            if target == index - 1:
                segments[index].onset = False
            else:
                segments[target].onset = False
        else:
            segments[index].midi = None
        segments = get_joined(segments)

    # 2. rests shorter than --min-rest: the note before holds on
    for index in range(1, len(segments)):
        segment = segments[index]
        if segment.midi is None and segments[index - 1].midi is not None and get_frame_ms(segment.end - segment.start) < settings.min_rest_ms:
            segment.midi = segments[index - 1].midi
            segment.onset = False
    segments = get_joined(segments)

    # 3. no rest at the start or the end
    while segments and segments[0].midi is None:
        segments.pop(0)
    while segments and segments[-1].midi is None:
        segments.pop()
    return segments


def get_joined(segments: list[Segment]) -> list[Segment]:
    # neighbors with the same pitch become one, unless the second starts with an onset
    joined = []
    for segment in segments:
        if joined and joined[-1].midi == segment.midi and not (segment.onset and segment.midi is not None):
            joined[-1].end = segment.end
        else:
            joined.append(segment)
    return joined


def get_note_hz(midi: float, settings: Settings) -> tuple[int, bool]:
    # (hz, moved by octaves)
    hz = get_hz(midi + settings.transpose)
    moved = False
    while hz < settings.low_hz and hz*2 <= settings.high_hz:
        hz *= 2
        moved = True
    while hz > settings.high_hz and hz/2 >= settings.low_hz:
        hz /= 2
        moved = True
    return min(MAX_HZ, max(MIN_HZ, round(hz))), moved


def get_notes(segments: list[Segment], settings: Settings) -> tuple[list[tuple[int, int]], int]:
    # [(hz, ms)], hz 0 = rest; the ms are rounded at the ends, so the total does not drift
    notes = []
    moved_count = 0
    first = segments[0].start if segments else 0
    for segment in segments:
        start_ms = round(get_frame_ms(segment.start - first)/settings.speed)
        end_ms = round(get_frame_ms(segment.end - first)/settings.speed)
        ms = end_ms - start_ms
        if ms <= 0:
            continue
        if segment.midi is None:
            hz = 0
        else:
            hz, moved = get_note_hz(segment.midi, settings)
            moved_count += moved
        # longer than uint16_t: split (a rest in between would be heard, so the same note again)
        while ms > MAX_MS:
            notes.append((hz, MAX_MS))
            ms -= MAX_MS
        notes.append((hz, ms))
    return notes, moved_count


# ---------- preview ----------

def write_preview(path: Path, notes: list[tuple[int, int]], gap_ms: int = 20) -> None:
    # the square wave a passive buzzer gets from audio::Player (with its GAP_MS), quiet enough to listen to
    rate = 44100
    parts = []
    for hz, ms in notes:
        count = round(rate*ms/1000)
        part = np.zeros(count, dtype=np.float32)
        sound = count - round(rate*gap_ms/1000) if ms >= 2*gap_ms else count
        if hz > 0 and sound > 0:
            times = np.arange(sound)/rate
            part[:sound] = np.where((times*hz) % 1 < 0.5, 0.25, -0.25)
        parts.append(part)
    samples = np.concatenate(parts) if parts else np.zeros(1, dtype=np.float32)
    with wave.open(str(path), "wb") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(rate)
        wav_file.writeframes((samples*32767).astype(np.int16).tobytes())


# ---------- c code ----------

def get_settings_str(settings: Settings, audio_info: str, notes: list[tuple[int, int]], moved_count: int) -> str:
    # every setting, whatever the file name says
    sounds = [hz for hz, _ in notes if hz > 0]
    total_ms = sum(ms for _, ms in notes)
    cut = f"start {settings.start:g} s, " + ("to the end" if settings.duration is None else f"duration {settings.duration:g} s")
    pitch_range = f"{min(sounds)} ~ {max(sounds)} Hz ({get_note_name(min(sounds))} ~ {get_note_name(max(sounds))})" if sounds else "none"
    lines = [
        f"audio       {audio_info}",
        f"cut         {cut}",
        f"detection   YIN every {1000*HOP/SAMPLE_RATE:.0f} ms, voicing {settings.voicing:g}, "
        f"silence -{settings.silence_db:g} dB, onset {settings.onset_db:g} dB",
        f"pitch       {settings.quantize}, transpose {settings.transpose:+d}, range {settings.low_hz:g} ~ {settings.high_hz:g} Hz "
        f"({moved_count} notes moved by octaves)",
        f"time        speed {settings.speed:g}, min note {settings.min_note_ms:g} ms, min rest {settings.min_rest_ms:g} ms",
        f"result      {len(sounds)} notes + {len(notes) - len(sounds)} rests, {total_ms/1000:.2f} s, {pitch_range}",
        f"data        {4*len(notes)} bytes",
    ]
    return "// settings:\n" + "".join(f"//   {line}\n" for line in lines)


def get_c_str(name: str, command: str, audio_path: Path, settings_str: str, notes: list[tuple[int, int]]) -> str:
    lines = []
    for start_index in range(0, len(notes), 8):
        lines.append(" ".join(f"{hz}, {ms}," for hz, ms in notes[start_index:start_index + 8]))
    total_ms = sum(ms for _, ms in notes)

    struct_str = ""
    struct_str += f"static constexpr uint16_t count = {len(notes)};\n"
    struct_str += f"static constexpr uint32_t durationMs = {total_ms};\n"
    struct_str += "\n"
    struct_str += "// pairs { hz, ms }, hz 0 = rest\n"
    struct_str += "static inline const uint16_t *getNotes() {\n"
    struct_str += "    static const uint16_t notes[] PROGMEM = {\n"
    struct_str += indent("\n".join(lines), 2) + "\n"
    struct_str += "    };\n"
    struct_str += "    return notes;\n"
    struct_str += "}\n"

    c_str = ""
    c_str += f"#ifndef MELODY_{name.upper()}_H\n"
    c_str += f"#define MELODY_{name.upper()}_H\n\n"
    c_str += "// generated by convert.py, do not edit\n"
    c_str += f"// {command}\n"
    c_str += check.get_format_str()
    c_str += check.get_source_str(audio_path)
    c_str += settings_str + "\n"
    c_str += "#include <stdint.h>\n"
    c_str += "#if defined(__AVR__)\n"
    c_str += "#include <avr/pgmspace.h>\n"
    c_str += "#endif\n"
    c_str += "#ifndef PROGMEM\n"
    c_str += "#define PROGMEM\n"
    c_str += "#endif\n\n"
    c_str += f"namespace {check.NAMESPACE} {{\n"
    c_str += f"    // player.play<melody::{name}>()\n"
    c_str += f"    struct {name} {{\n"
    c_str += indent(struct_str.rstrip("\n"), 2) + "\n"
    c_str += "    };\n"
    c_str += "}\n\n#endif\n"
    return c_str


def get_name(audio_path: Path, settings: Settings) -> str:
    file_name = "".join(c if c.isalnum() else "_" for c in audio_path.stem.lower())
    if file_name[0].isdigit():
        file_name = "_" + file_name
    return f"{file_name}_{settings.quantize}"


# ---------- arguments ----------

def get_positive_float(value: str) -> float:
    try:
        number = float(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be a number, got {value!r}")
    if not number > 0:
        raise argparse.ArgumentTypeError(f"must be > 0, got {number}")
    return number


def get_non_negative_float(value: str) -> float:
    try:
        number = float(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be a number, got {value!r}")
    if not number >= 0:
        raise argparse.ArgumentTypeError(f"must be >= 0, got {number}")
    return number


def get_identifier(value: str) -> str:
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", value):
        raise argparse.ArgumentTypeError(f"must be a C++ name (letters, digits, _), got {value!r}")
    return value


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert an audio file to a melody (c header) for audio::Player: one note at a time.",
        epilog=(
            "examples:\n"
            "  python source/convert.py assets/twinkle_star.wav output\n"
            "  python source/convert.py assets/song.mp3 output --start 12.5 --duration 8 --preview song.wav\n"
            "  python source/check.py    (checks output/selected.h)"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("audio_path", help="audio: " + " ".join(SUFFIXES))
    parser.add_argument("output_dir_path", help="dir for the generated .h (made if missing)")
    parser.add_argument("--name", type=get_identifier, help="file / struct name instead of the default (every setting is in the file anyway)")
    parser.add_argument("--preview", help="also write the result as a square wave .wav, to listen on the computer")

    time_group = parser.add_argument_group("time")
    time_group.add_argument("--start", type=get_non_negative_float, default=0.0, help="seconds into the file (default 0)")
    time_group.add_argument("--duration", type=get_positive_float, help="seconds from --start (default: to the end)")
    time_group.add_argument("--speed", type=get_positive_float, default=1.0, help="> 1 faster, < 1 slower (default 1)")
    time_group.add_argument("--min-note", type=get_non_negative_float, default=60.0, help="ms: shorter notes merge into a neighbor (default 60)")
    time_group.add_argument("--min-rest", type=get_non_negative_float, default=40.0, help="ms: shorter rests go to the note before (default 40)")

    pitch_group = parser.add_argument_group("pitch")
    pitch_group.add_argument("--quantize", choices=("semitone", "raw"), default="semitone", help="semitone: in tune (default); raw: as measured")
    pitch_group.add_argument("--transpose", type=int, default=0, help="semitones, + up / - down (default 0)")
    pitch_group.add_argument("--range", type=get_positive_float, nargs=2, default=(200.0, 4000.0), metavar=("LOW", "HIGH"),
                             help="Hz: notes outside move by octaves into it (default 200 4000)")

    detection_group = parser.add_argument_group("detection")
    detection_group.add_argument("--silence", type=get_positive_float, default=35.0, help="dB under the loudest: rest (default 35)")
    detection_group.add_argument("--voicing", type=get_positive_float, default=0.2, help="YIN threshold 0.05 ~ 0.5 (default 0.2)")
    detection_group.add_argument("--onset", type=get_non_negative_float, default=6.0, help="dB rise that starts a new note, 0 = off (default 6)")
    return parser


def check_args(parser: argparse.ArgumentParser, args: argparse.Namespace) -> list[str]:
    # errors stop here, warnings are returned
    warnings = []
    audio_path = Path(args.audio_path).expanduser()
    if not audio_path.is_file():
        parser.error(f"audio_path not found: {audio_path}")
    if audio_path.suffix.lower() not in SUFFIXES:
        parser.error(f"audio_path must be {' / '.join(SUFFIXES)}, got {audio_path.name}")
    low_hz, high_hz = args.range
    if not MIN_HZ <= low_hz < high_hz <= MAX_HZ:
        parser.error(f"--range must be {MIN_HZ} <= LOW < HIGH <= {MAX_HZ}, got {low_hz:g} {high_hz:g}")
    if high_hz < 2*low_hz:
        parser.error(f"--range must span an octave at least (HIGH >= 2 x LOW), got {low_hz:g} {high_hz:g}")
    if not 0.05 <= args.voicing <= 0.5:
        parser.error(f"--voicing must be 0.05 ~ 0.5, got {args.voicing:g}")
    if not 0.1 <= args.speed <= 10:
        parser.error(f"--speed must be 0.1 ~ 10, got {args.speed:g}")
    if low_hz < 100 or high_hz > 8000:
        warnings.append(f"--range {low_hz:g} {high_hz:g}: most buzzers are weak under 100 Hz and shrill over 8 kHz")
    if args.preview is not None and Path(args.preview).suffix.lower() != ".wav":
        parser.error(f"--preview must be a .wav, got {args.preview}")
    return warnings


def get_settings(args: argparse.Namespace) -> Settings:
    return Settings(
        start=args.start, duration=args.duration, quantize=args.quantize, transpose=args.transpose,
        low_hz=args.range[0], high_hz=args.range[1], speed=args.speed,
        min_note_ms=args.min_note, min_rest_ms=args.min_rest,
        silence_db=args.silence, voicing=args.voicing, onset_db=args.onset,
    )


def convert_audio_to_h(args: argparse.Namespace, command: str, warnings: list[str]) -> bool:
    # returns True if selected.h passes the check
    audio_path = Path(args.audio_path).expanduser().resolve()
    output_dir_path = Path(args.output_dir_path).expanduser().resolve()
    if output_dir_path.exists() and not output_dir_path.is_dir():
        raise NotADirectoryError(f"Not a dir: {output_dir_path}")
    output_dir_path.mkdir(parents=True, exist_ok=True)
    settings = get_settings(args)

    samples = decode(audio_path, settings.start, settings.duration)
    audio_info = f"{len(samples)/SAMPLE_RATE:.2f} s used, {audio_path.suffix.lower()[1:]}, analysed as mono {SAMPLE_RATE} Hz"
    frames = get_frames(samples)
    loudness = get_loudness_db(frames)
    midis = get_smoothed_midis(get_pitches(frames, settings.voicing))
    segments = get_cleaned(get_segments(midis, loudness, settings), settings)
    if not segments:
        raise ValueError("no note found: too quiet or no clear pitch (try --silence 50, --voicing 0.3)")
    notes, moved_count = get_notes(segments, settings)

    data_bytes = 4*len(notes)
    if len(notes) > MAX_NOTES:
        raise ValueError(f"{len(notes)} notes: at most {MAX_NOTES} (cut it with --start / --duration)")
    if data_bytes > FLASH_BYTES:
        raise ValueError(f"{data_bytes} bytes: more than the UNO's flash ({FLASH_BYTES}), cut it with --start / --duration")
    if data_bytes > RECOMMENDED_MAX_BYTES:
        warnings.append(f"{data_bytes} bytes: above {RECOMMENDED_MAX_BYTES}, little flash left for the program")
    short_count = sum(1 for hz, ms in notes if hz > 0 and ms < 40)
    if short_count:
        warnings.append(f"{short_count} notes are shorter than 40 ms (Player's GAP_MS 20 makes them silent half the time)")

    name = args.name or get_name(audio_path, settings)
    output_file_path = output_dir_path/(name + ".h")
    settings_str = get_settings_str(settings, audio_info, notes, moved_count)
    print(f"[Info] {name}:")
    print("\n".join(" "*4 + line[5:] for line in settings_str.splitlines()[1:]))
    for warning in warnings:
        print(f"[Warning] {warning}")
    if output_file_path.exists():
        print(f"[Info] Replaced the old {output_file_path.name}")
    output_file_path.write_text(get_c_str(name, command, audio_path, settings_str, notes))
    print(f"[Success] Saved as: {output_file_path}")
    if args.preview is not None:
        preview_path = Path(args.preview).expanduser()
        write_preview(preview_path, notes)
        print(f"[Success] Preview: {preview_path}")

    if check.add_selected(output_dir_path, output_file_path):
        print(f"[Success] Added to {check.SELECTED_NAME}")
    if output_dir_path != (check.LIB_DIR_PATH/"output").resolve():
        print(f"[Warning] audio.h includes {check.LIB_DIR_PATH.name}/output/{check.SELECTED_NAME}, not this dir")
    return check.print_check(output_dir_path)


def main():
    parser = get_parser()
    args = parser.parse_args()
    warnings = check_args(parser, args)
    command = " ".join([Path(args.audio_path).name] + sys.argv[2:])
    try:
        is_fine = convert_audio_to_h(args, command, warnings)
    except (ValueError, OSError) as e:
        # same form as the argument errors: usage + message, no traceback
        parser.error(str(e))
    if not is_fine:
        sys.exit(1)


if __name__ == "__main__":
    main()
