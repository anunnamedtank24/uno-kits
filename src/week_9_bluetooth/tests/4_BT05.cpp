#include <Arduino.h>
#include "Uno_serial_buffer.h"
#include "Uno_pin.h"
#include "Uno_soft_serial.h"
#include "bluetooth.h"

// UNO + BT05-type BLE module (HM-10 family, "+UUID=FFE0"): a bridge between the USB serial and BLE, for a round trip
//   python src/week_9_bluetooth/tests/round_trip.py --ble NAME      (no pairing: BLE connects by name)
//   a line from USB            -> packet 'T' over BLE -> the computer (notifications of FFE1)
//   packet 'T' from Bluetooth  -> "[FROM BLUETOOTH] text" on USB
//   (pio device monitor works too: a line typed there goes out over Bluetooth)
// at power on, before anything connects: name and PIN by AT commands, every result on USB, then "[INFO] ready"
// EN is the module's power switch on this board (3_diagnose.cpp: "EN is its power switch"): kept HIGH
// the module is on two other pins (a software UART), so D0 / D1 stay USB: upload with it connected

// ---------- settings ----------

// wiring: BT05 board VCC -> 5V, GND -> GND
constexpr uint8_t BT_RX_PIN = A0;           // <- the module's TXD: any pin (3.3 V: read as HIGH); its group's pin
                                            // change interrupt (A0 ~ A5): no other interrupt pin in that group
constexpr uint8_t BT_TX_PIN = A1;           // -> the module's RXD through 1 kΩ, with 2 kΩ from RXD to GND (3.3 V)
constexpr uint8_t EN_PIN = 12;              // -> EN (the power switch): any pin, through a divider like RXD
constexpr uint8_t STATE_PIN = 4;            // <- STATE: any pin, directly (3.3 V out: read as HIGH, no divider)
constexpr uint32_t BT_BAUD = 9600;          // the module's data baud ("AT+BAUD" -> 4: 9600)
constexpr uint32_t MONITOR_BAUD = 1000000;  // USB serial: the same as monitor_speed in platformio.ini

constexpr const char *NAME = "TanksUno";     // the name it advertises: 1 ~ 18 characters
constexpr const char *PAIRING_PIN = "123456"; // 6 digits (only apps that pair use it)
constexpr uint16_t USB_RX_BUFFER = 128;     // USB bytes kept while the board sends over Bluetooth (~1 ms a byte)
constexpr uint16_t USB_TX_BUFFER = 64;      // USB bytes to send: printing returns at once while they fit
constexpr uint16_t BT_RX_BUFFER = 64;       // Bluetooth bytes kept by the interrupt while loop() is busy
constexpr uint8_t LINE_MAX = 60;            // the longest line forwarded, bytes (longer: cut)
constexpr uint8_t PACKET_MAX_LENGTH = 64;   // the longest DATA of a packet from Bluetooth (RAM)
constexpr uint16_t PACKET_GAP_MS = 100;     // a packet whose next byte takes longer is dropped
constexpr bool SEND_ONLY_IF_CONNECTED = false; // true: lines go out only while STATE says connected (only when
                                            // STATE works: the "[INFO] STATE pin" lines show it going HIGH on connecting)
constexpr uint16_t STATE_PRINT_MS = 200;    // the raw STATE pin printed at most this often

// ---------- parts ----------

using Monitor = Uno::UnoSerial<MONITOR_BAUD, USB_RX_BUFFER, USB_TX_BUFFER>;
using BtLink = Uno::UnoSoftSerial<BT_RX_PIN, BT_TX_PIN, BT_BAUD, BT_RX_BUFFER>;
using En = Uno::UnoOutputPin<EN_PIN>;
using State = Uno::UnoInputPin<STATE_PIN>;
using Module = bluetooth::BT05<BtLink, State, En>;

bluetooth::Packet<Module, PACKET_MAX_LENGTH, PACKET_GAP_MS> packets;

// ---------- monitor ----------

bool wasConnected = false;
bool shownStateLevel = false;
uint32_t shownStateMs = 0;
uint16_t shownDropped = 0;
uint16_t shownBroken = 0;
uint16_t shownLost = 0;

const char *resultText(uint8_t result) {
    switch (result) {
        case Module::OK:
            return "OK";
        case Module::ERROR_NO_ANSWER:
            return "no answer";
        case Module::ERROR_REFUSED:
            return "refused";
        case Module::ERROR_BAD_VALUE:
            return "bad value";
        default:
            return "connected";
    }
}

void printResult(const char *what, uint8_t result) {
    Monitor::printText(result == Module::OK ? "[INFO] " : "[ERROR] ");
    Monitor::printText(what);
    Monitor::printText(": ");
    Monitor::printText(resultText(result));
    Monitor::printText("\r\n");
}

void printBytes(const uint8_t *data, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        Monitor::write8(data[i]);
    }
}

