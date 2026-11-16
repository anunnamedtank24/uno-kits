import sys
sys.dont_write_bytecode = True # importing convert.py must not leave __pycache__

import argparse
import glob
import re
import subprocess
import threading
import time
from pathlib import Path

import convert

try:
    import serial
except ImportError:
    serial = None


"""
Func: send sounds to audio::StreamPlayer (notes, rhythm) and audio::StreamSamplePlayer (pcm) over USB serial;
      the board says which file, and pulls the data piece by piece while it plays.
Usage:
    pip install pyserial
    python source/stream.py                         finds the board by itself, waits until one is plugged in
    python source/stream.py --port /dev/cu.usbmodem1101 --baud 1000000
    (close the PlatformIO serial monitor and the stream.py of lib/image / lib/video first: one program per port)
    uploads: while avrdude or "pio run -t upload" runs, the port is released, then taken again
    opening the port resets the board (macOS raises DTR on open, the UNO resets on it)
Files:
    the board sends Melody::getPath() (made by convert.py, relative to lib/audio), only files inside lib/audio
    are opened; --target mac files (output/mac/) and uno files both work
Protocol (board -> computer: commands; computer -> board: at most 1 byte unasked), see lib/audio/stream_link.h:
    'A' sound (chunk, path length, path)   'R' next piece   'M' a line of text to print (length, text)
    'D' the answer is ready (the board pulls it with 'R': [length 1 ~ chunk][bytes])
    answers: 'H' kind, encoding, rate (u16), count (u32), rhythmHz (u16), data size (u32), data;
             'E' error (code, message length, message)
"""


POLL_S = 0.005
PORT_PATTERNS = ("/dev/cu.usbmodem*", "/dev/cu.usbserial*", "/dev/cu.wchusbserial*", "/dev/cu.SLAB_USBtoUART*")
UPLOAD_CHECK_S = 0.3
# avrdude itself, or PlatformIO starting an upload (released before it even builds)
# only the program itself (its first word, or the script python runs), not a shell line that mentions it
UPLOAD_PATTERN = re.compile(r"^\S*\bavrdude\s|^(\S*python\S*\s+)?\S*\b(platformio|pio)\s.*(-t|--target)[ =]?upload\b")
LIB_DIR_PATH = Path(__file__).resolve().parent.parent # lib/audio
DEFAULT_BAUD = 1000000 # Uno::UnoSerial<1000000>

ERROR_NOT_FOUND = 1
ERROR_UNREADABLE = 2


class UploadRunning(Exception):
    pass


class UploadWatcher:
    # runs ps in its own thread: ps takes tens of ms, the stream must not stop for it
    def __init__(self):
        self.uploading = False
        thread = threading.Thread(target=self.run, daemon=True)
        thread.start()

    def run(self):
        while True:
            self.uploading = is_uploading()
            time.sleep(UPLOAD_CHECK_S)


def is_uploading() -> bool:
    try:
        lines = subprocess.run(["ps", "-axo", "command="], capture_output=True, text=True, timeout=2).stdout.splitlines()
    except (OSError, subprocess.SubprocessError):
        return False
    return any(UPLOAD_PATTERN.search(line) for line in lines)


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
            self.data = bytes(self.get_array(text, "samples"))
        else:
            words = self.get_array(text, "notes")
            self.data = b"".join(word.to_bytes(2, "little") for word in words)
        if len(self.data) != int(values["dataSize"]):
            raise ValueError(f"{len(self.data)} bytes of data, dataSize says {values['dataSize']}")

    @staticmethod
    def get_array(text: str, name: str) -> list[int]:
        match = re.search(rf"\b{name}\[\] PROGMEM = \{{(.*?)\}};", text, re.DOTALL)
        if not match:
            raise ValueError(f"no array {name}[] (a --target mac file for the board, without data?)")
        body = re.sub(r"//[^\n]*", "", match.group(1))
        return [int(value, 0) for value in re.findall(r"0x[0-9A-Fa-f]+|\d+", body)]

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


class Session:
    # one connected board: the answer it is pulling
    def __init__(self, port):
        self.port = port
        self.cache = {} # path -> (mtime, Sound)
        self.chunk_size = 32
        self.answer = None
        self.sent = 0
        self.started = 0.0
        self.path = ""

    def read_bytes(self, count: int) -> bytes | None:
        data = self.port.read(count)
        return data if len(data) == count else None

    def on_sound(self):
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
        print(f"[Info] Sending {self.path}: {result.get_description()}")
        self.offer(result.get_answer())

    def on_message(self):
        length = self.read_bytes(1)
        if length is None:
            return
        text = self.read_bytes(length[0])
        if text is not None:
            print(f"[Board] {text.decode('ascii', 'replace')}")

    def load(self, path: str):
        file_path = (LIB_DIR_PATH/path).resolve()
        if LIB_DIR_PATH not in file_path.parents or file_path.suffix != ".h":
            return ERROR_NOT_FOUND, f"only .h files inside {LIB_DIR_PATH.name}/ are opened"
        if not file_path.is_file():
            return ERROR_NOT_FOUND, "not found (run convert.py, or the board has an old path)"
        try:
            mtime = file_path.stat().st_mtime
            if file_path not in self.cache or self.cache[file_path][0] != mtime:
                self.cache[file_path] = (mtime, Sound(file_path))
        except (ValueError, KeyError, IndexError, OSError) as e:
            return ERROR_UNREADABLE, f"not readable: {e}"
        return self.cache[file_path][1]

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
                print(f"[Success] {self.path}: {len(self.answer)} bytes in {seconds:.2f} s "
                      f"({len(self.answer)/max(seconds, 1e-6):.0f} bytes a second)")
            self.answer = None

    def run(self, watcher: UploadWatcher):
        while True:
            # an upload needs the port: give it up at once
            if watcher.uploading:
                raise UploadRunning()
            command = self.port.read(1)
            if command == b"A":
                self.on_sound()
            elif command == b"R":
                self.on_pull()
            elif command == b"M":
                self.on_message()
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
    parser = argparse.ArgumentParser(
        description="Send sounds to audio::StreamPlayer / StreamSamplePlayer over USB serial; the board says which file.",
        epilog=(
            "examples:\n"
            "  python source/stream.py\n"
            "  python source/stream.py --port /dev/cu.usbmodem1101\n"
            "needs: pip install pyserial"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--port", help=f"serial port of the board (default: the first of {', '.join(PORT_PATTERNS)})")
    parser.add_argument("--baud", type=convert.get_positive_int, default=DEFAULT_BAUD,
                        help=f"same as Uno::UnoSerial<BAUD> (default {DEFAULT_BAUD})")
    return parser


def main():
    parser = get_parser()
    args = parser.parse_args()
    if serial is None:
        parser.error("pyserial is not installed: pip install pyserial")

    print("[Info] Ctrl+C to stop")
    watcher = UploadWatcher()
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
            print(f"[Info] Connected to {port_name} at {args.baud} baud, waiting for the board to ask for a sound")
            try:
                Session(port).run(watcher)
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
