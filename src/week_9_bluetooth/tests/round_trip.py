import sys
sys.dont_write_bytecode = True # importing lib/bluetooth/source/packet.py must not leave __pycache__

import argparse
import collections
import random
import string
import threading
import time
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "lib" / "bluetooth" / "source"))
from packet import Decoder, encode, is_bluetooth  # noqa: E402
from ble import BleSerial  # noqa: E402


"""
Func: the computer's side of tests/1_HC_05.cpp, 2_HC_06.cpp, 4_BT05.cpp: a round trip through the board and Bluetooth
          a line typed here  ──USB──▶ UNO ──Bluetooth──▶ back here     (shown with the time it took)
          "< line" typed     ──Bluetooth──▶ UNO ──USB──▶ back here
      both are open at once: the USB port (the board's log, lines to forward) and Bluetooth: a serial port
      (classic: HC-05, HC-06, --bt PORT) or a BLE connection (BT05 / HM-10 type, --ble NAME, needs bleak)
Usage:
    pip install pyserial
    python src/week_9_bluetooth/tests/round_trip.py --list                    the serial ports
    python src/week_9_bluetooth/tests/round_trip.py --bt /dev/cu.UnoKits      the USB port is found by itself
    python src/week_9_bluetooth/tests/round_trip.py --bt COM5 --usb COM3      (Windows: the Outgoing COM port)
    python src/week_9_bluetooth/tests/round_trip.py --bt /dev/cu.UnoKits --count 100    100 lines by itself, a summary
    python src/week_9_bluetooth/tests/round_trip.py --ble TanksUno         BLE: by its name, no pairing (pip install bleak)
    (close pio device monitor first: one program per port; classic modules: pair first)
Shown:
    [BOARD] …                  every line the board prints on USB (its AT results, what it forwards)
    [BACK] "hello" 52 ms, same   a line came back the other way, the same as sent
    [ERROR] … different         it came back changed;  [ERROR] … not back in …: lost
"""


USB_BAUD = 1000000      # MONITOR_BAUD of the programs
BT_BAUD = 9600          # matters only if --bt is a USB serial adapter, not a Bluetooth port
LINE_MAX = 60           # LINE_MAX of the programs: longer lines are cut by the board
READY_S = 20            # the board's AT commands (HC-06: about a second each) before "[INFO] ready"
BACK_S = 2.0            # a line not back by then is lost
FROM_BLUETOOTH = "[FROM BLUETOOTH] "


class RoundTrip:
    # the lines on their way: sent text and time, per direction, oldest first
    def __init__(self, usb, bt):
        self.usb = usb
        self.bt = bt
        self.lock = threading.Lock()
        self.waiting = {"usb": collections.deque(), "bt": collections.deque()}
        self.same = 0
        self.different = 0
        self.lost = 0
        self.times_ms = []
        self.stop = threading.Event()

    def send(self, text: str, way: str):
        # way "usb": USB -> board -> Bluetooth; way "bt": Bluetooth -> board -> USB
        data = text.encode("utf-8")
        if len(data) > LINE_MAX:
            print(f"[WARNING] round_trip.py: {len(data)} bytes, the board cuts at {LINE_MAX}")
        with self.lock:
            self.waiting[way].append((text, time.monotonic()))
        if way == "usb":
            self.usb.write(data + b"\n")
        else:
            self.bt.write(encode(ord("T"), data))

    def back(self, text: str, way: str):
        # a line arrived at the other end of `way`
        now = time.monotonic()
        with self.lock:
            if not self.waiting[way]:
                print(f'[BACK] "{text}" (not sent by round_trip.py)')
                return
            sent, at = self.waiting[way].popleft()
        ms = (now - at) * 1000
        sent_cut = sent.encode("utf-8")[:LINE_MAX].decode("utf-8", "ignore")
        if text == sent_cut:
            self.same += 1
            self.times_ms.append(ms)
            print(f'[BACK] "{text}" {ms:.0f} ms, same')
        else:
            self.different += 1
            print(f'[ERROR] round_trip.py: sent "{sent}", back "{text}" ({ms:.0f} ms): different')

    def check_lost(self):
        # lines not back in BACK_S: lost
        now = time.monotonic()
        with self.lock:
            for way in self.waiting:
                while self.waiting[way] and now - self.waiting[way][0][1] > BACK_S:
                    sent, _ = self.waiting[way].popleft()
                    self.lost += 1
                    print(f'[ERROR] round_trip.py: "{sent}" not back in {BACK_S} s: lost')

    def pending(self) -> int:
        with self.lock:
            return len(self.waiting["usb"]) + len(self.waiting["bt"])


