#ifndef IMAGE_STREAM_IMAGE_H
#define IMAGE_STREAM_IMAGE_H

#include "decoder.h"

namespace image {
    // draws a picture sent by the computer (lib/image/source/stream.py) over a Link
    //
    // Screen: as for Decoder (writeWindow, BusType::readFlash8 / nowMs)
    // Link: a struct providing (see lib/Uno/Uno_serial.h)
    //   void init()
    //   bool read8(uint8_t &byte, uint16_t timeoutMs)   false: nothing came in time
    //   void write8(uint8_t byte)
    //   void clearInput()
    //
    // Picture: a struct in namespace picture with getPath() (any target; --target mac is made for it)
    //   COLORS_256 cannot be streamed: its palette would need 512 B of RAM
    //
    // protocol (the computer never sends more than 1 byte unasked, the UNO keeps only 2):
    //   board: 'I', CHUNK, path length, path
    //   computer: 'D' when the answer is ready
    //   board: 'R' for every piece; computer: [length 1 ~ CHUNK][bytes]
    //   answer: 'H', width, height, scaleX, scaleY, colorMode, encoding, colorCount,
    //           data size (uint32_t), palette (colorCount x RGB565, only COLORS_2 / COLORS_16), pixel data
    //           or 'E', code, message length, message
    //
    // draw() waits until the whole picture is drawn (or something fails): a 240 x 240 RGB565
    // picture is ~115 KB, about 1.5 s at 1 Mbaud
    //
    // RAM: CHUNK + the decoder (2 x SEGMENT + 32 + ~20 bytes)
    template <class Screen, class Link, uint8_t CHUNK = 64, uint8_t SEGMENT = 32>
    class StreamImage {
    public:
        static_assert(CHUNK >= 1, "image::StreamImage: CHUNK must be 1 ~ 255");

        // results of draw()
        static constexpr uint8_t OK = 0;
        static constexpr uint8_t ERROR_NOT_FOUND = 1;       // stream.py has no such file
        static constexpr uint8_t ERROR_UNREADABLE = 2;      // the file is not a picture of convert.py
        static constexpr uint8_t ERROR_NOT_STREAMABLE = 3;  // COLORS_256
        static constexpr uint8_t ERROR_NO_ANSWER = 4;       // stream.py not running, or the port is used
        static constexpr uint8_t ERROR_BROKEN = 5;          // the data stopped or made no sense

        inline StreamImage(Screen &screen);

        // top-left at (x, y), clipped: any x, y works; returns OK or an error
        template <class Picture>
        inline uint8_t draw(int16_t x, int16_t y);

    private:
        using Bus = typename Screen::BusType;

        static constexpr uint16_t ANSWER_TIMEOUT_MS = 3000;
        static constexpr uint16_t CHUNK_TIMEOUT_MS = 200;

        // pulls the answer piece by piece, as the decoder reads it
        class Reader {
        public:
            inline explicit Reader(StreamImage &owner);
            inline uint8_t read8();
            inline bool failed() const;

        private:
            StreamImage &_owner;
            uint8_t _length = 0;
            uint8_t _position = 0;
            bool _failed = false;
        };

        Decoder<Screen, SEGMENT> _decoder;
        uint8_t _buffer[CHUNK];

        inline bool pull(uint8_t &length);
        inline uint8_t drawPath(const char *path, int16_t x, int16_t y);
    };

    // ================= definitions =================

    template <class Screen, class Link, uint8_t CHUNK, uint8_t SEGMENT>
    inline StreamImage<Screen, Link, CHUNK, SEGMENT>::Reader::Reader(StreamImage &owner) : _owner(owner) {
    }

    template <class Screen, class Link, uint8_t CHUNK, uint8_t SEGMENT>
    inline uint8_t StreamImage<Screen, Link, CHUNK, SEGMENT>::Reader::read8() {
        if (_failed) {
            return 0;
        }
        if (_position == _length) {
            if (!_owner.pull(_length)) {
                // the rest reads as 0: the picture is finished with black, draw() reports it
                _failed = true;
                return 0;
            }
            _position = 0;
        }
        return _owner._buffer[_position++];
    }

