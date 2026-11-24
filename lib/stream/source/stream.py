import sys
sys.dont_write_bytecode = True # loading the handlers (and their convert.py) must not leave __pycache__

import argparse
import importlib.util
import json
import re
import subprocess
import threading
import time
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None


"""
Func: the computer's side of lib/stream: sends the board whatever file it asks for, pictures, animations and
      sounds alike, over USB serial; one program for all of them (one program per port anyway)
Usage:
    pip install pyserial
    python lib/stream/source/stream.py                         finds the board by itself, waits until one is plugged in
    python lib/stream/source/stream.py --port /dev/cu.usbmodem1101   (Windows: --port COM3, Linux: --port /dev/ttyACM0)
    python lib/stream/source/stream.py --baud 1000000          the same as Uno::UnoSerial<BAUD> of the program
    (close the PlatformIO serial monitor first: only one program can use the port)
    uploads: while avrdude or "pio run -t upload" runs, the port is released, then taken again
    opening the port resets the board (the port raises DTR on open on every system, the UNO resets on it)
Handlers:
    every lib/<kind>/source/stream_handler.py is loaded: it names the commands it takes (COMMANDS) and reads its
    own files (only .h files inside lib/<kind>); now: lib/image ('I'), lib/video ('O' 'N' 'P' 'S' 'J' 'V' 'L'),
    lib/audio ('A'); a new kind only needs its own stream_handler.py
Protocol (board -> computer: commands; computer -> board: at most 1 byte unasked), see lib/stream/README.md:
    a handler's command (with CHUNK and a path)      -> 'D' when its answer is ready
    'R'  the next piece of the answer                -> [length 1 ~ CHUNK][bytes]
    'M'  length, text                                -> printed here (for tests)
    an answer starts with a letter; 'E' = error: code (1 not found, 2 unreadable, 3 not streamable), length, message
"""


POLL_S = 0.005
# the board's numbers (--board, default custom_board of platformio.ini): serial.baud (the default of --baud),
# serial.usbVendorIds (how the board is found), passed on to the handlers (the drawing cycles of animations)
PROJECT_DIR_PATH = Path(__file__).resolve().parent.parent.parent.parent
UPLOAD_CHECK_S = 0.3
# avrdude itself, or PlatformIO starting an upload (released before it even builds)
# only the program itself (its first word, or the script python runs), not a shell line that mentions it
PROGRAM = r'(?:"[^"]*?|\S*?)'
UPLOAD_PATTERN = re.compile(rf'^{PROGRAM}\bavrdude(?:\.exe)?"?\s|'
                            rf'^(?:{PROGRAM}python\S*"?\s+)?{PROGRAM}\b(?:platformio|pio)(?:\.exe)?"?\s.*(?:-t|--target)[ =]?upload\b')
LIB_DIR_PATH = Path(__file__).resolve().parent.parent.parent # lib
HANDLER_NAME = "stream_handler.py"
# modules a handler's folder has under the same names in every lib: loaded apart for each handler
SHARED_NAMES = ("convert", "check")


class UploadRunning(Exception):
    pass


class UploadWatcher:
    # runs ps in its own thread: ps takes tens of ms, a stream must not stop for it
    def __init__(self):
        self.uploading = False
        threading.Thread(target=self.run, daemon=True).start()

    def run(self):
        while True:
            self.uploading = is_uploading()
            time.sleep(UPLOAD_CHECK_S)


def get_command_lines() -> list[str]:
    # the command line of every running program (macOS / Linux: ps, Windows: PowerShell)
    if sys.platform == "win32":
        command = ["powershell", "-NoProfile", "-Command",
                   "Get-CimInstance Win32_Process | ForEach-Object { $_.CommandLine }"]
    else:
        command = ["ps", "-axo", "command="]
    try:
        return subprocess.run(command, capture_output=True, text=True, timeout=5).stdout.splitlines()
    except (OSError, subprocess.SubprocessError):
        return []


def is_uploading() -> bool:
    return any(UPLOAD_PATTERN.search(line) for line in get_command_lines())


