#ifndef BLUETOOTH_H
#define BLUETOOTH_H

// Bluetooth serial modules (SPP: a UART on one side, a serial port of the computer / phone on the other),
// entry point: include this file only
//   port.h     bluetooth::Port<Link, Key, State>: the UART plus EN / KEY (AT command mode) and STATE (connected),
//              either one may be NoPin; reading lines and answers of AT commands
// module/  the parts you buy
//   HC_05.h    HC05<Link, Key, State>  EN, VCC, GND, TXD, RXD, STATE; AT commands through EN, master or slave
//   HC_06.h    HC06<Link, State>       VCC, GND, TXD, RXD (STATE); no EN / KEY: AT commands only while unconnected
//   BT05.h     BT05<Link, State, Power> BLE (the HM-10 family): the same UART side, GATT FFE0 / FFE1 on the radio
// tool/  what is often built on it
//   packet.h   Packet<Link, MAX_LENGTH>: 0xAA, LENGTH, COMMAND, DATA, CRC-8 over the byte stream
// source/  the computer's side
//   packet.py  sends and receives the same packets over the Bluetooth serial port (or BLE: --ble NAME)
//   ble.py     a BLE module (FFE1) used like a serial port, with bleak
#include "port.h"
#include "module/HC_05.h"
#include "module/HC_06.h"
#include "module/BT05.h"
#include "tool/packet.h"

#endif
