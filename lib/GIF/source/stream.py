import sys
sys.dont_write_bytecode = True # importing convert.py must not leave __pycache__

import argparse
import bisect
import glob
import re
import time
from pathlib import Path

import convert

try:
    import serial
except ImportError:
    serial = None


"""
Func: send animations to GIF::StreamPlayer over USB serial; the board says which file, the computer keeps the time.
Usage:
    pip install pyserial
    python source/stream.py                         finds the board by itself, waits until one is plugged in
    python source/stream.py --port /dev/cu.usbmodem1101 --baud 1000000
    (close the PlatformIO serial monitor first: only one program can use the port)
Files:
    the board sends Animation::getPath() (made by convert.py, relative to lib/GIF), only files inside lib/GIF
    are opened; --target mac files and uno files both work, palette 256 does not (512 B RAM on the board)
Protocol (board -> computer: commands; computer -> board: at most 1 byte unasked), see lib/GIF/format.md:
    'O' open (chunk, flags, speed, start, path)   'N' next frame   'P' play   'S' pause
    'J' seek (uint32_t ms)   'V' speed (uint16_t %)   'L' loop (0 / 1)   'R' next chunk
    'D' an answer is ready (the board pulls it with 'R')
    answers: 'G' header, 'F' frame, 'K' no frame yet (every second, or when the status changes),
             'A' status, 'E' error; status = flags, position ms, frame
Time:
    position = start + (now - started) * speed while playing; looping, or stopping at the end (ended)
    'N' is answered when the frame on the screen is no longer the one due: late frames are skipped,
    the frame sent is the difference to the one on the screen
"""


KEEPALIVE_S = 1.0
STATUS_S = 5.0
POLL_S = 0.005
PORT_PATTERNS = ("/dev/cu.usbmodem*", "/dev/cu.usbserial*", "/dev/cu.wchusbserial*", "/dev/cu.SLAB_USBtoUART*")
LIB_DIR_PATH = Path(__file__).resolve().parent.parent # lib/GIF
PALETTES = {0: 16, 1: 256, 2: 565} # colorMode -> palette

FLAG_PLAYING = 1
FLAG_ENDED = 2
OPEN_PLAY = 1
OPEN_LOOP = 2

ERROR_NOT_FOUND = 1
ERROR_UNREADABLE = 2
ERROR_NOT_STREAMABLE = 3


class Animation:
    # the data of a .h made by convert.py, frames decoded to whole frames
    def __init__(self, header_path: Path):
        text = header_path.read_text()
        if "getFrames()" not in text:
            raise ValueError("no frames in this file (a --target mac file without frames?)")
        values = dict(re.findall(r"static constexpr \w+ (\w+) = (\d+);", text))
        for key in ("width", "height", "scaleX", "scaleY", "colorMode", "colorCount", "frameCount"):
            if key not in values:
                raise ValueError(f"no {key}: not made by convert.py (format 3)")
        self.width = int(values["width"])
        self.height = int(values["height"])
        self.scale_x = int(values["scaleX"])
        self.scale_y = int(values["scaleY"])
        self.palette_mode = PALETTES[int(values["colorMode"])]
        self.palette = self.get_array(text, "palette")[:int(values["colorCount"])]
        offsets = self.get_array(text, "offsets")
        frames_bytes = self.get_array(text, "frames")

        self.delays = []
        self.value_frames = []
        previous = None
        frame_count = int(values["frameCount"])
        ends = offsets[1:frame_count] + [len(frames_bytes)]
        for start, end in zip(offsets[:frame_count], ends):
            frame_bytes = frames_bytes[start:end]
            self.delays.append(frame_bytes[0] | (frame_bytes[1] << 8))
            previous = convert.get_decoded_frame(frame_bytes, previous, self.width, self.height, self.palette_mode)
            self.value_frames.append(previous)
        self.duration_ms = sum(self.delays)
        self.starts_ms = [sum(self.delays[:i]) for i in range(len(self.delays))]

    @staticmethod
    def get_array(text: str, name: str) -> list[int]:
        match = re.search(rf"\b{name}\[\] PROGMEM = \{{(.*?)\}};", text, re.DOTALL)
        if not match:
            raise ValueError(f"no array {name}[]")
        body = re.sub(r"//[^\n]*", "", match.group(1))
        return [int(value, 0) for value in re.findall(r"0x[0-9A-Fa-f]+|\d+", body)]


