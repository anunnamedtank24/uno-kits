#ifndef GIF_STREAM_PLAYER_H
#define GIF_STREAM_PLAYER_H

#include "decoder.h"

namespace GIF {
    // plays an animation that stream.py sends over a Link (e.g. Uno::UnoSerial)
    // the board knows the path (Animation::getPath(), made by convert.py --target mac),
    // stream.py on the computer needs no arguments: it loads whatever the board opens
    // the computer keeps the time; play / pause / stop / seek / speed / loop are sent to it
    //
    // Screen: as for Decoder, plus BusType::nowMs / readFlash8 / readFlash16
    // Link: a struct providing (see lib/Uno/Uno_serial.h)
    //   init(), available(), read8(uint8_t &byte, uint16_t timeoutMs), write8(uint8_t), clearInput()
    // CHUNK: bytes received at a time (RAM), 16 ~ 255
    //
    // protocol (full description: lib/GIF/format.md)
    //   board -> computer, commands:
    //     'O' open: chunk, flags (bit 0 play, bit 1 loop), speed (uint16_t %), start (uint32_t ms), path length, path
    //     'N' next frame (answered when one is due)   'P' play   'S' pause
    //     'J' seek: uint32_t ms   'V' speed: uint16_t %   'L' loop: 0 / 1   'R' next chunk
    //   computer -> board, never more than 1 byte unasked (the UART keeps only 2):
    //     'D' an answer is ready: the board sends 'R' when it can, then gets [length][bytes] chunks
    //   answers (first byte): 'G' header (to 'O'), 'F' frame (to 'N'), 'K' no frame yet (to 'N', every second,
    //     or when the status changes, e.g. ended; the board sends 'N' again), 'A' status (to P S J V L), 'E' error
    //     status (in G F K A): flags (bit 0 playing, bit 1 ended), position (uint32_t ms), frame (uint16_t)
    //   no answer in time, or a wrong byte: the board opens again, from where it was
    template <class Screen, class Link, uint8_t CHUNK = 64>
    class StreamPlayer {
        static_assert(CHUNK >= 16, "GIF::StreamPlayer: CHUNK must be 16 ~ 255");

    public:
        static constexpr uint16_t OPEN_TIMEOUT_MS = 3000;
        static constexpr uint16_t FRAME_TIMEOUT_MS = 3000;
        static constexpr uint16_t ANSWER_TIMEOUT_MS = 1000;
        static constexpr uint16_t CHUNK_TIMEOUT_MS = 200;

        // error(): what went wrong last
        static constexpr uint8_t ERROR_NONE = 0;
        static constexpr uint8_t ERROR_NOT_FOUND = 1; // stream.py: no such file (or outside lib/GIF)
        static constexpr uint8_t ERROR_UNREADABLE = 2; // stream.py: not a file made by convert.py
        static constexpr uint8_t ERROR_NOT_STREAMABLE = 3; // stream.py: palette 256
        static constexpr uint8_t ERROR_TOO_BIG = 4; // does not fit the screen at (x, y): read, not drawn
        static constexpr uint8_t ERROR_NO_ANSWER = 5; // stream.py not running, or the link broke

        inline explicit StreamPlayer(Screen &screen);

        // remembers the animation and starts the link; update() connects (and reconnects) by itself
        template <class Animation>
        inline void open(int16_t x, int16_t y, bool play = true, bool loop = true, uint16_t speedPercent = 100);

        // stops asking for frames (stream.py keeps waiting for the next open)
        inline void close();

        // call as often as possible: waits for nothing, but once a frame starts arriving it is drawn
        // completely before returning; true if a frame was drawn
        inline bool update();

        // sent at once, true when stream.py answered (false: not connected or no answer)
        inline bool play();
        inline bool pause();
        inline bool stop(); // pause + seek(0)
        inline bool seek(uint32_t positionMs);
        inline bool setSpeed(uint16_t percent); // 100 = normal, 1 ~ 1000
        inline bool setLoop(bool loop);