def read_usb(trip: RoundTrip):
    # the board's log; "[FROM BLUETOOTH] text" ends a trip Bluetooth -> USB
    buffer = b""
    while not trip.stop.is_set():
        try:
            buffer += trip.usb.read(trip.usb.in_waiting or 1)
        except serial.SerialException as error:
            print(f"[ERROR] round_trip.py: the USB port is gone: {error}")
            trip.stop.set()
            return
        while b"\n" in buffer:
            raw, buffer = buffer.split(b"\n", 1)
            line = raw.rstrip(b"\r").decode("utf-8", "replace")
            if not line:
                continue
            print(f"[BOARD] {line}")
            if line.startswith(FROM_BLUETOOTH):
                trip.back(line[len(FROM_BLUETOOTH):], "bt")


def read_bt(trip: RoundTrip):
    # packets from the board over Bluetooth; 'T' ends a trip USB -> Bluetooth
    decoder = Decoder()
    while not trip.stop.is_set():
        try:
            data = trip.bt.read(trip.bt.in_waiting or 1)
        except (serial.SerialException, ConnectionError, OSError) as error:
            print(f"[ERROR] round_trip.py: Bluetooth is gone: {error}")
            trip.stop.set()
            return
        dropped = decoder.dropped
        for command, payload in decoder.feed(data):
            if command == ord("T"):
                trip.back(payload.decode("utf-8", "replace"), "usb")
            else:
                print(f"[BLUETOOTH] packet 0x{command:02X}, {len(payload)} bytes")
        if decoder.dropped != dropped:
            print(f"[WARNING] round_trip.py: {decoder.dropped - dropped} broken packet(s) from Bluetooth skipped")
        trip.check_lost()


def find_usb_port() -> str:
    # the one USB serial port that is not Bluetooth
    ports = [p for p in list_ports.comports() if p.vid is not None and not is_bluetooth(p)]
    if len(ports) == 1:
        return ports[0].device
    return ""


def list_serial_ports():
    for port in sorted(list_ports.comports(), key=lambda p: p.device):
        kind = "Bluetooth" if is_bluetooth(port) else "USB      " if port.vid is not None else "         "
        print(f"[INFO] {kind}  {port.device}  {port.description}")


def wait_ready(usb):
    # opening the USB port resets the board: its AT commands first, then "[INFO] ready"
    print("[INFO] the board restarts (opening USB resets it): waiting for its AT commands")
    end = time.monotonic() + READY_S
    buffer = b""
    while time.monotonic() < end:
        buffer += usb.read(usb.in_waiting or 1)
        while b"\n" in buffer:
            raw, buffer = buffer.split(b"\n", 1)
            line = raw.rstrip(b"\r").decode("utf-8", "replace")
            if line:
                print(f"[BOARD] {line}")
            if line.startswith("[INFO] ready"):
                return True
    print(f"[WARNING] round_trip.py: no \"[INFO] ready\" in {READY_S} s (is the right program on the board?)")
    return False


def random_line(n: int) -> str:
    letters = "".join(random.choice(string.ascii_letters + string.digits) for _ in range(random.randint(5, 40)))
    return f"{n:04d} {letters}"