    template <class Screen, class Link, uint8_t CHUNK, uint8_t SEGMENT>
    inline bool StreamImage<Screen, Link, CHUNK, SEGMENT>::Reader::failed() const {
        return _failed;
    }

    template <class Screen, class Link, uint8_t CHUNK, uint8_t SEGMENT>
    inline StreamImage<Screen, Link, CHUNK, SEGMENT>::StreamImage(Screen &screen) : _decoder(screen) {
    }

    template <class Screen, class Link, uint8_t CHUNK, uint8_t SEGMENT>
    template <class Picture>
    inline uint8_t StreamImage<Screen, Link, CHUNK, SEGMENT>::draw(int16_t x, int16_t y) {
        static_assert(Picture::colorMode != COLORS_256,
                      "image::StreamImage: COLORS_256 cannot be streamed (its palette needs 512 B of RAM): "
                      "make it with --palette 16 or 565");
        static_assert(warn::NotForMac<!(Picture::target & TARGET_MAC)>::ok, "");
        return drawPath(Picture::getPath(), x, y);
    }

    template <class Screen, class Link, uint8_t CHUNK, uint8_t SEGMENT>
    inline bool StreamImage<Screen, Link, CHUNK, SEGMENT>::pull(uint8_t &length) {
        Link::write8('R');
        if (!Link::read8(length, CHUNK_TIMEOUT_MS) || length == 0 || length > CHUNK) {
            return false;
        }
        for (uint8_t i = 0; i < length; i++) {
            if (!Link::read8(_buffer[i], CHUNK_TIMEOUT_MS)) {
                return false;
            }
        }
        return true;
    }

    template <class Screen, class Link, uint8_t CHUNK, uint8_t SEGMENT>
    inline uint8_t StreamImage<Screen, Link, CHUNK, SEGMENT>::drawPath(const char *path, int16_t x, int16_t y) {
        Link::init();
        Link::clearInput();
        // the path is in flash (PROGMEM)
        uint8_t length = 0;
        while (Bus::readFlash8((const uint8_t *)path + length) != '\0' && length < 255) {
            length++;
        }
        Link::write8('I');
        Link::write8(CHUNK);
        Link::write8(length);
        for (uint8_t i = 0; i < length; i++) {
            Link::write8(Bus::readFlash8((const uint8_t *)path + i));
        }

        uint8_t notice = 0;
        if (!Link::read8(notice, ANSWER_TIMEOUT_MS) || notice != 'D') {
            return ERROR_NO_ANSWER;
        }
        Reader reader(*this);
        uint8_t kind = reader.read8();
        if (kind == 'E') {
            uint8_t code = reader.read8();
            return reader.failed() ? ERROR_BROKEN : code;
        }
        if (kind != 'H') {
            return ERROR_BROKEN;
        }
        uint8_t width = reader.read8();
        uint8_t height = reader.read8();
        uint8_t scaleX = reader.read8();
        uint8_t scaleY = reader.read8();
        uint8_t colorMode = reader.read8();
        uint8_t encoding = reader.read8();
        uint8_t colorCount = reader.read8();
        for (uint8_t i = 0; i < 4; i++) {
            reader.read8(); // data size: the computer knows when it ends
        }
        if (reader.failed() || colorMode > COLORS_565 || colorMode == COLORS_256 || encoding > ENCODING_RUNS
            || colorCount > MAX_RAM_COLORS) {
            return ERROR_BROKEN;
        }
        for (uint8_t i = 0; i < colorCount; i++) {
            uint16_t color = reader.read8();
            color |= (uint16_t)reader.read8() << 8;
            _decoder.setColor(i, color);
        }
        _decoder.setArea(x, y, width, height, scaleX, scaleY);
        _decoder.setFormat(colorMode, encoding);
        _decoder.draw(reader);
        return reader.failed() ? ERROR_BROKEN : OK;
    }
}

#endif
