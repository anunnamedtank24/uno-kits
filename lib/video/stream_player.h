#ifndef VIDEO_STREAM_PLAYER_H
#define VIDEO_STREAM_PLAYER_H

#include "decoder.h"

namespace video {
    // plays an animation that lib/stream/source/stream.py sends
    // the board knows the path (Animation::getPath(), made by convert.py --target stream),
    // stream.py on the computer needs no arguments: it loads whatever the board opens
    // the computer keeps the time; play / pause / stop / seek / speed / loop are sent to it
    //
    // Screen: as for Decoder, plus BusType::nowMs / readFlash16
    // Stream: the link to stream.py (lib/stream: stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>, CHUNK 16 ~ 255),
    //   a struct providing CHUNK_SIZE, OK, ERROR_NOT_FOUND ~ ERROR_BROKEN, begin(), write8/16/32(),
    //   writePath(flash path), available(), waitAnswer(timeoutMs), and a class Answer: begin(), read8(),
    //   read16(), read32(), failed()
    //
    // protocol (after Stream's own, see lib/stream/puller.h; full description: lib/stream/README.md)
    //   board -> computer, commands:
    //     'O' open: chunk, flags (bit 0 play, bit 1 loop), speed (uint16_t %), start (uint32_t ms), path length, path
    //     'N' next frame (answered when one is due)   'P' play   'S' pause
    //     'J' seek: uint32_t ms   'V' speed: uint16_t %   'L' loop: 0 / 1   'R' next chunk
    //   answers (first byte): 'G' header (to 'O'), 'F' frame (to 'N'), 'K' no frame yet (to 'N', every second,
    //     or when the status changes, e.g. ended; the board sends 'N' again), 'A' status (to P S J V L), 'E' error
    //     status (in G F K A): flags (bit 0 playing, bit 1 ended), position (uint32_t ms), frame (uint16_t)
    //   no answer in time, or a wrong byte: the board opens again, from where it was
    template <class Screen, class Stream>
    class StreamPlayer {
        static_assert(Stream::CHUNK_SIZE >= 16, "video::StreamPlayer: the Stream's CHUNK must be 16 ~ 255");

    public:
        static constexpr uint16_t OPEN_TIMEOUT_MS = 3000;
        static constexpr uint16_t FRAME_TIMEOUT_MS = 3000;
        static constexpr uint16_t ANSWER_TIMEOUT_MS = 1000;

        // error(): what went wrong last (Stream's codes, plus ERROR_TOO_BIG)
        static constexpr uint8_t OK = Stream::OK;
        static constexpr uint8_t ERROR_NOT_FOUND = Stream::ERROR_NOT_FOUND; // stream.py: no such file (or outside lib/video)
        static constexpr uint8_t ERROR_UNREADABLE = Stream::ERROR_UNREADABLE; // stream.py: not a file made by convert.py
        static constexpr uint8_t ERROR_NOT_STREAMABLE = Stream::ERROR_NOT_STREAMABLE; // stream.py: this file cannot be streamed
        static constexpr uint8_t ERROR_NO_ANSWER = Stream::ERROR_NO_ANSWER; // stream.py not running, or the link broke
        static constexpr uint8_t ERROR_BROKEN = Stream::ERROR_BROKEN; // an answer made no sense
        static constexpr uint8_t ERROR_NO_MEMORY = Stream::ERROR_NO_MEMORY; // a 256-color palette did not fit in RAM
        static constexpr uint8_t ERROR_TOO_BIG = 7; // not on the screen at (x, y), or scaled rows wider than MAX_SCALED_WIDTH: read, not drawn

        inline explicit StreamPlayer(Screen &screen);

        // remembers the animation and starts the link; update() connects (and reconnects) by itself
        template <class Animation>
        inline void open(int16_t x, int16_t y, bool play = true, bool loop = true, uint16_t speedPercent = 100);

        // stops asking for frames (stream.py keeps waiting for the next open); a 256-color palette is given back
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

    private:
        using Bus = typename Screen::BusType;

        static constexpr uint8_t STATE_CLOSED = 0;
        static constexpr uint8_t STATE_OPENING = 1;
        static constexpr uint8_t STATE_CONNECTED = 2;

        static constexpr uint8_t FLAG_PLAYING = 1;
        static constexpr uint8_t FLAG_ENDED = 2;
        static constexpr uint8_t OPEN_PLAY = 1;
        static constexpr uint8_t OPEN_LOOP = 2;

        using Color = typename Decoder<Screen>::Color;