        // from the last answer of stream.py
        inline bool isConnected() const;
        inline bool isPlaying() const;
        inline bool isEnded() const; // loop off and the last frame reached
        inline uint32_t positionMs() const;
        inline uint16_t frame() const;
        inline uint32_t durationMs() const;
        inline uint16_t frameCount() const;
        inline uint32_t drawnCount() const; // frames drawn since open()
        inline uint8_t error() const;

        // Reader for Decoder: next byte of the current answer, 0 after an error
        inline uint8_t read8();

    private:
        using Bus = typename Screen::BusType;

        static constexpr uint8_t STATE_CLOSED = 0;
        static constexpr uint8_t STATE_OPENING = 1;
        static constexpr uint8_t STATE_CONNECTED = 2;

        static constexpr uint8_t FLAG_PLAYING = 1;
        static constexpr uint8_t FLAG_ENDED = 2;
        static constexpr uint8_t OPEN_PLAY = 1;
        static constexpr uint8_t OPEN_LOOP = 2;

        Decoder<Screen> _decoder;
        uint8_t _state = STATE_CLOSED;
        uint32_t _sentMs = 0;

        // what open() was given, kept up to date to reopen at the same place
        const char *_path = nullptr; // in flash
        int16_t _x = 0;
        int16_t _y = 0;
        uint8_t _openFlags = 0;
        uint16_t _speed = 100;

        // status
        uint8_t _flags = 0;
        uint32_t _positionMs = 0;
        uint16_t _frame = 0;
        uint32_t _durationMs = 0;
        uint16_t _frameCount = 0;
        uint32_t _drawnCount = 0;
        uint8_t _error = ERROR_NONE;

        // the current answer
        uint8_t _buffer[CHUNK];
        uint8_t _length = 0;
        uint8_t _position = 0;
        bool _readError = false;

        inline void sendOpen();
        inline void sendNext();
        inline void lose();
        inline bool sendCommand(uint8_t command, uint32_t value, uint8_t valueBytes);

        inline void beginAnswer();
        inline bool receiveChunk();
        inline uint32_t read32();
        inline void readStatus();
        inline void readHeader();
        inline void readError();
        inline bool drawFrameAnswer();
    };

    // ================= definitions =================

    template <class Screen, class Link, uint8_t CHUNK>
    inline StreamPlayer<Screen, Link, CHUNK>::StreamPlayer(Screen &screen) : _decoder(screen) {
    }

