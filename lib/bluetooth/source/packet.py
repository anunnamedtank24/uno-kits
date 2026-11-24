import sys
sys.dont_write_bytecode = True # importing ble.py (--ble) must not leave __pycache__

import argparse
import struct
import threading
import time
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None


"""
Func: the computer's side of lib/bluetooth/tool/packet.h: sends packets to the board and prints the ones it sends
      back, over the serial port of a paired Bluetooth module (or the USB serial: the same packets)
Usage:
    pip install pyserial
    python lib/bluetooth/source/packet.py --list                         the serial ports (Bluetooth ones marked)
    python lib/bluetooth/source/packet.py --port /dev/cu.HC-05           (Windows: --port COM5, the "Outgoing" one;
                                                                          Linux: /dev/rfcomm0 after rfcomm bind)
    python lib/bluetooth/source/packet.py --port COM5 --baud 9600        --baud matters only for a USB serial
    python lib/bluetooth/source/packet.py --ble TanksUno                 a BLE module (BT05 / HM-10 type) by its name
                                                                         (pip install bleak; see ble.py)
    then type one packet a line, Enter sends it; Ctrl+C or an empty line + Ctrl+D ends
Typing a packet: COMMAND [VALUE ...]
    COMMAND   one character (L) or a number 0 ~ 255
    VALUE     200        one byte 0 ~ 255           -5        one byte -128 ~ -1
              h:-100     two bytes, int16           H:500     two bytes, uint16      (low first, like the board)
              s:hello    the text's bytes
    e.g.  L 1          M h:200 h:-100          E s:ping
Pairing first (once): the computer's Bluetooth settings, the module's name, PIN 1234 (or 0000)
    macOS then has /dev/cu.<name>; Windows two COM ports ("Standard Serial over Bluetooth link"): the Outgoing one
Packet (see lib/bluetooth/README.md):
    0xAA, LENGTH, COMMAND, DATA (LENGTH bytes), CRC = CRC-8 (polynomial 0x07, start 0) of LENGTH, COMMAND, DATA
"""


START = 0xAA
BLUETOOTH_WORDS = ("bluetooth", "hc-05", "hc-06", "hc05", "hc06", "rfcomm", "linvor")


def crc8(data: bytes, crc: int = 0) -> int:
    # CRC-8, polynomial 0x07, start 0, no reflection (the same as crc8() of packet.h)
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


def encode(command: int, data: bytes = b"") -> bytes:
    # one packet: START, LENGTH, COMMAND, DATA, CRC
    if not 0 <= command <= 255:
        raise ValueError(f"command {command} is not 0 ~ 255")
    if len(data) > 255:
        raise ValueError(f"{len(data)} bytes of data, at most 255")
    body = bytes([len(data), command]) + bytes(data)
    return bytes([START]) + body + bytes([crc8(body)])


class Decoder:
    # bytes in, whole packets out; a wrong CRC is counted in dropped and skipped
    def __init__(self):
        self.buffer = bytearray()
        self.dropped = 0

    def feed(self, data: bytes) -> list:
        # every whole packet in what came so far: [(command, data), ...]
        self.buffer += data
        packets = []
        while True:
            start = self.buffer.find(START)
            if start < 0:
                self.buffer.clear()
                return packets
            del self.buffer[:start]
            if len(self.buffer) < 4 or len(self.buffer) < 4 + self.buffer[1]:
                return packets
            length = self.buffer[1]
            command = self.buffer[2]
            data = bytes(self.buffer[3:3 + length])
            if crc8(bytes(self.buffer[1:3 + length])) == self.buffer[3 + length]:
                packets.append((command, data))
                del self.buffer[:4 + length]
            else:
                # not a packet: look for the next START after this one
                self.dropped += 1
                del self.buffer[:1]


def parse_line(line: str) -> bytes:
    # "M h:200 h:-100" -> one packet
    words = line.split()
    if not words:
        raise ValueError("empty")
    head = words[0]
    if len(head) == 1 and not head.isdigit():
        command = ord(head)
    else:
        command = int(head, 0)
    data = bytearray()
    for word in words[1:]:
        if word.startswith("s:"):
            data += word[2:].encode("utf-8")
        elif word.startswith("h:"):
            data += struct.pack("<h", int(word[2:], 0))
        elif word.startswith("H:"):
            data += struct.pack("<H", int(word[2:], 0))
        else:
            value = int(word, 0)
            if not -128 <= value <= 255:
                raise ValueError(f"{word} is not one byte (-128 ~ 255): use h: or H: for two bytes")
            data.append(value & 0xFF)
    return encode(command, bytes(data))