        Decoder<Screen> _decoder;
        // COLORS_256: the palette, taken when the header comes, given back by close() / the next header / ~StreamPlayer
        HeapPalette<Color> _heapPalette;
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
        uint8_t _error = OK;

        // the current answer (RAM: the Stream's CHUNK)
        typename Stream::Answer _answer;

        inline void sendOpen();
        inline void sendNext();
        inline void lose();
        inline bool sendCommand(uint8_t command, uint32_t value, uint8_t valueBytes);

        inline void readStatus();
        inline bool readHeader();
        inline void readError();
        inline bool drawFrameAnswer();
    };

    // ================= definitions =================

    template <class Screen, class Stream>
    inline StreamPlayer<Screen, Stream>::StreamPlayer(Screen &screen) : _decoder(screen) {
    }

    template <class Screen, class Stream>
    template <class Animation>
    inline void StreamPlayer<Screen, Stream>::open(int16_t x, int16_t y, bool play, bool loop, uint16_t speedPercent) {
        static_assert(warn::PaletteInRam<Animation::colorMode == COLORS_256>::ok, "");
        static_assert(warn::NotForStream<!(Animation::target & TARGET_STREAM)>::ok, "");
        _path = Animation::getPath();
        _x = x;
        _y = y;
        _openFlags = (play ? OPEN_PLAY : 0) | (loop ? OPEN_LOOP : 0);
        _speed = speedPercent;
        _positionMs = 0;
        _drawnCount = 0;
        _error = OK;
        sendOpen();
    }