class Link:
    # what a handler gets: the bytes after its command, and the answers, offered one at a time
    ERROR_NOT_FOUND = 1
    ERROR_UNREADABLE = 2
    ERROR_NOT_STREAMABLE = 3

    def __init__(self, port):
        self.port = port
        self.answers = [] # (bytes, chunk, message when done)
        self.current = None
        self.sent = 0
        self.started = 0.0

    def read_bytes(self, count: int) -> bytes | None:
        data = self.port.read(count)
        return data if len(data) == count else None

    def read_path(self) -> str | None:
        # path length, path
        length = self.read_bytes(1)
        if length is None:
            return None
        path_bytes = self.read_bytes(length[0])
        return None if path_bytes is None else path_bytes.decode("ascii", "replace")

    def answer(self, data: bytes, chunk: int, done: str | None = None):
        # queued; done: printed with the time once the board pulled all of it
        self.answers.append((data, max(1, min(255, chunk)), done))
        self.offer()

    def error(self, code: int, message: str, chunk: int, path: str):
        print(f"[ERROR] {path}: {message}")
        message_bytes = message.encode("ascii", "replace")[:200]
        self.answer(b"E" + bytes([code, len(message_bytes)]) + message_bytes, chunk)

    def drop_answers(self):
        # a new open: what was not pulled yet is of no use
        self.answers = []
        self.current = None

    def is_busy(self) -> bool:
        return self.current is not None or bool(self.answers)

    def offer(self):
        # at most 1 byte unasked: the next answer is offered only after the last one was pulled
        if self.current is None and self.answers:
            self.current = self.answers.pop(0)
            self.sent = 0
            self.started = time.perf_counter()
            self.port.write(b"D")

    def on_pull(self):
        if self.current is None:
            return
        data, chunk, done = self.current
        piece = data[self.sent:self.sent + chunk]
        self.port.write(bytes([len(piece)]) + piece)
        self.sent += len(piece)
        if self.sent >= len(data):
            if done:
                seconds = time.perf_counter() - self.started
                print(f"[SUCCESS] {done}: {len(data)} bytes in {seconds:.2f} s "
                      f"({len(data)/max(seconds, 1e-6):.0f} bytes a second)")
            self.current = None
            self.offer()

    def on_message(self):
        length = self.read_bytes(1)
        if length is None:
            return
        text = self.read_bytes(length[0])
        if text is not None:
            print(f"[BOARD] {text.decode('ascii', 'replace')}")