    template <class Screen, class Link, uint8_t CHUNK>
    template <class Animation>
    inline void StreamPlayer<Screen, Link, CHUNK>::open(int16_t x, int16_t y, bool play, bool loop, uint16_t speedPercent) {
        static_assert(Animation::colorMode != COLORS_256,
                      "GIF::StreamPlayer: palette 256 cannot be streamed (512 B RAM), make it with --palette 16 or 565");
        static_assert(warn::NotForMac<!(Animation::target & TARGET_MAC)>::ok, "");
        _path = Animation::getPath();
        _x = x;
        _y = y;
        _openFlags = (play ? OPEN_PLAY : 0) | (loop ? OPEN_LOOP : 0);
        _speed = speedPercent;
        _positionMs = 0;
        _drawnCount = 0;
        _error = ERROR_NONE;
        Link::init();
        sendOpen();
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline void StreamPlayer<Screen, Link, CHUNK>::close() {
        _state = STATE_CLOSED;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::update() {
        if (_state == STATE_CLOSED) {
            return false;
        }
        uint32_t now = Bus::nowMs();
        uint32_t timeoutMs = _state == STATE_OPENING ? OPEN_TIMEOUT_MS : FRAME_TIMEOUT_MS;
        if (!Link::available()) {
            if (now - _sentMs >= timeoutMs) {
                // keep an error stream.py told (e.g. not found), it is more useful than "no answer"
                if (_error == ERROR_NONE || _error == ERROR_TOO_BIG) {
                    _error = ERROR_NO_ANSWER;
                }
                lose();
            }
            return false;
        }

        uint8_t notice = 0;
        Link::read8(notice, 0);
        if (notice != 'D') {
            return false;
        }

        beginAnswer();
        uint8_t type = read8();
        if (_state == STATE_OPENING) {
            if (type == 'G') {
                readHeader();
            } else if (type == 'E') {
                readError();
            }
            if (_readError) {
                lose();
            } else if (type == 'G') {
                _state = STATE_CONNECTED;
                sendNext();
            }
            return false;
        }
        if (type == 'F') {
            return drawFrameAnswer();
        }
        if (type == 'K') {
            // still there, no frame due (paused, ended, or a long frame)
            readStatus();
            if (_readError) {
                lose();
            } else {
                sendNext();
            }
            return false;
        }
        lose();
        return false;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::play() {
        _openFlags |= OPEN_PLAY;
        return sendCommand('P', 0, 0);
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::pause() {
        _openFlags &= ~OPEN_PLAY;
        return sendCommand('S', 0, 0);
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::stop() {
        return pause() && seek(0);
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::seek(uint32_t positionMs) {
        _positionMs = positionMs;
        return sendCommand('J', positionMs, 4);
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::setSpeed(uint16_t percent) {
        _speed = percent;
        return sendCommand('V', percent, 2);
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::setLoop(bool loop) {
        if (loop) {
            _openFlags |= OPEN_LOOP;
        } else {
            _openFlags &= ~OPEN_LOOP;
        }
        return sendCommand('L', loop ? 1 : 0, 1);
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::isConnected() const {
        return _state == STATE_CONNECTED;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::isPlaying() const {
        return (_flags & FLAG_PLAYING) != 0;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::isEnded() const {
        return (_flags & FLAG_ENDED) != 0;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline uint32_t StreamPlayer<Screen, Link, CHUNK>::positionMs() const {
        return _positionMs;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline uint16_t StreamPlayer<Screen, Link, CHUNK>::frame() const {
        return _frame;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline uint32_t StreamPlayer<Screen, Link, CHUNK>::durationMs() const {
        return _durationMs;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline uint16_t StreamPlayer<Screen, Link, CHUNK>::frameCount() const {
        return _frameCount;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline uint32_t StreamPlayer<Screen, Link, CHUNK>::drawnCount() const {
        return _drawnCount;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline uint8_t StreamPlayer<Screen, Link, CHUNK>::error() const {
        return _error;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline uint8_t StreamPlayer<Screen, Link, CHUNK>::read8() {
        if (_readError) {
            return 0;
        }
        if (_position == _length && !receiveChunk()) {
            _readError = true;
            return 0;
        }
        uint8_t byte = _buffer[_position];
        _position++;
        return byte;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline void StreamPlayer<Screen, Link, CHUNK>::sendOpen() {
        // anything left from a broken answer is dropped
        Link::clearInput();
        Link::write8('O');
        Link::write8(CHUNK);
        Link::write8(_openFlags);
        Link::write8((uint8_t)_speed);
        Link::write8((uint8_t)(_speed >> 8));
        for (uint8_t i = 0; i < 4; i++) {
            Link::write8((uint8_t)(_positionMs >> (8 * i)));
        }
        uint8_t length = 0;
        while (length < 255 && Bus::readFlash8((const uint8_t *)_path + length) != 0) {
            length++;
        }
        Link::write8(length);
        for (uint8_t i = 0; i < length; i++) {
            Link::write8(Bus::readFlash8((const uint8_t *)_path + i));
        }
        _state = STATE_OPENING;
        _sentMs = Bus::nowMs();
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline void StreamPlayer<Screen, Link, CHUNK>::sendNext() {
        Link::write8('N');
        _sentMs = Bus::nowMs();
    }

    // open again from where it was
    template <class Screen, class Link, uint8_t CHUNK>
    inline void StreamPlayer<Screen, Link, CHUNK>::lose() {
        if (_state != STATE_CLOSED) {
            sendOpen();
        }
    }

    // a command answered with 'A'; frames that were already on their way are drawn meanwhile
    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::sendCommand(uint8_t command, uint32_t value, uint8_t valueBytes) {
        if (_state != STATE_CONNECTED) {
            return false;
        }
        Link::write8(command);
        for (uint8_t i = 0; i < valueBytes; i++) {
            Link::write8((uint8_t)(value >> (8 * i)));
        }
        uint32_t startMs = Bus::nowMs();
        while (Bus::nowMs() - startMs < ANSWER_TIMEOUT_MS) {
            uint8_t notice = 0;
            if (!Link::read8(notice, 10) || notice != 'D') {
                continue;
            }
            beginAnswer();
            uint8_t type = read8();
            if (type == 'A' || type == 'K') {
                readStatus();
                if (_readError) {
                    break;
                }
                if (type == 'A') {
                    return true;
                }
                // the answer to an 'N' sent before: ask again, keep waiting for 'A'
                sendNext();
                continue;
            }
            if (type == 'F') {
                drawFrameAnswer();
                if (_state != STATE_CONNECTED) {
                    return false;
                }
                continue;
            }
            break;
        }
        _error = ERROR_NO_ANSWER;
        lose();
        return false;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline void StreamPlayer<Screen, Link, CHUNK>::beginAnswer() {
        // 'D' was read: the first chunk comes after 'R'
        _length = 0;
        _position = 0;
        _readError = false;
    }

    // nothing is drawn while a chunk arrives: the UART keeps only 2 bytes
    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::receiveChunk() {
        Link::write8('R');
        uint8_t length = 0;
        if (!Link::read8(length, CHUNK_TIMEOUT_MS) || length == 0 || length > CHUNK) {
            return false;
        }
        for (uint8_t i = 0; i < length; i++) {
            if (!Link::read8(_buffer[i], CHUNK_TIMEOUT_MS)) {
                return false;
            }
        }
        _length = length;
        _position = 0;
        return true;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline uint32_t StreamPlayer<Screen, Link, CHUNK>::read32() {
        uint32_t value = 0;
        for (uint8_t i = 0; i < 4; i++) {
            value |= (uint32_t)read8() << (8 * i);
        }
        return value;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline void StreamPlayer<Screen, Link, CHUNK>::readStatus() {
        _flags = read8();
        _positionMs = read32();
        _frame = read8();
        _frame |= (uint16_t)read8() << 8;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline void StreamPlayer<Screen, Link, CHUNK>::readHeader() {
        readStatus();
        uint8_t width = read8();
        uint8_t height = read8();
        uint8_t scaleX = read8();
        uint8_t scaleY = read8();
        uint8_t colorMode = read8();
        uint8_t colorCount = read8();
        _frameCount = read8();
        _frameCount |= (uint16_t)read8() << 8;
        _durationMs = read32();
        bool isColorModeFine = colorMode == COLORS_565 || (colorMode == COLORS_16 && colorCount <= MAX_RAM_COLORS);
        if (_readError || width == 0 || height == 0 || scaleX == 0 || scaleY == 0 || !isColorModeFine) {
            _readError = true;
            return;
        }
        _decoder.setColorMode(colorMode);
        if (colorMode == COLORS_16) {
            for (uint8_t i = 0; i < colorCount; i++) {
                uint16_t color = read8();
                color |= (uint16_t)read8() << 8;
                _decoder.setColor(i, color);
            }
        }
        // too big: still connected, frames are read but not drawn
        _error = _decoder.setArea(_x, _y, width, height, scaleX, scaleY) ? ERROR_NONE : ERROR_TOO_BIG;
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline void StreamPlayer<Screen, Link, CHUNK>::readError() {
        _error = read8();
        uint8_t length = read8();
        for (uint8_t i = 0; i < length; i++) {
            (void)read8();
        }
        // stream.py answered: wait a whole timeout before trying again (the file may appear later)
        _sentMs = Bus::nowMs();
    }

    template <class Screen, class Link, uint8_t CHUNK>
    inline bool StreamPlayer<Screen, Link, CHUNK>::drawFrameAnswer() {
        readStatus();
        bool ok = !_readError && _decoder.drawFrame(*this);
        if (!ok || _readError) {
            lose();
            return false;
        }
        sendNext();
        _drawnCount++;
        return true;
    }
}

#endif
