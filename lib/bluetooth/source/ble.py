import argparse
import asyncio
import threading
import time

try:
    from bleak import BleakClient, BleakScanner
except ImportError:
    BleakClient = None


"""
Func: a BLE serial module (BT05 / HM-10 family: service FFE0, characteristic FFE1) used like a serial port,
      so the same scripts work over BLE: BleSerial has read(), write(), in_waiting, close() as pyserial has
      the module's TXD -> notifications of FFE1 -> read();  write() -> FFE1 -> the module's RXD
Usage:
    pip install bleak
    python lib/bluetooth/source/ble.py --scan          the BLE devices around (FFE0 ones marked)
    in a script:
        from ble import BleSerial
        link = BleSerial("TanksUno")                   finds it by name and connects (ConnectionError if not)
        link.write(b"hello")                           pieces of 20 bytes, paced for a 9600 baud UART
        data = link.read(64)                           what has come, waits up to timeout
        link.close()
macOS: the first time, allow Bluetooth for the terminal app (Terminal, iTerm, VS Code) when asked
       (System Settings -> Privacy & Security -> Bluetooth); without it nothing is ever found
no pairing: BLE connects by name; a module connected to a phone does not advertise (disconnect it there first)
"""


SERVICE = "0000ffe0-0000-1000-8000-00805f9b34fb"
CHARACTERISTIC = "0000ffe1-0000-1000-8000-00805f9b34fb"
CHUNK = 20              # bytes a write (the BLE default)
CHUNK_GAP_S = 0.025     # between two writes: 20 bytes take ~21 ms at 9600 baud on the module's UART
SCAN_S = 10.0


class BleSerial:
    # a serial-port-like link over FFE1; a background thread runs bleak's event loop
    def __init__(self, name: str, timeout: float = 0.05, scan_s: float = SCAN_S):
        if BleakClient is None:
            raise ImportError("bleak is missing: pip install bleak")
        self.timeout = timeout
        self.name = name
        self._buffer = bytearray()
        self._lock = threading.Lock()
        self._arrived = threading.Event()
        self._connected = False
        self._loop = asyncio.new_event_loop()
        self._thread = threading.Thread(target=self._loop.run_forever, daemon=True)
        self._thread.start()
        try:
            self._client, self._response = self._run(self._connect(name, scan_s), scan_s + 20)
        except BaseException:
            self._stop_loop()
            raise

    def _run(self, coroutine, timeout=None):
        return asyncio.run_coroutine_threadsafe(coroutine, self._loop).result(timeout)

    async def _connect(self, name: str, scan_s: float):
        device = await BleakScanner.find_device_by_name(name, timeout=scan_s)
        if device is None:
            raise ConnectionError(f'no BLE device named "{name}" in {scan_s:.0f} s '
                                  "(powered? connected to a phone? Bluetooth allowed for this terminal?)")
        client = BleakClient(device, disconnected_callback=self._on_disconnect)
        await client.connect()
        characteristic = client.services.get_characteristic(CHARACTERISTIC)
        if characteristic is None:
            await client.disconnect()
            raise ConnectionError(f'"{name}" has no characteristic FFE1: not a BT05 / HM-10 type module')
        await client.start_notify(CHARACTERISTIC, self._on_notify)
        self._connected = True
        # write without response when it can (faster); with response otherwise
        response = "write-without-response" not in characteristic.properties
        return client, response

    def _on_notify(self, _sender, data: bytearray):
        with self._lock:
            self._buffer += data
        self._arrived.set()

    def _on_disconnect(self, _client):
        self._connected = False
        self._arrived.set()

    @property
    def in_waiting(self) -> int:
        with self._lock:
            return len(self._buffer)

    def read(self, size: int = 1) -> bytes:
        # up to size bytes that have come; waits up to timeout when none has
        if not self.in_waiting:
            if not self._connected:
                raise ConnectionError(f'"{self.name}" disconnected')
            self._arrived.wait(self.timeout)
        with self._lock:
            data = bytes(self._buffer[:size])
            del self._buffer[:size]
            if not self._buffer:
                self._arrived.clear()
        return data

    def write(self, data: bytes) -> int:
        if not self._connected:
            raise ConnectionError(f'"{self.name}" disconnected')
        for start in range(0, len(data), CHUNK):
            if start:
                time.sleep(CHUNK_GAP_S)
            self._run(self._client.write_gatt_char(CHARACTERISTIC, data[start:start + CHUNK],
                                                   response=self._response), 5)
        return len(data)

    def close(self):
        try:
            if self._connected:
                self._run(self._client.disconnect(), 5)
        except Exception:
            pass
        self._stop_loop()

    def _stop_loop(self):
        self._loop.call_soon_threadsafe(self._loop.stop)
        self._thread.join(2)


async def _scan(seconds: float):
    found = await BleakScanner.discover(timeout=seconds, return_adv=True)
    rows = []
    for device, adv in found.values():
        name = adv.local_name or device.name or ""
        uuids = [u.lower() for u in (adv.service_uuids or [])]
        rows.append((SERVICE in uuids, name, adv.rssi, device.address))
    return sorted(rows, key=lambda row: (not row[0], row[1] == "", -row[2]))


def scan(seconds: float = SCAN_S):
    # prints the BLE devices around; FFE0 (BT05 / HM-10 type) first
    if BleakClient is None:
        raise ImportError("bleak is missing: pip install bleak")
    rows = asyncio.run(_scan(seconds))
    if not rows:
        print("[INFO] no BLE device found (Bluetooth on? allowed for this terminal app?)")
    for ffe0, name, rssi, address in rows:
        mark = "FFE0" if ffe0 else "    "
        print(f"[INFO] {mark}  {name or '-':20}  {rssi:4} dBm  {address}")


def main():
    parser = argparse.ArgumentParser(description="BLE serial modules (FFE0 / FFE1) like a serial port")
    parser.add_argument("--scan", action="store_true", help="list the BLE devices around")
    parser.add_argument("--seconds", type=float, default=SCAN_S, help="how long to scan (default 10)")
    args = parser.parse_args()
    if BleakClient is None:
        parser.error("bleak is missing: pip install bleak")
    if not args.scan:
        parser.error("nothing to do: --scan (or import BleSerial in a script)")
    print(f"[INFO] scanning {args.seconds:.0f} s")
    scan(args.seconds)


if __name__ == "__main__":
    main()