def describe(command: int, data: bytes) -> str:
    # "'S' 5 bytes: 68 65 6C 6C 6F  "hello""
    name = f"'{chr(command)}'" if 32 <= command < 127 else f"0x{command:02X}"
    text = f"{name} {len(data)} byte{'' if len(data) == 1 else 's'}"
    if data:
        text += ": " + " ".join(f"{b:02X}" for b in data)
        if all(32 <= b < 127 for b in data):
            text += f'  "{data.decode("ascii")}"'
    return text


def is_bluetooth(port) -> bool:
    words = f"{port.device} {port.description} {port.hwid}".lower()
    return any(word in words for word in BLUETOOTH_WORDS)


def list_serial_ports():
    ports = sorted(list_ports.comports(), key=lambda port: port.device)
    if not ports:
        print("[INFO] no serial ports (pair the module in the computer's Bluetooth settings first)")
        return
    for port in ports:
        mark = "Bluetooth" if is_bluetooth(port) else "         "
        print(f"[INFO] {mark}  {port.device}  {port.description}")


def receive(link, decoder: Decoder, stop: threading.Event):
    # prints every packet the board sends
    while not stop.is_set():
        try:
            data = link.read(link.in_waiting or 1)
        except (serial.SerialException, ConnectionError, OSError) as error:
            print(f"[ERROR] packet.py: the connection is gone: {error}")
            stop.set()
            return
        dropped = decoder.dropped
        for command, payload in decoder.feed(data):
            print(f"[BOARD] {describe(command, payload)}")
        if decoder.dropped != dropped:
            print(f"[WARNING] packet.py: {decoder.dropped - dropped} broken packet(s) skipped")


def main():
    parser = argparse.ArgumentParser(description="send and receive the packets of lib/bluetooth/tool/packet.h")
    parser.add_argument("--list", action="store_true", help="list the serial ports and end")
    parser.add_argument("--port", help="the serial port: /dev/cu.HC-05, COM5, /dev/rfcomm0")
    parser.add_argument("--ble", help="a BLE module (BT05 / HM-10 type) by its name instead of --port")
    parser.add_argument("--baud", type=int, default=9600,
                        help="the baud: matters only for a USB serial (default 9600, the modules' default)")
    args = parser.parse_args()
    if serial is None:
        parser.error("pyserial is missing: pip install pyserial")
    if args.list:
        list_serial_ports()
        return
    if not args.port and not args.ble:
        parser.error("--port (python lib/bluetooth/source/packet.py --list shows them) or --ble NAME is needed")

    try:
        if args.ble:
            sys.path.insert(0, str(Path(__file__).resolve().parent))
            from ble import BleSerial
            print(f'[INFO] looking for the BLE device "{args.ble}" and connecting')
            link = BleSerial(args.ble, timeout=0.1)
        else:
            print(f"[INFO] opening {args.port} (the module's LED changes when it connects)")
            link = serial.Serial(args.port, args.baud, timeout=0.1)
    except ImportError as error:
        print(f"[ERROR] packet.py: {error}")
        sys.exit(1)
    except (serial.SerialException, ConnectionError, OSError) as error:
        print(f"[ERROR] packet.py: cannot connect: {error}")
        print("    paired? another program using it? (Windows: the Outgoing COM port); BLE: the name?")
        sys.exit(1)
    print("[SUCCESS] connected: type COMMAND [VALUE ...], e.g. L 1   (Ctrl+C ends)")

    stop = threading.Event()
    reader = threading.Thread(target=receive, args=(link, Decoder(), stop), daemon=True)
    reader.start()
    try:
        while not stop.is_set():
            line = input()
            if not line.strip():
                continue
            try:
                packet = parse_line(line)
            except ValueError as error:
                print(f"[ERROR] packet.py: {error}")
                continue
            link.write(packet)
            print(f"[INFO] sent {' '.join(f'{b:02X}' for b in packet)}")
    except (KeyboardInterrupt, EOFError):
        pass
    stop.set()
    time.sleep(0.2)
    link.close()


if __name__ == "__main__":
    main()