// a line only when one of the counts changed
void printErrors() {
    uint16_t dropped = packets.dropped();
    uint16_t broken = BtLink::brokenCount();
    uint16_t lost = BtLink::lostCount() + Monitor::lostCount();
    if (dropped == shownDropped && broken == shownBroken && lost == shownLost) {
        return;
    }
    shownDropped = dropped;
    shownBroken = broken;
    shownLost = lost;
    Monitor::printText("[WARNING] packets dropped ");
    Monitor::printNumber(dropped);
    Monitor::printText(", bytes broken ");
    Monitor::printNumber(broken);
    Monitor::printText(", bytes lost ");
    Monitor::printNumber(lost);
    Monitor::printText("\r\n");
}

// ---------- bridge ----------

uint8_t line[LINE_MAX];
uint8_t lineLength = 0;
bool lineCut = false;

// the line from USB -> packet 'T' over Bluetooth
void forwardLine() {
    if (lineLength == 0) {
        return;
    }
    if (SEND_ONLY_IF_CONNECTED && !Module::isConnected()) {
        Monitor::printText("[ERROR] not connected (STATE LOW): not sent: ");
        printBytes(line, lineLength);
        Monitor::printText("\r\n");
        return;
    }
    packets.send('T', line, lineLength);
    Monitor::printText("[TO BLUETOOTH] ");
    printBytes(line, lineLength);
    Monitor::printText(lineCut ? "   (cut at LINE_MAX)\r\n" : "\r\n");
}

// USB bytes -> lines ("\r\n" or "\n" ends one)
void readUsb() {
    uint8_t byte;
    while (Monitor::available() && Monitor::read8(byte, 0)) {
        if (byte == '\r') {
            continue;
        }
        if (byte == '\n') {
            forwardLine();
            lineLength = 0;
            lineCut = false;
        } else if (lineLength < LINE_MAX) {
            line[lineLength] = byte;
            lineLength++;
        } else {
            lineCut = true;
        }
    }
}

// a packet from Bluetooth -> USB
void showPacket() {
    if (packets.command() == 'T') {
        Monitor::printText("[FROM BLUETOOTH] ");
        printBytes(packets.data(), packets.length());
        Monitor::printText("\r\n");
        return;
    }
    Monitor::printText("[FROM BLUETOOTH] packet ");
    Monitor::printHex(packets.command());
    Monitor::printText(", ");
    Monitor::printNumber(packets.length());
    Monitor::printText(" bytes (only 'T' is text)\r\n");
}

// ---------- main ----------

void setup() {
    Monitor::init();
    Monitor::printText("\r\n[INFO] BT05 (BLE): module TXD -> ");
    Monitor::printPin(BT_RX_PIN);
    Monitor::printText(", RXD <- ");
    Monitor::printPin(BT_TX_PIN);
    Monitor::printText(", EN (power) <- ");
    Monitor::printPin(EN_PIN);
    Monitor::printText(", STATE -> ");
    Monitor::printPin(STATE_PIN);
    Monitor::printText(", ");
    Monitor::printNumber(BT_BAUD);
    Monitor::printText(" baud\r\n");

    packets.init();                         // Module::init() too: EN HIGH (the module on), the software UART, STATE
    printResult("AT", Module::check());
    printResult("AT+NAME", Module::setName(NAME));
    printResult("AT+PIN", Module::setPin(PAIRING_PIN));
    char text[24];
    if (Module::name(text, sizeof(text)) == Module::OK) {
        Monitor::printText("[INFO] name ");
        Monitor::printText(text);
        Monitor::printText("\r\n");
    }
    if (Module::version(text, sizeof(text)) == Module::OK) {
        Monitor::printText("[INFO] firmware ");
        Monitor::printText(text);
        Monitor::printText("\r\n");
    }
    if (Module::address(text, sizeof(text)) == Module::OK) {
        Monitor::printText("[INFO] address ");
        Monitor::printText(text);
        Monitor::printText("\r\n");
    }
    Monitor::printText("[INFO] ready: connect to \"");
    Monitor::printText(NAME);
    Monitor::printText("\" over BLE (round_trip.py --ble), then type lines here\r\n");
}

void loop() {
    bool connected = Module::isConnected();
    if (connected != wasConnected) {
        Monitor::printText(connected ? "[INFO] connected\r\n" : "[INFO] disconnected\r\n");
    }
    wasConnected = connected;
    // the pin itself, to see whether STATE follows the connection at all
    bool level = Module::readState();
    if (level != shownStateLevel && Module::nowMs() - shownStateMs >= STATE_PRINT_MS) {
        Monitor::printText(level ? "[INFO] STATE pin HIGH\r\n" : "[INFO] STATE pin LOW\r\n");
        shownStateLevel = level;
        shownStateMs = Module::nowMs();
    }

    readUsb();
    if (packets.update()) {
        showPacket();
    }
    printErrors();
}
