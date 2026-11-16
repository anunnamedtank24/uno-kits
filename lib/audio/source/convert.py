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
Func: convert an audio file to .h (c header) in one of the exchange formats of lib/audio/format.h,
      chosen by what plays it (--sound) and how well (--quality), for the board's flash (--target uno)
      and / or for stream.py (--target mac)
Usage:
    python source/convert.py assets/twinkle_star.wav output
    python source/convert.py assets/sos.webm output --sound active
    python source/convert.py assets/hello.aiff output --sound pcm --quality low
    python source/convert.py assets/song.mp3 output --target mac --sound pcm --quality high --start 12.5 --duration 30
Output (struct melody::{name}, name = {file}_{targets}_{sound}_{quality} or {file}_{targets}_pcm_{rate}_{encoding}, or --name):
    --target uno / uno mac: output/{name}.h with the data (Player / SamplePlayer play it, the stream players can ask for it)
    --target mac:           output/{name}.h without data (for the board) + output/mac/{name}.h with it (stream.py reads it)
    added to output/selected.h, then selected.h is checked (source/check.py); every setting is written at the top
Sound (what plays it) and quality:
    --sound passive  notes { pitch, ms } for a passive buzzer (audio::Player / StreamPlayer)
        --quality low     NOTES_COMPACT: MIDI note + 10 ms ticks, 2 bytes a note, in tune only
        --quality medium  NOTES_HZ_MS: hz + ms, 4 bytes a note, in tune (default)
        --quality high    NOTES_HZ_MS, pitch as measured (slides and vibrato as steps), notes down to 20 ms
    --sound active   a rhythm { on / off, ms } for an active buzzer (one pitch): RHYTHM_MS, 2 bytes a step;
                     anything loud is "on" (drums and noise too); --quality: the shortest notes / rests kept
        low 80 / 60 ms, medium 50 / 40 ms (default), high 20 / 20 ms
    --sound pcm      samples (the real wave: voice, any instrument) for audio::SamplePlayer / StreamSamplePlayer
        --quality low     8000 Hz, IMA ADPCM (4 bits): 4 KB a second   (flash: ~6 s)
        --quality medium  11025 Hz, 8 bits: 11 KB a second             (flash: ~2 s; the mac: no limit)
        --quality high    16000 Hz, 8 bits: 16 KB a second
        --rate HZ (2000 ~ 32000), --encoding u8 / u4 / adpcm, --gain DB override the preset
Files:
    .wav .flac .aiff .aif .mp3 .aac .m4a .m4r .mp4 .mov .caf .ogg .oga .opus .webm (anything ffmpeg reads)
    decoded by ffmpeg (brew install ffmpeg); without it macOS afconvert (no ogg / opus / webm),
    without both only .wav; stereo is mixed to mono
How (notes and rhythm):
    one pitch at a time (a buzzer plays one note): the strongest pitch of every 10 ms (YIN),
    quiet parts are rests, a sudden rise in loudness starts a new note (repeated notes),
    then notes too short are merged into their neighbors
    a chord or a song with drums and voice gives the loudest line, not always the tune:
    cut a part (--start / --duration) where the melody is alone or on top
How (pcm):
    resampled by ffmpeg to the rate the board really makes (Uno::UnoPcmOut: 2 MHz or 500 kHz / whole ticks),
    the loudest sample brought to full scale (+ --gain), then 8 / 4 bits or IMA ADPCM
Pitch (passive):
    --quantize semitone / raw, --transpose N, --range LOW HIGH (notes outside move by octaves)
Time:
    --start S --duration S, --speed F (pitch unchanged), --min-note MS, --min-rest MS
Detection (passive, active):
    --silence DB, --voicing F, --onset DB
Check:
    --preview FILE.wav      what the board plays (square waves, or the decoded samples), to listen on the Mac
