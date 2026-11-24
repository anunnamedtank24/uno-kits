#ifndef BLUETOOTH_PACKET_H
#define BLUETOOTH_PACKET_H

#include <stdint.h>

namespace bluetooth {
    // CRC-8, polynomial x^8 + x^2 + x + 1 (0x07), start 0, no reflection: crc = crc8(crc, byte) for each byte
    inline uint8_t crc8(uint8_t crc, uint8_t byte) {
        crc ^= byte;
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
        return crc;
    }

    // packets over a byte stream: where one starts and ends, and whether it arrived whole
    // (Bluetooth serial, like any UART, gives only bytes: a packet may arrive in pieces or two may stick together)
    //
    //   ┌──────┬────────┬─────────┬──────────────────┬──────┐
    //   │ 0xAA │ LENGTH │ COMMAND │ DATA (LENGTH)     │ CRC  │
    //   └──────┴────────┴─────────┴──────────────────┴──────┘
    //    START   0 ~ 255  any byte  numbers: little      CRC-8 (polynomial 0x07, start 0) of LENGTH, COMMAND
    //                     ('L', 'M')  endian (low first)  and every DATA byte
    //
    // against noise: the CRC catches every error of 1 ~ 2 bits and every burst up to 8 bits (a sum misses
    // two errors that cancel); a broken packet is dropped and the next START begins again; a packet whose
    // bytes stop coming (GAP_MS) is dropped, so one lost byte never swallows the next packet
    //
    // the computer's side: lib/bluetooth/source/packet.py (the same format)
    //
    // Link: anything with available(), read8(byte, timeoutMs), write8(byte), nowMs(), init():
    // bluetooth::HC05, HC06, Port, or Uno::UnoSerial<BAUD> directly (the same packets over USB)
    //
    // MAX_LENGTH: the longest DATA taken (RAM: MAX_LENGTH bytes); a longer packet is dropped
    // GAP_MS: a packet whose next byte takes longer than this is dropped (a lost byte never blocks the next ones)
    template <class Link, uint8_t MAX_LENGTH = 32, uint16_t GAP_MS = 100>
    class Packet {
        static_assert(MAX_LENGTH >= 1, "bluetooth::Packet: MAX_LENGTH must be 1 ~ 255");

    public:
        static constexpr uint8_t START = 0xAA;

        // Link::init()
        inline void init();

        // reads what has arrived (never waits); true when a whole packet with the right CRC is in:
        // command(), length(), data() are then good until the next update()
        // call it often: without a receive buffer (Uno::UnoSerial<BAUD> polling) the UNO keeps only 2 bytes
        inline bool update();

        inline uint8_t command() const;
        inline uint8_t length() const;
        inline const uint8_t *data() const;

        // DATA from index: one byte, or two bytes low first; 0 beyond length()
        inline uint8_t readUint8(uint8_t index) const;
        inline int8_t readInt8(uint8_t index) const;
        inline uint16_t readUint16(uint8_t index) const;
        inline int16_t readInt16(uint8_t index) const;

        // packets dropped since init(): wrong CRC, longer than MAX_LENGTH, or cut off (GAP_MS)
        inline uint16_t dropped() const;

        // sends one packet (waits until every byte is sent)
        inline void send(uint8_t command, const uint8_t *data = nullptr, uint8_t length = 0);

        // sends the text's bytes as DATA (without '\0'; cut at 255)
        inline void sendText(uint8_t command, const char *text);

    private:
        static constexpr uint8_t WAIT_START = 0;
        static constexpr uint8_t WAIT_LENGTH = 1;
        static constexpr uint8_t WAIT_COMMAND = 2;
        static constexpr uint8_t WAIT_DATA = 3;
        static constexpr uint8_t WAIT_CRC = 4;

        uint8_t _state = WAIT_START;
        uint8_t _length = 0;
        uint8_t _command = 0;
        uint8_t _count = 0;
        uint8_t _crc = 0;
        uint16_t _dropped = 0;
        uint32_t _lastMs = 0;
        uint8_t _data[MAX_LENGTH];