def load_handler(handler_path: Path):
    # each handler imports its own convert.py / check.py: loaded with its folder first on the path,
    # and those names removed again, so the next handler gets its own
    source_dir_str = str(handler_path.parent)
    saved = {name: sys.modules.pop(name) for name in SHARED_NAMES if name in sys.modules}
    sys.path.insert(0, source_dir_str)
    try:
        kind = handler_path.parent.parent.name
        spec = importlib.util.spec_from_file_location(f"stream_handler_{kind}", handler_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    finally:
        sys.path.remove(source_dir_str)
        for name in SHARED_NAMES:
            sys.modules.pop(name, None)
        sys.modules.update(saved)


def get_handler_modules() -> list:
    modules = []
    for handler_path in sorted(LIB_DIR_PATH.glob(f"*/source/{HANDLER_NAME}")):
        try:
            modules.append(load_handler(handler_path))
        except Exception as e: # a broken handler must not stop the others
            print(f"[WARNING] {handler_path.relative_to(LIB_DIR_PATH)} not loaded: {e}")
    return modules


class Session:
    # one connected board: commands go to the handler that takes them
    def __init__(self, port, modules: list, args):
        self.link = Link(port)
        self.handlers = []
        self.commands = {}
        for module in modules:
            handler = module.Handler(self.link, args)
            self.handlers.append(handler)
            for command in module.COMMANDS:
                self.commands[bytes([command])] = handler

    def run(self, watcher: UploadWatcher):
        while True:
            # an upload needs the port: give it up at once
            if watcher.uploading:
                raise UploadRunning()
            command = self.link.port.read(1)
            if command == b"R":
                self.link.on_pull()
            elif command == b"M":
                self.link.on_message()
            elif command in self.commands:
                self.commands[command].on_command(command)
            # anything else: noise (e.g. the bootloader after a reset), ignored
            for handler in self.handlers:
                handler.poll()


def wait_for_upload():
    print("[INFO] An upload is running: the port is released until it ends")
    while is_uploading():
        time.sleep(UPLOAD_CHECK_S)
    # at once: the reset of opening the port then falls on the board's first moments
    print("[INFO] Upload done, taking the port again")


def get_board_path(given: str | None) -> Path | None:
    # --board, else custom_board of platformio.ini (relative to the project)
    if given:
        return Path(given).expanduser().resolve()
    ini_path = PROJECT_DIR_PATH/"platformio.ini"
    match = re.search(r"^custom_board[ \t]*=[ \t]*(.*?)[ \t]*(?:[ \t];.*)?$", ini_path.read_text(), re.MULTILINE) \
        if ini_path.exists() else None
    if match is None or not match.group(1):
        return None
    path = Path(match.group(1)).expanduser()
    return path if path.is_absolute() else PROJECT_DIR_PATH/path


def find_port(vendor_ids: set[int]) -> str | None:
    # any system (macOS /dev/cu.usbmodem..., Linux /dev/ttyACM0 / ttyUSB0, Windows COM3 ...): the board's USB serial
    # chips by their USB vendor id (board.json: serial.usbVendorIds)
    ports = sorted(info.device for info in list_ports.comports() if info.vid in vendor_ids)
    if len(ports) > 1:
        print(f"[WARNING] More than one board: {', '.join(ports)}; using {ports[0]} (or give --port)")
    return ports[0] if ports else None


def get_positive_int(value: str) -> int:
    try:
        number = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be an integer, got {value!r}")
    if number <= 0:
        raise argparse.ArgumentTypeError(f"must be > 0, got {number}")
    return number


def get_parser(modules: list) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Send the board whatever file it asks for (pictures, animations, sounds) over USB serial.",
        epilog=(
            "examples:\n"
            "  python lib/stream/source/stream.py\n"
            "  python lib/stream/source/stream.py --port /dev/cu.usbmodem1101\n"
            "needs: pip install pyserial"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--port", help="serial port of the board, e.g. /dev/cu.usbmodem1101, COM3, /dev/ttyACM0 (default: found by its USB id)")
    parser.add_argument("--board", metavar="FILE",
                        help="the board's numbers, e.g. lib/Uno/board.json (default: custom_board of platformio.ini)")
    parser.add_argument("--baud", type=get_positive_int,
                        help="the same as the program's BAUD (default: serial.baud of the board)")
    for module in modules:
        if hasattr(module, "add_arguments"):
            module.add_arguments(parser)
    return parser


def main():
    modules = get_handler_modules()
    parser = get_parser(modules)
    args = parser.parse_args()
    if serial is None:
        parser.error("pyserial is not installed: pip install pyserial")
    board_path = get_board_path(args.board)
    if board_path is None:
        parser.error("no board: give --board, or custom_board in platformio.ini (e.g. lib/Uno/board.json)")
    try:
        board = json.loads(board_path.read_text())
        vendor_ids = {int(value, 0) for value in board["serial"]["usbVendorIds"]}
        if args.baud is None:
            args.baud = board["serial"]["baud"]
    except (OSError, ValueError, KeyError) as e:
        parser.error(f"board file {board_path}: {e}")
    args.board = str(board_path)
    names = ", ".join(f"{module.NAME} ({' '.join(chr(c) for c in module.COMMANDS)})" for module in modules)
    print(f"[INFO] Serving {names}; Ctrl+C to stop")

    watcher = UploadWatcher()
    waiting_printed = False
    try:
        while True:
            if is_uploading():
                wait_for_upload()
                continue
            port_name = args.port or find_port(vendor_ids)
            if port_name is None:
                if not waiting_printed:
                    print("[INFO] Waiting for a board to be plugged in")
                    waiting_printed = True
                time.sleep(1)
                continue
            try:
                port = serial.Serial(port_name, args.baud, timeout=POLL_S)
            except (serial.SerialException, OSError) as e:
                if not waiting_printed:
                    print(f"[INFO] Waiting for {port_name}: {e}")
                    waiting_printed = True
                time.sleep(1)
                continue
            waiting_printed = False
            print(f"[INFO] Connected to {port_name} at {args.baud} baud, waiting for the board to ask for a file")
            try:
                Session(port, modules, args).run(watcher)
            except UploadRunning:
                port.close()
                wait_for_upload()
                continue
            except (serial.SerialException, OSError) as e:
                print(f"[INFO] {port_name} is gone ({e}), waiting again")
            finally:
                port.close()
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[INFO] Stopped")


if __name__ == "__main__":
    main()