"""


SUFFIXES = (".wav", ".flac", ".aiff", ".aif", ".mp3", ".aac", ".m4a", ".m4r", ".mp4", ".mov", ".caf",
            ".ogg", ".oga", ".opus", ".webm")
AFCONVERT_SUFFIXES = (".wav", ".flac", ".aiff", ".aif", ".mp3", ".aac", ".m4a", ".m4r", ".mp4", ".mov", ".caf")

SAMPLE_RATE = 22050 # notes and rhythm are analysed at this rate: enough for pitches up to ~5 kHz
FRAME = 1024 # samples per YIN frame (46 ms): pitches down to 2 * SAMPLE_RATE / FRAME = 43 Hz
HOP = 220 # samples between frames (~10 ms)
MIN_PITCH_HZ = 50.0
MAX_PITCH_HZ = 5000.0
MEDIAN_FRAMES = 5 # pitch smoothing: odd
ONSET_FRAMES = 4 # --onset: the rise is measured over this many frames

# the board: Uno::UnoTone plays 16 ~ 20000 Hz, Player counts steps in uint16_t
MIN_HZ = 16
MAX_HZ = 20000
MAX_PLAYER_STEPS = 65535
FLASH_BYTES = 32256 # UNO flash for the program
NOTE_NAMES = ("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B")

# Uno::UnoPcmOut: Timer2 at 2 MHz (n <= 256) or 500 kHz
PCM_MIN_RATE = 2000
PCM_MAX_RATE = 32000
PCM_RECOMMENDED_MAX_RATE = 22050 # above: the interrupt takes a lot of the CPU
STREAM_RECOMMENDED_BYTES_PER_S = 12000 # above: the link may not keep up (measured: 16000 is at the edge, see README)

TARGET_BITS = {"uno": 1, "mac": 2} # audio::TARGET_UNO / TARGET_MAC
MAC_DIR_NAME = "mac" # output/mac/: data for stream.py only
KINDS = {"passive": 0, "active": 1, "pcm": 2} # audio::KIND_NOTES / KIND_RHYTHM / KIND_PCM
KIND_NAMES = {0: "KIND_NOTES", 1: "KIND_RHYTHM", 2: "KIND_PCM"}
ENCODINGS = {"hz_ms": 0, "compact": 1, "rhythm": 2, "u8": 3, "u4": 4, "adpcm": 5} # format.h
ENCODING_NAMES = {0: "NOTES_HZ_MS", 1: "NOTES_COMPACT", 2: "RHYTHM_MS", 3: "PCM_U8", 4: "PCM_U4", 5: "PCM_ADPCM"}
COMPACT_TICK_MS = 10
RHYTHM_MAX_MS = 32767
RHYTHM_GAP_MS = 30 # active: between two notes that touch, so they are heard as two (one pitch only)
RECOMMENDED_MAX_BYTES = {"passive": 8192, "active": 8192, "pcm": 20000} # --target uno: more = little flash left

# --quality presets, overridden by the single options
NOTE_PRESETS = {
    # passive: encoding, quantize, min note, min rest
    "passive": {
        "low": {"encoding": "compact", "quantize": "semitone", "min_note": 60.0, "min_rest": 40.0},
        "medium": {"encoding": "hz_ms", "quantize": "semitone", "min_note": 60.0, "min_rest": 40.0},
        "high": {"encoding": "hz_ms", "quantize": "raw", "min_note": 20.0, "min_rest": 20.0},
    },
    "active": {
        "low": {"encoding": "rhythm", "quantize": "semitone", "min_note": 80.0, "min_rest": 60.0},
        "medium": {"encoding": "rhythm", "quantize": "semitone", "min_note": 50.0, "min_rest": 40.0},
        "high": {"encoding": "rhythm", "quantize": "semitone", "min_note": 20.0, "min_rest": 20.0},
    },
}
PCM_PRESETS = {
    "low": {"rate": 8000, "encoding": "adpcm"},
    "medium": {"rate": 11025, "encoding": "u8"},
    "high": {"rate": 16000, "encoding": "u8"},
}

ADPCM_STEPS = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
)
ADPCM_INDEX_CHANGES = (-1, -1, -1, -1, 2, 4, 6, 8)


@dataclass
class Settings:
    # everything one conversion needs, after the arguments and the preset are resolved
    targets: list[str] # "uno", "mac" or both
    sound: str # "passive", "active", "pcm"
    quality: str # "low", "medium", "high"
    encoding: str # a key of ENCODINGS
    start: float
    duration: float | None
    speed: float
    # notes / rhythm
    quantize: str # "semitone" or "raw"
    transpose: int
    low_hz: float
    high_hz: float
    min_note_ms: float
    min_rest_ms: float
    silence_db: float
    voicing: float
    onset_db: float
    rhythm_hz: int | None # None: from the notes
    # pcm
    rate: int
    gain_db: float


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

def read_wav(path: Path, rate_out: int) -> np.ndarray:
    # 8 / 16 / 24 / 32-bit integer pcm wav without ffmpeg; mono, resampled to rate_out
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
    if rate != rate_out:
        times = np.arange(0, len(samples)/rate, 1/rate_out)
        samples = np.interp(times, np.arange(len(samples))/rate, samples).astype(np.float32)
    return samples


def decode(path: Path, start: float, duration: float | None, rate: int = SAMPLE_RATE, tempo: float = 1.0) -> np.ndarray:
    # mono float32 samples at rate, only [start, start + duration); tempo != 1: faster / slower, same pitch (ffmpeg)
    if shutil.which("ffmpeg"):
        command = ["ffmpeg", "-v", "error", "-ss", str(start)]
        if duration is not None:
            command += ["-t", str(duration)]
        command += ["-i", str(path), "-vn", "-ac", "1"]
        if tempo != 1.0:
            command += ["-af", get_atempo_str(tempo)]
        command += ["-ar", str(rate), "-f", "f32le", "-"]
        result = subprocess.run(command, capture_output=True)
        if result.returncode != 0:
            raise ValueError(f"ffmpeg cannot read {path.name}: {result.stderr.decode(errors='replace').strip()}")
        samples = np.frombuffer(result.stdout, dtype=np.float32)
    else:
        if shutil.which("afconvert") and path.suffix.lower() in AFCONVERT_SUFFIXES:
            with tempfile.TemporaryDirectory() as temp_dir:
                wav_path = Path(temp_dir)/"audio.wav"
                result = subprocess.run(["afconvert", "-f", "WAVE", "-d", f"LEI16@{rate}", "-c", "1",
                                         str(path), str(wav_path)], capture_output=True)
                if result.returncode != 0:
                    raise ValueError(f"afconvert cannot read {path.name}: {result.stderr.decode(errors='replace').strip()}")
                samples = read_wav(wav_path, rate)
        elif path.suffix.lower() == ".wav":
            samples = read_wav(path, rate)
        else:
            raise ValueError(f"{path.suffix} needs ffmpeg (brew install ffmpeg)")
        if tempo != 1.0:
            raise ValueError("--speed with --sound pcm needs ffmpeg (brew install ffmpeg)")
        first = int(start*rate)
        last = None if duration is None else first + int(duration*rate)
        samples = samples[first:last]
    if len(samples) < FRAME:
        raise ValueError(f"{path.name}: less than {1000*FRAME/rate:.0f} ms of audio (check --start / --duration)")
    return samples


def get_atempo_str(tempo: float) -> str:
    # ffmpeg atempo takes 0.5 ~ 2 per filter: chained for more
    parts = []
    while tempo > 2.0:
        parts.append(2.0)
        tempo /= 2.0
    while tempo < 0.5:
        parts.append(0.5)
        tempo /= 0.5
    parts.append(tempo)
    return ",".join(f"atempo={part:.6f}" for part in parts)


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
    if settings.sound == "active":
        # a rhythm: loud is on, pitch or not (drums, clicks and noise count too)
        voiced = loud
        labels = [1 if voiced[index] else None for index in range(len(midis))]
    else:
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
    if settings.sound == "passive" and settings.quantize == "raw":
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
        elif settings.sound == "active":
            hz = 1 # on
        else:
            hz, moved = get_note_hz(segment.midi, settings)
            moved_count += moved
        # too long for an encoding: split there (get_steps), the same note again
        notes.append((hz, ms))
    return notes, moved_count


# ---------- encodings ----------

def get_steps(notes: list[tuple[int, int]], encoding: str) -> list[int]:
    # (hz, ms) -> uint16 words of format.h; ms are rounded at the ends, so the total does not drift
    words = []
    if encoding == "hz_ms":
        for hz, ms in notes:
            while ms > 65535:
                words += [hz, 65535]
                ms -= 65535
            words += [hz, ms]
        return words
    if encoding == "rhythm":
        # notes that touch: the first one ends RHYTHM_GAP_MS early
        spaced = []
        for index, (hz, ms) in enumerate(notes):
            touches = hz and index + 1 < len(notes) and notes[index + 1][0] and ms > 2*RHYTHM_GAP_MS
            spaced += [(hz, ms - RHYTHM_GAP_MS), (0, RHYTHM_GAP_MS)] if touches else [(hz, ms)]
        for hz, ms in spaced:
            on = 0x8000 if hz else 0
            while ms > RHYTHM_MAX_MS:
                words.append(on | RHYTHM_MAX_MS)
                ms -= RHYTHM_MAX_MS
            words.append(on | ms)
        return words
    # compact: MIDI note << 8 | 10 ms ticks (1 ~ 255)
    end_ms = 0
    end_ticks = 0
    for hz, ms in notes:
        end_ms += ms
        ticks = round(end_ms/COMPACT_TICK_MS) - end_ticks
        if ticks <= 0:
            continue
        end_ticks += ticks
        midi = 0 if hz == 0 else round(get_midi(hz))
        if hz and not 12 <= midi <= 119:
            raise ValueError(f"{hz} Hz is outside what --quality low stores (C0 ~ B8): use --range or medium")
        while ticks > 255:
            words.append((midi << 8) | 255)
            ticks -= 255
        words.append((midi << 8) | ticks)
    return words


def get_step_notes(words: list[int], encoding: str, rhythm_hz: int) -> list[tuple[int, int]]:
    # words -> (hz, ms) as the board decodes them (format.h decodeStep)
    notes = []
    if encoding == "hz_ms":
        return [(words[i], words[i + 1]) for i in range(0, len(words), 2)]
    for word in words:
        if encoding == "rhythm":
            notes.append((rhythm_hz if word & 0x8000 else 0, word & 0x7FFF))
        else:
            midi = word >> 8
            hz = 0 if not 12 <= midi <= 119 else get_board_note_hz(midi)
            notes.append((hz, (word & 0xFF)*COMPACT_TICK_MS))
    return notes


def get_board_note_hz(midi: int) -> int:
    # audio::midiHz: equal temperament from octave 8 in 0.01 Hz, rounded like note.h
    octave_8 = (418601, 443492, 469864, 497803, 527404, 558765, 591991, 627196, 664488, 704000, 745862, 790213)
    octave = midi//12 - 1
    shift = 8 - octave
    return (octave_8[midi % 12] + (50 << shift))//(100 << shift)


def get_real_rate(rate: int) -> float:
    # what Uno::UnoPcmOut makes of rate: whole ticks of Timer2 at 2 MHz (n <= 256) or 500 kHz
    ticks = (2000000 + rate//2)//rate
    clock = 2000000
    if ticks > 256:
        ticks = (500000 + rate//2)//rate
        clock = 500000
    return clock/ticks


def get_pcm_values(samples: np.ndarray, encoding: str) -> tuple[list[int], list[int]]:
    # float -1 ~ 1 -> (bytes of the data, the 8-bit samples the board plays)
    if encoding == "u8":
        values = np.clip(np.round(samples*127.0) + 128, 0, 255).astype(int).tolist()
        return values, values
    if encoding == "u4":
        codes = np.clip(np.round((samples + 1.0)*7.5), 0, 15).astype(int).tolist()
        return get_packed(codes), [code*17 for code in codes]
    # adpcm
    pcm16 = np.clip(np.round(samples*32767.0), -32768, 32767).astype(int).tolist()
    codes = []
    played = []
    predictor = 0
    index = 0
    for sample in pcm16:
        step = ADPCM_STEPS[index]
        difference = sample - predictor
        code = 0
        if difference < 0:
            code = 8
            difference = -difference
        if difference >= step:
            code |= 4
            difference -= step
        if difference >= step >> 1:
            code |= 2
            difference -= step >> 1
        if difference >= step >> 2:
            code |= 1
        predictor, index, value = get_adpcm_decoded(predictor, index, code)
        codes.append(code)
        played.append(value)
    return get_packed(codes), played


def get_adpcm_decoded(predictor: int, index: int, code: int) -> tuple[int, int, int]:
    # one code as audio::Adpcm::decode: (predictor, index, 8-bit sample)
    step = ADPCM_STEPS[index]
    difference = step >> 3
    if code & 4:
        difference += step
    if code & 2:
        difference += step >> 1
    if code & 1:
        difference += step >> 2
    predictor = predictor - difference if code & 8 else predictor + difference
    predictor = max(-32768, min(32767, predictor))
    index = max(0, min(88, index + ADPCM_INDEX_CHANGES[code & 7]))
    return predictor, index, (predictor >> 8) + 128


def get_packed(codes: list[int]) -> list[int]:
    # two 4-bit codes a byte, the low nibble first (an odd count ends with 0)
    if len(codes) % 2:
        codes = codes + [0]
    return [codes[i] | (codes[i + 1] << 4) for i in range(0, len(codes), 2)]


def get_unpacked_played(data: list[int], count: int, encoding: str) -> list[int]:
    # the 8-bit samples the board plays from data (a check of the encoder, like the board decodes)
    if encoding == "u8":
        return data[:count]
    codes = []
    for byte in data:
        codes += [byte & 0x0F, byte >> 4]
    codes = codes[:count]
    if encoding == "u4":
        return [code*17 for code in codes]
    played = []
    predictor = 0
    index = 0
    for code in codes:
        predictor, index, value = get_adpcm_decoded(predictor, index, code)
        played.append(value)
    return played


# ---------- preview ----------

def write_wav(path: Path, samples: np.ndarray, rate: int) -> None:
    with wave.open(str(path), "wb") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(rate)
        wav_file.writeframes((np.clip(samples, -1, 1)*32767).astype(np.int16).tobytes())


def write_notes_preview(path: Path, notes: list[tuple[int, int]], gap: bool, gap_ms: int = 20) -> None:
    # the square wave a passive buzzer gets from audio::Player (with its GAP_MS), quiet enough to listen to
    rate = 44100
    parts = []
    for hz, ms in notes:
        count = round(rate*ms/1000)
        part = np.zeros(count, dtype=np.float32)
        sound = count - round(rate*gap_ms/1000) if gap and ms >= 2*gap_ms else count
        if hz > 0 and sound > 0:
            times = np.arange(sound)/rate
            part[:sound] = np.where((times*hz) % 1 < 0.5, 0.25, -0.25)
        parts.append(part)
    write_wav(path, np.concatenate(parts) if parts else np.zeros(1, dtype=np.float32), rate)


def write_pcm_preview(path: Path, played: list[int], real_rate: float) -> None:
    write_wav(path, (np.array(played, dtype=np.float32) - 128)/128*0.5, round(real_rate))


# ---------- c code ----------

def get_settings_str(settings: Settings, audio_info: str, lines_extra: list[str], data_bytes: int) -> str:
    # every setting, whatever the file name says
    cut = f"start {settings.start:g} s, " + ("to the end" if settings.duration is None else f"duration {settings.duration:g} s")
    lines = [f"target      {' '.join(settings.targets)}",
             f"sound       {settings.sound}, quality {settings.quality}, encoding {ENCODING_NAMES[ENCODINGS[settings.encoding]]}",
             f"audio       {audio_info}",
             f"cut         {cut}, speed {settings.speed:g}"]
    lines += lines_extra
    lines.append(f"data        {data_bytes} bytes")
    return "// settings:\n" + "".join(f"//   {line}\n" for line in lines)


def get_notes_lines(settings: Settings, notes: list[tuple[int, int]], moved_count: int, rhythm_hz: int) -> list[str]:
    sounds = [hz for hz, _ in notes if hz > 0]
    total_ms = sum(ms for _, ms in notes)
    lines = [f"detection   YIN every {1000*HOP/SAMPLE_RATE:.0f} ms, voicing {settings.voicing:g}, "
             f"silence -{settings.silence_db:g} dB, onset {settings.onset_db:g} dB",
             f"time        min note {settings.min_note_ms:g} ms, min rest {settings.min_rest_ms:g} ms"]
    if settings.sound == "passive":
        pitch_range = f"{min(sounds)} ~ {max(sounds)} Hz ({get_note_name(min(sounds))} ~ {get_note_name(max(sounds))})" if sounds else "none"
        lines.insert(1, f"pitch       {settings.quantize}, transpose {settings.transpose:+d}, range {settings.low_hz:g} ~ {settings.high_hz:g} Hz "
                        f"({moved_count} notes moved by octaves)")
        lines.append(f"result      {len(sounds)} notes + {len(notes) - len(sounds)} rests, {total_ms/1000:.2f} s, {pitch_range}")
    else:
        lines.append(f"result      {len(sounds)} on + {len(notes) - len(sounds)} off, {total_ms/1000:.2f} s, "
                     f"{rhythm_hz} Hz on a passive buzzer")
    return lines


def get_c_table_str(c_type: str, name: str, values: list[int], values_per_line: int) -> str:
    lines = [", ".join(str(value) for value in values[i:i + values_per_line]) + "," for i in range(0, len(values), values_per_line)]
    if not lines:
        lines = ["0, // empty"]
    return f"static const {c_type} {name}[] PROGMEM = {{\n" + indent("\n".join(lines)) + "\n};\n"


def get_c_getter_str(comment: str, c_type: str, function: str, table_str: str, name: str) -> str:
    getter_str = f"// {comment}\n"
    getter_str += f"static inline const {c_type} *{function}() {{\n"
    getter_str += indent(table_str.rstrip("\n")) + "\n"
    getter_str += f"    return {name};\n"
    getter_str += "}\n"
    return getter_str


def get_c_info_str(settings: Settings, count: int, duration_ms: int, rate: int, rhythm_hz: int, data_bytes: int, path: str) -> str:
    # what every output/{name}.h has, with or without the data
    kind = KINDS[settings.sound]
    encoding = ENCODINGS[settings.encoding]
    info = ""
    info += f"static constexpr uint8_t target = {sum(TARGET_BITS[t] for t in settings.targets)}; // "
    info += " | ".join(f"audio::TARGET_{t.upper()}" for t in settings.targets) + "\n"
    info += f"static constexpr uint8_t kind = {kind}; // audio::{KIND_NAMES[kind]}\n"
    info += f"static constexpr uint8_t encoding = {encoding}; // audio::{ENCODING_NAMES[encoding]}\n"
    info += f"static constexpr uint32_t count = {count}; // " + ("samples\n" if settings.sound == "pcm" else "steps\n")
    info += f"static constexpr uint32_t durationMs = {duration_ms};\n"
    info += f"static constexpr uint16_t rate = {rate}; // samples a second (PCM only)\n"
    info += f"static constexpr uint16_t rhythmHz = {rhythm_hz}; // the pitch of a rhythm on a passive buzzer\n"
    info += f"static constexpr uint32_t dataSize = {data_bytes};\n"
    info += "\n"
    info += get_c_getter_str("where stream.py finds the data, relative to lib/audio (the stream players send it)",
                             "char", "getPath", f'static const char path[] PROGMEM = "{path}";\n', "path")
    return info


def get_c_data_getter_str(settings: Settings, data: list[int]) -> str:
    if settings.sound == "pcm":
        return get_c_getter_str(f"samples, {ENCODING_NAMES[ENCODINGS[settings.encoding]]} (lib/audio/format.h)", "uint8_t", "getSamples",
                                get_c_table_str("uint8_t", "samples", data, 24), "samples")
    return get_c_getter_str(f"steps, {ENCODING_NAMES[ENCODINGS[settings.encoding]]} (lib/audio/format.h)", "uint16_t", "getNotes",
                            get_c_table_str("uint16_t", "notes", data, 16), "notes")


def get_usage_str(name: str, settings: Settings, with_data: bool) -> str:
    flash_player = "audio::SamplePlayer::play" if settings.sound == "pcm" else "player.play"
    stream_player = "audio::StreamSamplePlayer::open" if settings.sound == "pcm" else "player.open"
    usages = []
    if "uno" in settings.targets:
        usages.append(f"{flash_player}<melody::{name}>()")
    if "mac" in settings.targets:
        usages.append(f"{stream_player}<melody::{name}>(), with stream.py running")
    if settings.targets == ["mac"] and with_data:
        usages = [f"data for stream.py only, never included: {stream_player}<melody::{name}>()"]
    return "\n".join(usages)


def get_c_str(name: str, command: str, audio_path: Path, settings_str: str, info_str: str, usage: str, data_str: str) -> str:
    struct_str = info_str + ("\n" + data_str if data_str else "")
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
    c_str += indent("".join(f"// {line}\n" for line in usage.split("\n")) + f"struct {name} {{\n" + indent(struct_str.rstrip("\n")) + "\n};") + "\n"
    c_str += "}\n\n#endif\n"
    return c_str


def get_name(audio_path: Path, settings: Settings) -> str:
    file_name = "".join(c if c.isalnum() else "_" for c in audio_path.stem.lower())
    if file_name[0].isdigit():
        file_name = "_" + file_name
    targets = "_".join(settings.targets)
    if settings.sound == "pcm":
        return f"{file_name}_{targets}_pcm_{settings.rate}_{settings.encoding}"
    return f"{file_name}_{targets}_{settings.sound}_{settings.quality}"


def get_lib_relative_str(path: Path) -> str:
    # path as stream.py looks for it: relative to lib/audio
    try:
        return path.resolve().relative_to(check.LIB_DIR_PATH).as_posix()
    except ValueError:
        return path.resolve().as_posix()


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


def get_positive_int(value: str) -> int:
    try:
        number = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be an integer, got {value!r}")
    if number <= 0:
        raise argparse.ArgumentTypeError(f"must be > 0, got {number}")
    return number


def get_identifier(value: str) -> str:
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", value):
        raise argparse.ArgumentTypeError(f"must be a C++ name (letters, digits, _), got {value!r}")
    return value


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert an audio file to a sound (c header) of lib/audio: notes, a rhythm or samples.",
        epilog=(
            "examples:\n"
            "  python source/convert.py assets/twinkle_star.wav output\n"
            "  python source/convert.py assets/sos.webm output --sound active\n"
            "  python source/convert.py assets/hello.aiff output --sound pcm --quality low\n"
            "  python source/convert.py assets/song.mp3 output --target mac --sound pcm --quality high --preview song.wav\n"
            "  python source/check.py    (checks output/selected.h)"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("audio_path", help="audio: " + " ".join(SUFFIXES))
    parser.add_argument("output_dir_path", help="dir for the generated .h (made if missing)")
    parser.add_argument("--target", choices=("uno", "mac"), nargs="+", default=["uno"],
                        help="uno: data in flash; mac: data stays here, stream.py sends it; both: data in flash, streamable too (default uno)")
    parser.add_argument("--sound", choices=("passive", "active", "pcm"), default="passive",
                        help="passive: notes; active: a rhythm (one pitch); pcm: samples (default passive)")
    parser.add_argument("--quality", choices=("low", "medium", "high"), default="medium", help="a preset, see the top of this file (default medium)")
    parser.add_argument("--name", type=get_identifier, help="file / struct name instead of the default (every setting is in the file anyway)")
    parser.add_argument("--preview", help="also write what the board plays as a .wav, to listen on the computer")

    time_group = parser.add_argument_group("time")
    time_group.add_argument("--start", type=get_non_negative_float, default=0.0, help="seconds into the file (default 0)")
    time_group.add_argument("--duration", type=get_positive_float, help="seconds from --start (default: to the end)")
    time_group.add_argument("--speed", type=get_positive_float, default=1.0, help="> 1 faster, < 1 slower, same pitch (default 1)")
    time_group.add_argument("--min-note", type=get_non_negative_float, help="ms: shorter notes merge into a neighbor (default: --quality)")
    time_group.add_argument("--min-rest", type=get_non_negative_float, help="ms: shorter rests go to the note before (default: --quality)")

    pitch_group = parser.add_argument_group("pitch (passive)")
    pitch_group.add_argument("--quantize", choices=("semitone", "raw"), help="semitone: in tune; raw: as measured (default: --quality)")
    pitch_group.add_argument("--transpose", type=int, default=0, help="semitones, + up / - down (default 0)")
    pitch_group.add_argument("--range", type=get_positive_float, nargs=2, default=(200.0, 4000.0), metavar=("LOW", "HIGH"),
                             help="Hz: notes outside move by octaves into it (default 200 4000)")

    rhythm_group = parser.add_argument_group("rhythm (active)")
    rhythm_group.add_argument("--hz", type=get_positive_int, help="the pitch of the rhythm on a passive buzzer (default: the median note, or 2000)")

    pcm_group = parser.add_argument_group("samples (pcm)")
    pcm_group.add_argument("--rate", type=get_positive_int, help=f"samples a second, {PCM_MIN_RATE} ~ {PCM_MAX_RATE} (default: --quality)")
    pcm_group.add_argument("--encoding", choices=("u8", "u4", "adpcm"), help="u8: 8 bits; u4: 4 bits; adpcm: 4 bits, sounds better (default: --quality)")
    pcm_group.add_argument("--gain", type=float, default=0.0, help="dB after the loudest sample is brought to full scale; > 0 clips the peaks, louder (default 0)")

    detection_group = parser.add_argument_group("detection (passive, active)")
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
    args.target = [target for target in ("uno", "mac") if target in args.target]
    low_hz, high_hz = args.range
    if not MIN_HZ <= low_hz < high_hz <= MAX_HZ:
        parser.error(f"--range must be {MIN_HZ} <= LOW < HIGH <= {MAX_HZ}, got {low_hz:g} {high_hz:g}")
    if high_hz < 2*low_hz:
        parser.error(f"--range must span an octave at least (HIGH >= 2 x LOW), got {low_hz:g} {high_hz:g}")
    if not 0.05 <= args.voicing <= 0.5:
        parser.error(f"--voicing must be 0.05 ~ 0.5, got {args.voicing:g}")
    if not 0.1 <= args.speed <= 10:
        parser.error(f"--speed must be 0.1 ~ 10, got {args.speed:g}")
    if args.preview is not None and Path(args.preview).suffix.lower() != ".wav":
        parser.error(f"--preview must be a .wav, got {args.preview}")

    pitch_given = args.quantize is not None or args.transpose != 0 or tuple(args.range) != (200.0, 4000.0)
    pcm_given = args.rate is not None or args.encoding is not None or args.gain != 0.0
    notes_given = args.min_note is not None or args.min_rest is not None or args.hz is not None
    if args.sound == "pcm":
        if pitch_given or notes_given:
            parser.error("--quantize / --transpose / --range / --min-note / --min-rest / --hz are for notes: not with --sound pcm")
        rate = args.rate if args.rate is not None else PCM_PRESETS[args.quality]["rate"]
        if not PCM_MIN_RATE <= rate <= PCM_MAX_RATE:
            parser.error(f"--rate must be {PCM_MIN_RATE} ~ {PCM_MAX_RATE}, got {rate}")
        if rate > PCM_RECOMMENDED_MAX_RATE:
            warnings.append(f"--rate {rate}: the sample interrupt takes much of the CPU above {PCM_RECOMMENDED_MAX_RATE}")
        if rate < 4000:
            warnings.append(f"--rate {rate}: muffled, nothing above {rate//2} Hz")
    else:
        if pcm_given:
            parser.error("--rate / --encoding / --gain are for --sound pcm")
        if args.sound == "active" and pitch_given:
            parser.error("--quantize / --transpose / --range are for --sound passive (an active buzzer has one pitch)")
        if args.sound == "passive" and args.hz is not None:
            parser.error("--hz is for --sound active")
        quantize = args.quantize or NOTE_PRESETS[args.sound][args.quality]["quantize"]
        if args.sound == "passive" and args.quality == "low" and quantize == "raw":
            parser.error("--quality low stores MIDI notes: it cannot keep --quantize raw (use medium or high)")
        if args.sound == "passive" and (low_hz < 100 or high_hz > 8000):
            warnings.append(f"--range {low_hz:g} {high_hz:g}: most buzzers are weak under 100 Hz and shrill over 8 kHz")
    return warnings


def get_settings(args: argparse.Namespace) -> Settings:
    if args.sound == "pcm":
        preset = PCM_PRESETS[args.quality]
        encoding = args.encoding or preset["encoding"]
        rate = args.rate or preset["rate"]
        quantize, min_note, min_rest = "semitone", 0.0, 0.0
    else:
        preset = NOTE_PRESETS[args.sound][args.quality]
        encoding = preset["encoding"]
        rate = 0
        quantize = args.quantize or preset["quantize"]
        min_note = args.min_note if args.min_note is not None else preset["min_note"]
        min_rest = args.min_rest if args.min_rest is not None else preset["min_rest"]
    return Settings(
        targets=args.target, sound=args.sound, quality=args.quality, encoding=encoding,
        start=args.start, duration=args.duration, speed=args.speed,
        quantize=quantize, transpose=args.transpose, low_hz=args.range[0], high_hz=args.range[1],
        min_note_ms=min_note, min_rest_ms=min_rest,
        silence_db=args.silence, voicing=args.voicing, onset_db=args.onset, rhythm_hz=args.hz,
        rate=rate, gain_db=args.gain,
    )


# ---------- conversion ----------

def get_notes_data(audio_path: Path, settings: Settings, warnings: list[str]) -> tuple:
    # (data words, count, duration ms, rhythm hz, extra setting lines, audio info, played notes)
    samples = decode(audio_path, settings.start, settings.duration)
    audio_info = f"{len(samples)/SAMPLE_RATE:.2f} s used, {audio_path.suffix.lower()[1:]}, analysed as mono {SAMPLE_RATE} Hz"
    frames = get_frames(samples)
    loudness = get_loudness_db(frames)
    midis = get_smoothed_midis(get_pitches(frames, settings.voicing))
    segments = get_cleaned(get_segments(midis, loudness, settings), settings)
    if not segments:
        raise ValueError("nothing found: too quiet or no clear pitch (try --silence 50, --voicing 0.3)")
    notes, moved_count = get_notes(segments, settings)

    rhythm_hz = 0
    if settings.sound == "active":
        if settings.rhythm_hz is not None:
            rhythm_hz = settings.rhythm_hz
        else:
            voiced = midis[~np.isnan(midis)]
            rhythm_hz = min(4000, max(200, round(get_hz(float(np.median(voiced)))))) if len(voiced) else 2000
    words = get_steps(notes, settings.encoding)
    played = get_step_notes(words, settings.encoding, rhythm_hz)
    count = len(played)
    duration_ms = sum(ms for _, ms in played)
    short_count = sum(1 for hz, ms in played if hz > 0 and ms < 40)
    if short_count and settings.sound == "passive":
        warnings.append(f"{short_count} notes are shorter than 40 ms (Player's GAP_MS 20 makes them silent half the time)")
    if "uno" in settings.targets and count > MAX_PLAYER_STEPS:
        raise ValueError(f"{count} steps: audio::Player plays at most {MAX_PLAYER_STEPS} (cut it, or --target mac)")
    lines = get_notes_lines(settings, played, moved_count, rhythm_hz)
    return words, count, duration_ms, rhythm_hz, lines, audio_info, played


def get_pcm_data(audio_path: Path, settings: Settings, warnings: list[str]) -> tuple:
    # (data bytes, count, duration ms, real rate, extra setting lines, audio info, played samples)
    real_rate = get_real_rate(settings.rate)
    resample_rate = round(real_rate)
    samples = decode(audio_path, settings.start, settings.duration, rate=resample_rate, tempo=settings.speed)
    audio_info = f"{len(samples)/resample_rate:.2f} s, {audio_path.suffix.lower()[1:]}, resampled to mono {resample_rate} Hz"
    peak = float(np.max(np.abs(samples)))
    if peak < 1e-4:
        raise ValueError("silent: nothing to keep")
    scaled = samples/peak*0.98*10**(settings.gain_db/20)
    clipped = float(np.mean(np.abs(scaled) > 1.0))
    scaled = np.clip(scaled, -1.0, 1.0)
    data, played = get_pcm_values(scaled, settings.encoding)
    if get_unpacked_played(data, len(played), settings.encoding) != played:
        raise ValueError("internal: the encoded samples do not decode back (please report)")
    count = len(played)
    duration_ms = round(1000*count/real_rate)
    error = np.sqrt(np.mean(((np.array(played) - 128)/128 - np.clip(scaled, -1, 127/128))**2))
    snr = 20*math.log10(np.sqrt(np.mean(scaled**2))/max(error, 1e-9))
    lines = [f"rate        {settings.rate} Hz asked, {real_rate:.1f} Hz on the board (Uno::UnoPcmOut)",
             f"level       peak to 0.98, gain {settings.gain_db:+g} dB, {100*clipped:.2f} % clipped",
             f"result      {count} samples, {count/real_rate:.2f} s, signal / error {snr:.1f} dB, "
             f"{len(data)/(count/real_rate):.0f} bytes a second"]
    if clipped > 0.01:
        warnings.append(f"{100*clipped:.1f} % of the samples clipped: lower --gain")
    return data, count, duration_ms, real_rate, lines, audio_info, played


def convert_audio_to_h(args: argparse.Namespace, command: str, warnings: list[str]) -> bool:
    # returns True if selected.h passes the check
    audio_path = Path(args.audio_path).expanduser().resolve()
    output_dir_path = Path(args.output_dir_path).expanduser().resolve()
    if output_dir_path.exists() and not output_dir_path.is_dir():
        raise NotADirectoryError(f"Not a dir: {output_dir_path}")
    output_dir_path.mkdir(parents=True, exist_ok=True)
    settings = get_settings(args)

    if settings.sound == "pcm":
        data, count, duration_ms, real_rate, lines, audio_info, played = get_pcm_data(audio_path, settings, warnings)
        rate, rhythm_hz = settings.rate, 0
        data_bytes = len(data)
        bytes_per_s = data_bytes/max(duration_ms/1000, 1e-3)
    else:
        data, count, duration_ms, rhythm_hz, lines, audio_info, played = get_notes_data(audio_path, settings, warnings)
        rate = 0
        data_bytes = 2*len(data)
        bytes_per_s = 0

    if "uno" in settings.targets:
        if data_bytes > FLASH_BYTES:
            raise ValueError(f"{data_bytes} bytes: more than the UNO's flash ({FLASH_BYTES}); cut it (--duration), "
                             f"lower --quality, or --target mac")
        if data_bytes > RECOMMENDED_MAX_BYTES[settings.sound]:
            warnings.append(f"{data_bytes} bytes: above {RECOMMENDED_MAX_BYTES[settings.sound]}, little flash left for the program")
    if "mac" in settings.targets and bytes_per_s > STREAM_RECOMMENDED_BYTES_PER_S:
        warnings.append(f"{bytes_per_s:.0f} bytes a second: streaming may run short above {STREAM_RECOMMENDED_BYTES_PER_S} "
                        f"(StreamSamplePlayer counts underruns())")

    name = args.name or get_name(audio_path, settings)
    output_file_path = output_dir_path/(name + ".h")
    settings_str = get_settings_str(settings, audio_info, lines, data_bytes)
    data_str = get_c_data_getter_str(settings, data)
    if settings.targets == ["mac"]:
        # data in output/mac/ (never included), the board gets a small file with the path
        data_file_path = output_dir_path/MAC_DIR_NAME/(name + ".h")
        path = get_lib_relative_str(data_file_path)
        info_str = get_c_info_str(settings, count, duration_ms, rate, rhythm_hz, data_bytes, path)
        files = [
            (data_file_path, get_c_str(name, command, audio_path, settings_str, info_str, get_usage_str(name, settings, True), data_str)),
            (output_file_path, get_c_str(name, command, audio_path, settings_str, info_str,
                                         get_usage_str(name, settings, False) + f"\nthe data is in {path}", "")),
        ]
    else:
        path = get_lib_relative_str(output_file_path)
        info_str = get_c_info_str(settings, count, duration_ms, rate, rhythm_hz, data_bytes, path)
        files = [(output_file_path, get_c_str(name, command, audio_path, settings_str, info_str, get_usage_str(name, settings, False), data_str))]
    if path.startswith("/"):
        warnings.append(f"{path} is outside lib/audio: stream.py only opens files inside lib/audio")

    print(f"[Info] {name}:")
    print("\n".join(" "*4 + line[5:] for line in settings_str.splitlines()[1:]))
    for warning in warnings:
        print(f"[Warning] {warning}")
    for file_path, c_str in files:
        file_path.parent.mkdir(parents=True, exist_ok=True)
        if file_path.exists():
            print(f"[Info] Replaced the old {file_path.name}")
        file_path.write_text(c_str)
        print(f"[Success] Saved as: {file_path}")
    if args.preview is not None:
        preview_path = Path(args.preview).expanduser()
        if settings.sound == "pcm":
            write_pcm_preview(preview_path, played, real_rate)
        else:
            write_notes_preview(preview_path, played, gap=settings.sound == "passive")
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
