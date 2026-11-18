import sys
sys.dont_write_bytecode = True # importing convert.py must not leave __pycache__

import re
from pathlib import Path

import convert


"""
Func: what lib/stream/source/stream.py does for sounds: the board asks with 'A', this reads the file and
      answers with its data (audio::StreamPlayer: notes, rhythm; audio::StreamSamplePlayer: pcm); not run by itself
Protocol (after lib/stream's own, see lib/stream/README.md):
    board: 'A', CHUNK, path length, path (Melody::getPath(), relative to lib/audio; only .h files inside lib/audio)
    answer: 'H' kind, encoding, rate (u16), count (u32), rhythmHz (u16), data size (u32), data
            'E' error (code, message length, message)
    --target stream files (the data in #if 0) and flash files both work; the board pulls the data piece by piece
    while it plays
"""


NAME = "sounds"
COMMANDS = b"A"
LIB_DIR_PATH = Path(__file__).resolve().parent.parent # lib/audio


class Sound:
    # the data of a .h made by convert.py
    def __init__(self, header_path: Path):
        text = header_path.read_text()
        values = dict(re.findall(r"static constexpr \w+ (\w+) = (\d+);", text))
        for key in ("kind", "encoding", "count", "rate", "rhythmHz", "dataSize"):
            if key not in values:
                raise ValueError(f"no {key}: not made by convert.py (format {convert.check.FORMAT_VERSION})")
        self.kind = int(values["kind"])
        self.encoding = int(values["encoding"])
        self.count = int(values["count"])
        self.rate = int(values["rate"])
        self.rhythm_hz = int(values["rhythmHz"])
        if self.kind == 2:
            self.data = bytes(get_array(text, "samples"))
        else:
            words = get_array(text, "notes")
            self.data = b"".join(word.to_bytes(2, "little") for word in words)
        if len(self.data) != int(values["dataSize"]):
            raise ValueError(f"{len(self.data)} bytes of data, dataSize says {values['dataSize']}")

    def get_answer(self) -> bytes:
        header = bytes([self.kind, self.encoding]) + self.rate.to_bytes(2, "little") + self.count.to_bytes(4, "little")
        header += self.rhythm_hz.to_bytes(2, "little") + len(self.data).to_bytes(4, "little")
        return b"H" + header + self.data

    def get_description(self) -> str:
        kind = convert.KIND_NAMES[self.kind]
        encoding = convert.ENCODING_NAMES[self.encoding]
        if self.kind == 2:
            return f"{kind}, {encoding}, {self.count} samples at {self.rate} Hz, {len(self.data)} bytes"
        return f"{kind}, {encoding}, {self.count} steps, {len(self.data)} bytes"


def get_array(text: str, name: str) -> list[int]:
    match = re.search(rf"\b{name}\[\] PROGMEM = \{{(.*?)\}};", text, re.DOTALL)
    if not match:
        raise ValueError(f"no array {name}[]")
    body = re.sub(r"//[^\n]*", "", match.group(1))
    return [int(value, 0) for value in re.findall(r"0x[0-9A-Fa-f]+|\d+", body)]


class Handler:
    def __init__(self, link, args):
        self.link = link
        self.cache = {} # path -> (mtime, Sound)

    def on_command(self, command: bytes):
        # a new request: the board gave up what it did not pull of the last answer
        self.link.drop_answers()
        header = self.link.read_bytes(1)
        path = self.link.read_path()
        if header is None or path is None:
            return
        chunk = header[0]
        result = self.load(path)
        if isinstance(result, tuple):
            self.link.error(*result, chunk, path)
            return
        print(f"[INFO] Sending {path}: {result.get_description()}")
        self.link.answer(result.get_answer(), chunk, path)

    def poll(self):
        pass

    def load(self, path: str):
        file_path = (LIB_DIR_PATH/path).resolve()
        if LIB_DIR_PATH not in file_path.parents or file_path.suffix != ".h":
            return self.link.ERROR_NOT_FOUND, f"only .h files inside {LIB_DIR_PATH.name}/ are opened"
        if not file_path.is_file():
            return self.link.ERROR_NOT_FOUND, "not found (run convert.py, or the board has an old path)"
        try:
            mtime = file_path.stat().st_mtime
            if file_path not in self.cache or self.cache[file_path][0] != mtime:
                self.cache[file_path] = (mtime, Sound(file_path))
        except (ValueError, KeyError, IndexError, OSError) as e:
            return self.link.ERROR_UNREADABLE, f"not readable: {e}"
        return self.cache[file_path][1]