def run_count(trip: RoundTrip, count: int):
    # count lines USB -> Bluetooth, each after the last one is back (or lost); the summary counts only these
    start = (trip.same, trip.different, trip.lost, len(trip.times_ms))
    for n in range(1, count + 1):
        if trip.stop.is_set():
            break
        trip.send(random_line(n), "usb")
        end = time.monotonic() + BACK_S + 0.5
        while trip.pending() and time.monotonic() < end:
            time.sleep(0.005)
            trip.check_lost()
    time.sleep(0.2)
    times = trip.times_ms[start[3]:]
    print(f"[INFO] {count} sent: {trip.same - start[0]} same, {trip.different - start[1]} different, "
          f"{trip.lost - start[2]} lost")
    if times:
        print(f"[INFO] round trip: {min(times):.0f} ~ {max(times):.0f} ms, average {sum(times) / len(times):.0f} ms")


def main():
    parser = argparse.ArgumentParser(description="a round trip USB -> UNO -> Bluetooth (and back the other way)")
    parser.add_argument("--list", action="store_true", help="list the serial ports and end")
    way = parser.add_mutually_exclusive_group()
    way.add_argument("--bt", help="classic Bluetooth (HC-05, HC-06): the serial port, /dev/cu.UnoKits, COM5")
    way.add_argument("--ble", help="BLE (BT05 / HM-10 type): the module's name, e.g. TanksUno")
    parser.add_argument("--usb", help="the board's USB port (found by itself when there is only one)")
    parser.add_argument("--count", type=int, default=0, help="send this many lines by itself, then a summary")
    args = parser.parse_args()
    if serial is None:
        parser.error("pyserial is missing: pip install pyserial")
    if args.list:
        list_serial_ports()
        return
    if not args.bt and not args.ble:
        parser.error("--bt PORT (classic: --list shows the ports) or --ble NAME (BLE) is needed")
    usb_port = args.usb or find_usb_port()
    if not usb_port:
        parser.error("the board's USB port was not found: give --usb (--list shows the ports)")

    try:
        usb = serial.Serial(usb_port, USB_BAUD, timeout=0.05)
    except serial.SerialException as error:
        print(f"[ERROR] round_trip.py: cannot open {usb_port}: {error}")
        print("    pio device monitor still open?")
        sys.exit(1)
    wait_ready(usb)
    try:
        if args.ble:
            print(f'[INFO] looking for the BLE device "{args.ble}" and connecting')
            bt = BleSerial(args.ble)
        else:
            print(f"[INFO] opening {args.bt} (this connects Bluetooth)")
            bt = serial.Serial(args.bt, BT_BAUD, timeout=0.05)
    except ImportError as error:
        print(f"[ERROR] round_trip.py: {error}")
        usb.close()
        sys.exit(1)
    except (serial.SerialException, ConnectionError, OSError) as error:
        print(f"[ERROR] round_trip.py: cannot connect: {error}")
        print("    classic: paired? (Windows: the Outgoing COM port); BLE: the name, Bluetooth allowed for this terminal?")
        usb.close()
        sys.exit(1)

    trip = RoundTrip(usb, bt)
    for target in (read_usb, read_bt):
        threading.Thread(target=target, args=(trip,), daemon=True).start()
    time.sleep(1.5)   # the board's STATE filter: connected after CONNECTED_MS
    print("[SUCCESS] both open: type a line (USB -> Bluetooth), or \"< line\" (Bluetooth -> USB); Ctrl+C ends")

    try:
        if args.count > 0:
            run_count(trip, args.count)
        else:
            while not trip.stop.is_set():
                text = input()
                if text.startswith("<"):
                    text = text[1:].strip()
                    if text:
                        trip.send(text, "bt")
                elif text.strip():
                    trip.send(text, "usb")
    except (KeyboardInterrupt, EOFError):
        pass
    trip.stop.set()
    time.sleep(0.2)
    usb.close()
    bt.close()


if __name__ == "__main__":
    main()