    template <class Screen, class Stream>
    inline void StreamPlayer<Screen, Stream>::close() {
        _state = STATE_CLOSED;
        _heapPalette.release();
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::update() {
        if (_state == STATE_CLOSED) {
            return false;
        }
        uint32_t now = Bus::nowMs();
        uint32_t timeoutMs = _state == STATE_OPENING ? OPEN_TIMEOUT_MS : FRAME_TIMEOUT_MS;
        if (!Stream::available()) {
            if (now - _sentMs >= timeoutMs) {
                // keep an error stream.py told (e.g. not found), it is more useful than "no answer"
                if (_error == OK || _error == ERROR_TOO_BIG) {
                    _error = ERROR_NO_ANSWER;
                }
                lose();
            }
            return false;
        }

        if (!Stream::waitAnswer(0)) {
            return false;
        }

        _answer.begin();
        uint8_t type = _answer.read8();
        if (_state == STATE_OPENING) {
            bool fine = false;
            if (type == 'G') {
                fine = readHeader();
            } else if (type == 'E') {
                readError();
            }
            if (!_answer.failed() && type == 'G' && !fine && _error == ERROR_NO_MEMORY) {
                // no RAM for the palette: open again after a whole timeout (RAM may be free then)
                _sentMs = Bus::nowMs();
            } else if (_answer.failed() || (type == 'G' && !fine)) {
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
            if (_answer.failed()) {
                lose();
            } else {
                sendNext();
            }
            return false;
        }
        lose();
        return false;
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::play() {
        _openFlags |= OPEN_PLAY;
        return sendCommand('P', 0, 0);
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::pause() {
        _openFlags &= ~OPEN_PLAY;
        return sendCommand('S', 0, 0);
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::stop() {
        return pause() && seek(0);
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::seek(uint32_t positionMs) {
        _positionMs = positionMs;
        return sendCommand('J', positionMs, 4);
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::setSpeed(uint16_t percent) {
        _speed = percent;
        return sendCommand('V', percent, 2);
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::setLoop(bool loop) {
        if (loop) {
            _openFlags |= OPEN_LOOP;
        } else {
            _openFlags &= ~OPEN_LOOP;
        }
        return sendCommand('L', loop ? 1 : 0, 1);
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::isConnected() const {
        return _state == STATE_CONNECTED;
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::isPlaying() const {
        return (_flags & FLAG_PLAYING) != 0;
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::isEnded() const {
        return (_flags & FLAG_ENDED) != 0;
    }

    template <class Screen, class Stream>
    inline uint32_t StreamPlayer<Screen, Stream>::positionMs() const {
        return _positionMs;
    }

    template <class Screen, class Stream>
    inline uint16_t StreamPlayer<Screen, Stream>::frame() const {
        return _frame;
    }

    template <class Screen, class Stream>
    inline uint32_t StreamPlayer<Screen, Stream>::durationMs() const {
        return _durationMs;
    }

    template <class Screen, class Stream>
    inline uint16_t StreamPlayer<Screen, Stream>::frameCount() const {
        return _frameCount;
    }

    template <class Screen, class Stream>
    inline uint32_t StreamPlayer<Screen, Stream>::drawnCount() const {
        return _drawnCount;
    }

    template <class Screen, class Stream>
    inline uint8_t StreamPlayer<Screen, Stream>::error() const {
        return _error;
    }

    template <class Screen, class Stream>
    inline void StreamPlayer<Screen, Stream>::sendOpen() {
        // anything left from a broken answer is dropped
        Stream::begin();
        Stream::write8('O');
        Stream::write8(Stream::CHUNK_SIZE);
        Stream::write8(_openFlags);
        Stream::write16(_speed);
        Stream::write32(_positionMs);
        Stream::writePath(_path);
        _state = STATE_OPENING;
        _sentMs = Bus::nowMs();
    }

    template <class Screen, class Stream>
    inline void StreamPlayer<Screen, Stream>::sendNext() {
        Stream::write8('N');
        _sentMs = Bus::nowMs();
    }

    // open again from where it was
    template <class Screen, class Stream>
    inline void StreamPlayer<Screen, Stream>::lose() {
        if (_state != STATE_CLOSED) {
            sendOpen();
        }
    }

    // a command answered with 'A'; frames that were already on their way are drawn meanwhile
    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::sendCommand(uint8_t command, uint32_t value, uint8_t valueBytes) {
        if (_state != STATE_CONNECTED) {
            return false;
        }
        Stream::write8(command);
        for (uint8_t i = 0; i < valueBytes; i++) {
            Stream::write8((uint8_t)(value >> (8 * i)));
        }
        uint32_t startMs = Bus::nowMs();
        while (Bus::nowMs() - startMs < ANSWER_TIMEOUT_MS) {
            if (!Stream::waitAnswer(10)) {
                continue;
            }
            _answer.begin();
            uint8_t type = _answer.read8();
            if (type == 'A' || type == 'K') {
                readStatus();
                if (_answer.failed()) {
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

    template <class Screen, class Stream>
    inline void StreamPlayer<Screen, Stream>::readStatus() {
        _flags = _answer.read8();
        _positionMs = _answer.read32();
        _frame = _answer.read16();
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::readHeader() {
        readStatus();
        uint8_t width = _answer.read8();
        uint8_t height = _answer.read8();
        uint8_t scaleX = _answer.read8();
        uint8_t scaleY = _answer.read8();
        uint8_t colorMode = _answer.read8();
        uint8_t countByte = _answer.read8();
        _frameCount = _answer.read16();
        _durationMs = _answer.read32();
        uint16_t colorCount = colorMode == COLORS_256 && countByte == 0 ? 256 : countByte;
        bool isColorModeFine = colorMode == COLORS_565 || colorMode == COLORS_256
                            || (colorMode == COLORS_16 && colorCount <= MAX_RAM_COLORS);
        if (_answer.failed() || width == 0 || height == 0 || scaleX == 0 || scaleY == 0 || !isColorModeFine) {
            _error = ERROR_BROKEN;
            return false;
        }
        _decoder.setColorMode(colorMode);
        // the palette of the last open is of no use any more
        _heapPalette.release();
        if (colorMode == COLORS_16) {
            for (uint8_t i = 0; i < colorCount; i++) {
                _decoder.setColor(i, _answer.read16());
            }
        } else if (colorMode == COLORS_256) {
            if (!_heapPalette.take(colorCount)) {
                _error = ERROR_NO_MEMORY;
                return false;
            }
            for (uint16_t i = 0; i < colorCount; i++) {
                _heapPalette.colors()[i] = Decoder<Screen>::ColorFormat::fromRGB565(_answer.read16());
            }
            _decoder.setRamPalette(_heapPalette.colors());
        }
        // too big: still connected, frames are read but not drawn
        _error = _decoder.setArea(_x, _y, width, height, scaleX, scaleY) ? OK : ERROR_TOO_BIG;
        return !_answer.failed();
    }

    template <class Screen, class Stream>
    inline void StreamPlayer<Screen, Stream>::readError() {
        _error = _answer.read8();
        uint8_t length = _answer.read8();
        for (uint8_t i = 0; i < length; i++) {
            (void)_answer.read8();
        }
        // stream.py answered: wait a whole timeout before trying again (the file may appear later)
        _sentMs = Bus::nowMs();
    }

    template <class Screen, class Stream>
    inline bool StreamPlayer<Screen, Stream>::drawFrameAnswer() {
        readStatus();
        bool ok = !_answer.failed() && _decoder.drawFrame(_answer);
        if (!ok || _answer.failed()) {
            lose();
            return false;
        }
        sendNext();
        _drawnCount++;
        return true;
    }
}

#endif