def get_u16(value: int) -> bytes:
    return bytes([value & 0xFF, (value >> 8) & 0xFF])


def get_u32(value: int) -> bytes:
    return bytes((value >> (8*i)) & 0xFF for i in range(4))


class Session:
    # one connected board: what it opened, the clock, the answers waiting to be pulled
    def __init__(self, port, tile: int, byte_cycles: float):
        self.port = port
        self.tile = tile
        self.byte_cycles = byte_cycles
        self.cache = {} # path -> Animation
        self.animation = None
        self.chunk_size = 64

        # clock: position = base_ms + (now - base_time) * speed / 100 while playing
        self.playing = False
        self.looping = True
        self.speed = 100
        self.base_ms = 0.0
        self.base_time = time.perf_counter()

        self.shown_count = None # frames since the start (not wrapped) on the screen, None = nothing yet
        self.shown_frame = None
        self.next_asked_time = None # 'N' waiting
        self.told_flags = None # flags in the last answer to 'N'

        # answers: one is offered ('D'), pulled chunk by chunk with 'R'
        self.answers = []
        self.current = None
        self.sent = 0

        self.status_time = time.perf_counter()
        self.status_drawn = 0
        self.status_skipped = 0

    # ---------- clock ----------

    def get_position_ms(self) -> float:
        if not self.playing:
            return self.base_ms
        return self.base_ms + (time.perf_counter() - self.base_time)*1000*self.speed/100

    def set_position_ms(self, position_ms: float):
        self.base_ms = position_ms
        self.base_time = time.perf_counter()

    def is_ended(self) -> bool:
        return not self.looping and self.animation is not None and self.get_position_ms() >= self.animation.duration_ms

    def get_count(self, position_ms: float) -> int:
        # frame at the position, not wrapped (loop k, frame i -> k * frameCount + i)
        animation = self.animation
        frame_count = len(animation.delays)
        if not self.looping and position_ms >= animation.duration_ms:
            return frame_count - 1
        loop = int(position_ms//animation.duration_ms)
        index = bisect.bisect_right(animation.starts_ms, position_ms - loop*animation.duration_ms) - 1
        return loop*frame_count + index

    def get_status(self) -> bytes:
        position_ms = self.get_position_ms()
        if self.is_ended():
            # stop the clock at the end
            self.playing = False
            self.set_position_ms(self.animation.duration_ms)
            position_ms = self.base_ms
        flags = (FLAG_PLAYING if self.playing else 0) | (FLAG_ENDED if self.is_ended() else 0)
        shown_ms = position_ms
        frame = 0
        if self.animation is not None:
            shown_ms = position_ms % self.animation.duration_ms if self.looping else min(position_ms, self.animation.duration_ms)
            frame = self.get_count(position_ms) % len(self.animation.delays)
        return bytes([flags]) + get_u32(int(shown_ms)) + get_u16(frame)

    # ---------- answers ----------

    def add_answer(self, answer: bytes):
        self.answers.append(answer)
        self.offer()

    def offer(self):
        # at most 1 byte unasked: the next answer is offered only after the last one was pulled
        if self.current is None and self.answers:
            self.current = self.answers.pop(0)
            self.sent = 0
            self.port.write(b"D")

    def on_pull(self):
        if self.current is None:
            return
        data = self.current[self.sent:self.sent + self.chunk_size]
        self.port.write(bytes([len(data)]) + data)
        self.sent += len(data)
        if self.sent >= len(self.current):
            self.current = None
            self.offer()

    # ---------- commands ----------

    def read_bytes(self, count: int) -> bytes | None:
        data = self.port.read(count)
        return data if len(data) == count else None

    def on_open(self):
        header = self.read_bytes(8)
        if header is None:
            return
        chunk_size, flags = header[0], header[1]
        speed = header[2] | (header[3] << 8)
        start_ms = int.from_bytes(header[4:8], "little")
        length = self.read_bytes(1)
        if length is None:
            return
        path_bytes = self.read_bytes(length[0])
        if path_bytes is None:
            return
        path = path_bytes.decode("ascii", "replace")

        # a new open drops everything of the old one
        self.answers = []
        self.current = None
        self.next_asked_time = None
        self.shown_count = None
        self.shown_frame = None
        self.chunk_size = max(1, min(255, chunk_size))
        error = self.load(path)
        if error is not None:
            code, message = error
            print(f"[Error] {path}: {message}")
            message_bytes = message.encode("ascii", "replace")[:200]
            self.add_answer(b"E" + bytes([code, len(message_bytes)]) + message_bytes)
            return

        animation = self.animation
        self.playing = bool(flags & OPEN_PLAY)
        self.looping = bool(flags & OPEN_LOOP)
        self.speed = max(1, min(1000, speed))
        self.set_position_ms(min(start_ms, animation.duration_ms))
        color_mode = convert.COLOR_MODES[animation.palette_mode]
        header = bytes([animation.width, animation.height, animation.scale_x, animation.scale_y, color_mode, len(animation.palette)])
        header += get_u16(len(animation.delays)) + get_u32(animation.duration_ms)
        for color in animation.palette:
            header += get_u16(color)
        print(f"[Info] Opened {path}: {animation.width*animation.scale_x} x {animation.height*animation.scale_y} px, "
              f"palette {animation.palette_mode}, {len(animation.delays)} frames, {animation.duration_ms} ms, "
              f"{'playing' if self.playing else 'paused'}, loop {'on' if self.looping else 'off'}, speed {self.speed}%")
        self.add_answer(b"G" + self.get_status() + header)

    def load(self, path: str) -> tuple[int, str] | None:
        file_path = (LIB_DIR_PATH/path).resolve()
        if LIB_DIR_PATH not in file_path.parents or file_path.suffix != ".h":
            return ERROR_NOT_FOUND, f"only .h files inside {LIB_DIR_PATH.name}/ are opened"
        if not file_path.is_file():
            return ERROR_NOT_FOUND, "not found (run convert.py, or the board has an old path)"
        try:
            if file_path not in self.cache or self.cache[file_path][0] != file_path.stat().st_mtime:
                self.cache[file_path] = (file_path.stat().st_mtime, Animation(file_path))
        except (ValueError, KeyError, IndexError, OSError) as e:
            return ERROR_UNREADABLE, f"not readable: {e}"
        animation = self.cache[file_path][1]
        if animation.palette_mode == 256:
            return ERROR_NOT_STREAMABLE, "palette 256 cannot be streamed (512 B RAM on the board)"
        self.animation = animation
        return None

    def on_control(self, command: bytes):
        if command == b"P":
            if self.is_ended():
                self.set_position_ms(0)
            self.set_position_ms(self.get_position_ms())
            self.playing = True
        elif command == b"S":
            self.set_position_ms(self.get_position_ms())
            self.playing = False
        elif command == b"J":
            value = self.read_bytes(4)
            if value is None:
                return
            position_ms = int.from_bytes(value, "little")
            if self.animation is not None:
                position_ms = position_ms % self.animation.duration_ms if self.looping else min(position_ms, self.animation.duration_ms)
            self.set_position_ms(position_ms)
        elif command == b"V":
            value = self.read_bytes(2)
            if value is None:
                return
            self.set_position_ms(self.get_position_ms())
            self.speed = max(1, min(1000, value[0] | (value[1] << 8)))
        elif command == b"L":
            value = self.read_bytes(1)
            if value is None:
                return
            position_ms = self.get_position_ms()
            if self.animation is not None and self.looping:
                position_ms %= self.animation.duration_ms
            self.set_position_ms(position_ms)
            self.looping = value[0] != 0
        self.add_answer(b"A" + self.get_status())

    def on_command(self, command: bytes):
        if command == b"O":
            self.on_open()
        elif self.animation is None:
            return
        elif command == b"N":
            self.next_asked_time = time.perf_counter()
        elif command == b"R":
            self.on_pull()
        elif command in (b"P", b"S", b"J", b"V", b"L"):
            self.on_control(command)
        # anything else: noise (e.g. the bootloader after a reset), ignored

    # ---------- frames ----------

    def serve_next(self):
        # 'N' waiting and nothing else to say: the frame due, or 'K' (status) after a while or when it changed
        if self.next_asked_time is None or self.current is not None or self.answers:
            return
        position_ms = self.get_position_ms()
        count = self.get_count(position_ms)
        if self.shown_count is None or count != self.shown_count:
            if self.shown_count is not None and count > self.shown_count + 1:
                self.status_skipped += count - self.shown_count - 1
            self.send_frame(count)
            return
        status = self.get_status()
        if time.perf_counter() - self.next_asked_time >= KEEPALIVE_S or status[0] != self.told_flags:
            self.next_asked_time = None
            self.told_flags = status[0]
            self.add_answer(b"K" + status)

    def send_frame(self, count: int):
        animation = self.animation
        index = count % len(animation.delays)
        current = animation.value_frames[index]
        frame_bytes, _, _ = convert.get_frame_bytes(self.shown_frame, current, animation.width, animation.height,
                                                    animation.delays[index], animation.scale_x, animation.scale_y,
                                                    animation.palette_mode, self.tile, self.byte_cycles)
        self.shown_count = count
        self.shown_frame = current
        self.next_asked_time = None
        self.status_drawn += 1
        status = self.get_status()
        self.told_flags = status[0]
        self.add_answer(b"F" + status + bytes(frame_bytes))
        self.print_status()

    def print_status(self):
        now = time.perf_counter()
        if now - self.status_time < STATUS_S:
            return
        seconds = now - self.status_time
        print(f"[Info] {self.status_drawn/seconds:.1f} fps drawn, {self.status_skipped} frames skipped to keep the time, "
              f"at {self.get_position_ms()/1000:.1f} s")
        self.status_time = now
        self.status_drawn = 0
        self.status_skipped = 0

    def run(self):
        while True:
            command = self.port.read(1)
            if command:
                self.on_command(command)
            if self.animation is not None:
                self.serve_next()


def find_port() -> str | None:
    ports = sorted(port for pattern in PORT_PATTERNS for port in glob.glob(pattern))
    if len(ports) > 1:
        print(f"[Warning] More than one board: {', '.join(ports)}; using {ports[0]} (or give --port)")
    return ports[0] if ports else None


def get_parser() -> argparse.ArgumentParser:
    mac = convert.TARGETS["mac"]
    parser = argparse.ArgumentParser(
        description="Send animations to GIF::StreamPlayer over USB serial; the board says which file, the computer keeps the time.",
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
    parser.add_argument("--tile", type=convert.get_positive_int, default=8, help="tile size in px for finding changed areas (default 8)")
    return parser


def main():
    parser = get_parser()
    args = parser.parse_args()
    if serial is None:
        parser.error("pyserial is not installed: pip install pyserial")
    # one byte = 10 bits on the line, in CPU cycles (+ chunk requests)
    byte_cycles = convert.BOARD["cpuHz"]*10/args.baud*1.1

    print("[Info] Ctrl+C to stop")
    waiting_printed = False
    try:
        while True:
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
            print(f"[Info] Connected to {port_name} at {args.baud} baud, waiting for the board to open an animation")
            try:
                Session(port, args.tile, byte_cycles).run()
            except (serial.SerialException, OSError) as e:
                print(f"[Info] {port_name} is gone ({e}), waiting again")
            finally:
                port.close()
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[Info] Stopped")


if __name__ == "__main__":
    main()