        inline void drop();
    };

    // ================= definitions =================

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline void Packet<Link, MAX_LENGTH, GAP_MS>::init() {
        Link::init();
        _state = WAIT_START;
        _length = 0;
        _dropped = 0;
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline bool Packet<Link, MAX_LENGTH, GAP_MS>::update() {
        if (_state != WAIT_START && Link::nowMs() - _lastMs > GAP_MS) {
            drop();
        }
        while (Link::available()) {
            uint8_t byte;
            if (!Link::read8(byte, 0)) {
                break;
            }
            _lastMs = Link::nowMs();
            switch (_state) {
                case WAIT_START:
                    if (byte == START) {
                        _state = WAIT_LENGTH;
                    }
                    break;
                case WAIT_LENGTH:
                    if (byte > MAX_LENGTH) {
                        drop();
                        break;
                    }
                    _length = byte;
                    _crc = crc8(0, byte);
                    _state = WAIT_COMMAND;
                    break;
                case WAIT_COMMAND:
                    _command = byte;
                    _crc = crc8(_crc, byte);
                    _count = 0;
                    _state = _length > 0 ? WAIT_DATA : WAIT_CRC;
                    break;
                case WAIT_DATA:
                    _data[_count] = byte;
                    _crc = crc8(_crc, byte);
                    _count++;
                    if (_count == _length) {
                        _state = WAIT_CRC;
                    }
                    break;
                default:
                    if (byte != _crc) {
                        drop();
                        break;
                    }
                    _state = WAIT_START;
                    return true;
            }
        }
        return false;
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline uint8_t Packet<Link, MAX_LENGTH, GAP_MS>::command() const {
        return _command;
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline uint8_t Packet<Link, MAX_LENGTH, GAP_MS>::length() const {
        return _length;
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline const uint8_t *Packet<Link, MAX_LENGTH, GAP_MS>::data() const {
        return _data;
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline uint8_t Packet<Link, MAX_LENGTH, GAP_MS>::readUint8(uint8_t index) const {
        return index < _length ? _data[index] : 0;
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline int8_t Packet<Link, MAX_LENGTH, GAP_MS>::readInt8(uint8_t index) const {
        return (int8_t)readUint8(index);
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline uint16_t Packet<Link, MAX_LENGTH, GAP_MS>::readUint16(uint8_t index) const {
        if (index + 1 >= _length) {
            return 0;
        }
        return (uint16_t)(_data[index] | ((uint16_t)_data[index + 1] << 8));
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline int16_t Packet<Link, MAX_LENGTH, GAP_MS>::readInt16(uint8_t index) const {
        return (int16_t)readUint16(index);
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline uint16_t Packet<Link, MAX_LENGTH, GAP_MS>::dropped() const {
        return _dropped;
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline void Packet<Link, MAX_LENGTH, GAP_MS>::send(uint8_t command, const uint8_t *data, uint8_t length) {
        uint8_t crc = crc8(crc8(0, length), command);
        Link::write8(START);
        Link::write8(length);
        Link::write8(command);
        for (uint8_t i = 0; i < length; i++) {
            Link::write8(data[i]);
            crc = crc8(crc, data[i]);
        }
        Link::write8(crc);
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline void Packet<Link, MAX_LENGTH, GAP_MS>::sendText(uint8_t command, const char *text) {
        uint8_t length = 0;
        while (text[length] && length < 255) {
            length++;
        }
        send(command, (const uint8_t *)text, length);
    }

    template <class Link, uint8_t MAX_LENGTH, uint16_t GAP_MS>
    inline void Packet<Link, MAX_LENGTH, GAP_MS>::drop() {
        _state = WAIT_START;
        _length = 0;
        if (_dropped < 0xFFFF) {
            _dropped++;
        }
    }
}

#endif
