import sys
sys.dont_write_bytecode = True # importing convert.py must not leave __pycache__

import argparse
import glob
import re
import subprocess
import time
from pathlib import Path

import convert

try:
    import serial
except ImportError:
    serial = None


"""
Func: send pictures to image::StreamImage over USB serial; the board says which file.
Usage:
    pip install pyserial
    python source/stream.py                         finds the board by itself, waits until one is plugged in
    python source/stream.py --port /dev/cu.usbmodem1101 --baud 1000000
    (close the PlatformIO serial monitor and lib/video/source/stream.py first: only one program can use the port)
    uploads: while avrdude or "pio run -t upload" runs, the port is released, then taken again
    opening the port resets the board (macOS raises DTR on open, the UNO resets on it)
Files:
    the board sends Picture::getPath() (made by convert.py, relative to lib/image), only files inside lib/image
    are opened; --target mac files and uno files both work, palette 256 does not (512 B RAM on the board)
Protocol (board -> computer: commands; computer -> board: at most 1 byte unasked), see lib/image/source/README.md:
    'I' picture (chunk, path length, path)   'R' next piece
    'D' the answer is ready (the board pulls it with 'R': [length 1 ~ chunk][bytes])
    answers: 'H' header + palette + pixels, 'E' error (code, message length, message)
"""


POLL_S = 0.005
PORT_PATTERNS = ("/dev/cu.usbmodem*", "/dev/cu.usbserial*", "/dev/cu.wchusbserial*", "/dev/cu.SLAB_USBtoUART*")
UPLOAD_CHECK_S = 0.3
# avrdude itself, or PlatformIO starting an upload (released before it even builds)
UPLOAD_PATTERN = re.compile(r"\bavrdude\b|\b(platformio|pio)\b.*(-t|--target)[ =]?upload\b")
LIB_DIR_PATH = Path(__file__).resolve().parent.parent # lib/image
PALETTES = {mode: palette for palette, mode in convert.COLOR_MODES.items()} # colorMode -> palette
ENCODING_NAMES = {code: name for name, code in convert.ENCODINGS.items()}

ERROR_NOT_FOUND = 1
ERROR_UNREADABLE = 2
ERROR_NOT_STREAMABLE = 3


class UploadRunning(Exception):
    pass


def is_uploading() -> bool:
    try:
        lines = subprocess.run(["ps", "-axo", "command="], capture_output=True, text=True, timeout=2).stdout.splitlines()
    except (OSError, subprocess.SubprocessError):
        return False
    return any(UPLOAD_PATTERN.search(line) for line in lines)


class Picture:
    # the data of a .h made by convert.py
    def __init__(self, header_path: Path):
        text = header_path.read_text()
        if "getPixels()" not in text:
            raise ValueError("no pixels in this file (a --target mac file without pixels?)")
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
        self.palette = self.get_array(text, "palette")[:int(values["colorCount"])]
        self.data = bytes(self.get_array(text, "pixels"))
        # the pixels must decode to width x height: a broken file is refused here, not drawn half
        count = self.width*self.height
        decoded = convert.get_decoded_values(list(self.data), count, self.palette_mode, ENCODING_NAMES[self.encoding])
        if len(decoded) != count:
            raise ValueError(f"{len(decoded)} pixels, {count} expected")

    @staticmethod
    def get_array(text: str, name: str) -> list[int]:
        match = re.search(rf"\b{name}\[\] PROGMEM = \{{(.*?)\}};", text, re.DOTALL)
        if not match:
            raise ValueError(f"no array {name}[]")
        body = re.sub(r"//[^\n]*", "", match.group(1))
        return [int(value, 0) for value in re.findall(r"0x[0-9A-Fa-f]+|\d+", body)]

    def get_answer(self) -> bytes:
        # 'H', width, height, scaleX, scaleY, colorMode, encoding, colorCount, data size (uint32), palette, pixels
        header = bytes([self.width, self.height, self.scale_x, self.scale_y, self.color_mode, self.encoding, len(self.palette)])
        header += len(self.data).to_bytes(4, "little")
        palette = b"".join(color.to_bytes(2, "little") for color in self.palette)
        return b"H" + header + palette + self.data


