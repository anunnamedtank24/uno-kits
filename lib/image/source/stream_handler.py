import sys
sys.dont_write_bytecode = True # importing convert.py must not leave __pycache__

import re
from pathlib import Path

import convert


"""
Func: what lib/stream/source/stream.py does for pictures: the board asks with 'I', this reads the file
      and answers with its pixels (image::StreamImage); not run by itself
Protocol (after lib/stream's own, see lib/stream/README.md):
    board: 'I', CHUNK, path length, path (Picture::getPath(), relative to lib/image; only .h files inside lib/image)
    answer: 'H' width, height, scaleX, scaleY, colorMode, encoding, colorCount, data size (uint32), palette, pixels
            'E' error (code, message length, message)
    --target stream files (the pixels in #if 0) and flash files both work; palette 256 too: colorCount 0 = 256,
    the board keeps that palette in RAM taken while it draws
"""


NAME = "pictures"
COMMANDS = b"I"
LIB_DIR_PATH = Path(__file__).resolve().parent.parent # lib/image
PALETTES = {mode: palette for palette, mode in convert.COLOR_MODES.items()} # colorMode -> palette
ENCODING_NAMES = {code: name for name, code in convert.ENCODINGS.items()}


class Picture:
    # the data of a .h made by convert.py
    def __init__(self, header_path: Path):
        text = header_path.read_text()
        if "getPixels()" not in text:
            raise ValueError("no pixels in this file")
        values = dict(re.findall(r"static constexpr \w+ (\w+) = (\d+);", text))
        for key in ("width", "height", "scaleX", "scaleY", "colorMode", "encoding", "colorCount"):
            if key not in values:
                raise ValueError(f"no {key}: not made by convert.py (format 1)")
        self.width = int(values["width"])
        self.height = int(values["height"])
        self.scale_x = int(values["scaleX"])
        self.scale_y = int(values["scaleY"])
        self.color_mode = int(values["colorMode"])
        self.encoding = int(values["encoding"])
        self.palette_mode = PALETTES[self.color_mode]
        self.palette = get_array(text, "palette")[:int(values["colorCount"])]
        self.data = bytes(get_array(text, "pixels"))
        # the pixels must decode to width x height: a broken file is refused here, not drawn half
        count = self.width*self.height
        decoded = convert.get_decoded_values(list(self.data), count, self.palette_mode, ENCODING_NAMES[self.encoding])
        if len(decoded) != count:
            raise ValueError(f"{len(decoded)} pixels, {count} expected")

    def get_answer(self) -> bytes:
        # 'H', width, height, scaleX, scaleY, colorMode, encoding, colorCount, data size (uint32), palette, pixels
        # colorCount: one byte, 0 = 256 (palette 256)
        header = bytes([self.width, self.height, self.scale_x, self.scale_y, self.color_mode, self.encoding, len(self.palette) & 0xFF])
        header += len(self.data).to_bytes(4, "little")
        palette = b"".join(color.to_bytes(2, "little") for color in self.palette)
        return b"H" + header + palette + self.data


def get_array(text: str, name: str) -> list[int]:
    match = re.search(rf"\b{name}\[\][^=\n]*= \{{(.*?)\}};", text, re.DOTALL)
    if not match:
        raise ValueError(f"no array {name}[]")
    body = re.sub(r"//[^\n]*", "", match.group(1))
    return [int(value, 0) for value in re.findall(r"0x[0-9A-Fa-f]+|\d+", body)]


class Handler:
    def __init__(self, link, args):
        self.link = link
        self.cache = {} # path -> (mtime, Picture)

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
        picture = result
        print(f"[INFO] Sending {path}: {picture.width*picture.scale_x} x {picture.height*picture.scale_y} px, "
              f"palette {picture.palette_mode}, {ENCODING_NAMES[picture.encoding]}, {len(picture.data)} bytes")
        self.link.answer(picture.get_answer(), chunk, path)

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
                self.cache[file_path] = (mtime, Picture(file_path))
        except (ValueError, KeyError, IndexError, OSError) as e:
            return self.link.ERROR_UNREADABLE, f"not readable: {e}"
        return self.cache[file_path][1]