class Session:
    # one connected board: the answer waiting to be pulled
    def __init__(self, port):
        self.port = port
        self.cache = {} # path -> (mtime, Picture)
        self.chunk_size = 64
        self.answer = None
        self.sent = 0
        self.started = 0.0
        self.path = ""

    def read_bytes(self, count: int) -> bytes | None:
        data = self.port.read(count)
        return data if len(data) == count else None

    def on_picture(self):
        header = self.read_bytes(2)
        if header is None:
            return
        chunk_size, length = header
        path_bytes = self.read_bytes(length)
        if path_bytes is None:
            return
        self.path = path_bytes.decode("ascii", "replace")
        self.chunk_size = max(1, chunk_size)
        result = self.load(self.path)
        if isinstance(result, tuple):
            code, message = result
            print(f"[Error] {self.path}: {message}")
            message_bytes = message.encode("ascii", "replace")[:200]
            self.offer(b"E" + bytes([code, len(message_bytes)]) + message_bytes)
            return
        picture = result
        print(f"[Info] Sending {self.path}: {picture.width*picture.scale_x} x {picture.height*picture.scale_y} px, "
              f"palette {picture.palette_mode}, {ENCODING_NAMES[picture.encoding]}, {len(picture.data)} bytes")
        self.offer(picture.get_answer())

    def load(self, path: str):
        file_path = (LIB_DIR_PATH/path).resolve()
        if LIB_DIR_PATH not in file_path.parents or file_path.suffix != ".h":
            return ERROR_NOT_FOUND, f"only .h files inside {LIB_DIR_PATH.name}/ are opened"
        if not file_path.is_file():
            return ERROR_NOT_FOUND, "not found (run convert.py, or the board has an old path)"
        try:
            mtime = file_path.stat().st_mtime
            if file_path not in self.cache or self.cache[file_path][0] != mtime:
                self.cache[file_path] = (mtime, Picture(file_path))
        except (ValueError, KeyError, IndexError, OSError) as e:
            return ERROR_UNREADABLE, f"not readable: {e}"
        picture = self.cache[file_path][1]
        if picture.palette_mode == 256:
            return ERROR_NOT_STREAMABLE, "palette 256 cannot be streamed (512 B RAM on the board)"
        return picture

    def offer(self, answer: bytes):
        # at most 1 byte unasked: 'D', then the board pulls
        self.answer = answer
        self.sent = 0
        self.started = time.perf_counter()
        self.port.write(b"D")

    def on_pull(self):
        if self.answer is None:
            return
        data = self.answer[self.sent:self.sent + self.chunk_size]
        self.port.write(bytes([len(data)]) + data)
        self.sent += len(data)
        if self.sent >= len(self.answer):
            seconds = time.perf_counter() - self.started
            if self.answer[:1] == b"H":
                print(f"[Success] {self.path}: {len(self.answer)} bytes in {seconds:.2f} s")
            self.answer = None

    def run(self):
        next_check = 0.0
        while True:
            # an upload needs the port: give it up at once
            now = time.perf_counter()
            if now >= next_check:
                next_check = now + UPLOAD_CHECK_S
                if is_uploading():
                    raise UploadRunning()
            command = self.port.read(1)
            if command == b"I":
                self.on_picture()
            elif command == b"R":
                self.on_pull()
            # anything else: noise (e.g. the bootloader after a reset), ignored


def wait_for_upload():
    print("[Info] An upload is running: the port is released until it ends")
    while is_uploading():
        time.sleep(UPLOAD_CHECK_S)
    print("[Info] Upload done, taking the port again")


def find_port() -> str | None:
    ports = sorted(port for pattern in PORT_PATTERNS for port in glob.glob(pattern))
    if len(ports) > 1:
        print(f"[Warning] More than one board: {', '.join(ports)}; using {ports[0]} (or give --port)")
    return ports[0] if ports else None


def get_parser() -> argparse.ArgumentParser:
    mac = convert.TARGETS["mac"]
    parser = argparse.ArgumentParser(
        description="Send pictures to image::StreamImage over USB serial; the board says which file.",
        epilog=(
            "examples:\n"
            "  python source/stream.py\n"
            "  python source/stream.py --port /dev/cu.usbmodem1101\n"
            "needs: pip install pyserial"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--port", help=f"serial port of the board (default: the first of {', '.join(PORT_PATTERNS)})")
    parser.add_argument("--baud", type=convert.get_positive_int, default=mac["baud"],
                        help=f"same as Uno::UnoSerial<BAUD> (default {mac['baud']}, config.json)")
    return parser


def main():
    parser = get_parser()
    args = parser.parse_args()
    if serial is None:
        parser.error("pyserial is not installed: pip install pyserial")

    print("[Info] Ctrl+C to stop")
    waiting_printed = False
    try:
        while True:
            if is_uploading():
                wait_for_upload()
                continue
            port_name = args.port or find_port()
            if port_name is None:
                if not waiting_printed:
                    print("[Info] Waiting for a board to be plugged in")
                    waiting_printed = True
                time.sleep(1)
                continue
            try:
                port = serial.Serial(port_name, args.baud, timeout=POLL_S)
            except (serial.SerialException, OSError) as e:
                if not waiting_printed:
                    print(f"[Info] Waiting for {port_name}: {e}")
                    waiting_printed = True
                time.sleep(1)
                continue
            waiting_printed = False
            print(f"[Info] Connected to {port_name} at {args.baud} baud, waiting for the board to ask for a picture")
            try:
                Session(port).run()
            except UploadRunning:
                port.close()
                wait_for_upload()
                continue
            except (serial.SerialException, OSError) as e:
                print(f"[Info] {port_name} is gone ({e}), waiting again")
            finally:
                port.close()
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[Info] Stopped")


if __name__ == "__main__":
    main()
