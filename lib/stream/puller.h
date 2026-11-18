#ifndef STREAM_PULLER_H
#define STREAM_PULLER_H

#include <stdint.h>

namespace stream {
    // the board's side of lib/stream/source/stream.py: asks the computer for a file over a Link and pulls
    // the answer piece by piece, so the UNO (2 bytes of UART buffer, 2 KB RAM) never gets more than it asked for
    //
    // used as a template parameter by image::StreamImage, video::StreamPlayer, audio::StreamPlayer and
    // audio::StreamSamplePlayer (they only need the functions below: any struct with them works)
    //
    // protocol (the computer never sends more than 1 byte unasked):
    //   board: a command ('I' picture, 'A' sound, 'O' animation ...), CHUNK, ..., path length, path
    //   computer: 'D' when the answer is ready
    //   board: 'R' for every piece; computer: [length 1 ~ CHUNK][bytes]
    //   an answer starts with a letter: 'E' = error: code (ERROR_NOT_FOUND ~ ERROR_NOT_STREAMABLE), message length,
    //   message; the other letters belong to the library that asked
    //   board: 'M', length, text: stream.py prints it (for tests)
    //
    // Link: a struct providing (see lib/Uno/Uno_serial.h, Uno::UnoSerial)
    //   void init()
    //   bool available()                                 a byte is waiting
    //   bool read8(uint8_t &byte, uint16_t timeoutMs)    false: nothing came in time
    //   void write8(uint8_t byte)
    //   void clearInput()
    //   uint8_t readFlash8(const uint8_t *address)       the path is in flash (PROGMEM)
    // CHUNK: bytes asked for at a time, 1 ~ 255: the RAM of an Answer; bigger = fewer round trips
    //   (every piece waits ~4 ms for the UNO's USB chip)
    //
    // everything but Answer is static: there is one link
    template <class Link, uint8_t CHUNK = 64>
    struct Puller {
        static_assert(CHUNK >= 1, "stream::Puller: CHUNK must be 1 ~ 255");

        using LinkType = Link;
        static constexpr uint8_t CHUNK_SIZE = CHUNK;

        // results: 1 ~ 3 are sent by stream.py ('E'), 4 ~ 6 are found on the board
        static constexpr uint8_t OK = 0;
        static constexpr uint8_t ERROR_NOT_FOUND = 1;       // stream.py has no such file
        static constexpr uint8_t ERROR_UNREADABLE = 2;      // the file is not one of convert.py
        static constexpr uint8_t ERROR_NOT_STREAMABLE = 3;  // this player cannot use it (e.g. a PCM sound to notes)
        static constexpr uint8_t ERROR_NO_ANSWER = 4;       // stream.py not running, or the port is used
        static constexpr uint8_t ERROR_BROKEN = 5;          // the data stopped or made no sense
        static constexpr uint8_t ERROR_NO_MEMORY = 6;       // RAM taken at run time (a 256-color palette) did not fit

        static constexpr uint16_t ANSWER_TIMEOUT_MS = 3000;
        static constexpr uint16_t PIECE_TIMEOUT_MS = 200;

        // starts the link and drops anything left (e.g. half of a broken answer)
        static inline void begin();

        // little-endian
        static inline void write8(uint8_t value);
        static inline void write16(uint16_t value);
        static inline void write32(uint32_t value);

        // path length, path; the path is in flash (Xxx::getPath() of convert.py)
        static inline void writePath(const char *path);

        // a byte from the computer is waiting
        static inline bool available();

        // the next byte is 'D' (an answer is ready), within timeoutMs; false: nothing, or something else
        static inline bool waitAnswer(uint16_t timeoutMs = ANSWER_TIMEOUT_MS);

        // asks for the next piece; length: 1 ~ CHUNK bytes to take with receive(); false: nothing / nonsense came
        static inline bool pullLength(uint8_t &length);

        // one byte of the piece; false: it stopped
        static inline bool receive(uint8_t &byte);

        // a line that stream.py prints (at most 255 characters, in RAM)
        static inline void message(const char *text);

        // one answer read byte by byte, a piece pulled whenever the last one is used up (RAM: CHUNK + 3)
        class Answer {
        public:
            // after waitAnswer(): nothing read yet
            inline void begin();

            // the next byte (little-endian for 16 / 32 bits); 0 once something failed
            inline uint8_t read8();
            inline uint16_t read16();
            inline uint32_t read32();

            // a piece did not come, or made no sense
            inline bool failed() const;

        private:
            uint8_t _buffer[CHUNK];
            uint8_t _length = 0;
            uint8_t _position = 0;
            bool _failed = false;
        };
    };

    // ================= definitions =================

    template <class Link, uint8_t CHUNK>
    inline void Puller<Link, CHUNK>::begin() {
        Link::init();
        Link::clearInput();
    }

    template <class Link, uint8_t CHUNK>
    inline void Puller<Link, CHUNK>::write8(uint8_t value) {
        Link::write8(value);
    }

    template <class Link, uint8_t CHUNK>
    inline void Puller<Link, CHUNK>::write16(uint16_t value) {
        Link::write8((uint8_t)value);
        Link::write8((uint8_t)(value >> 8));
    }

    template <class Link, uint8_t CHUNK>
    inline void Puller<Link, CHUNK>::write32(uint32_t value) {
        for (uint8_t i = 0; i < 4; i++) {
            Link::write8((uint8_t)(value >> (8 * i)));
        }
    }

    template <class Link, uint8_t CHUNK>
    inline void Puller<Link, CHUNK>::writePath(const char *path) {
        uint8_t length = 0;
        while (length < 255 && Link::readFlash8((const uint8_t *)path + length) != '\0') {
            length++;
        }
        Link::write8(length);
        for (uint8_t i = 0; i < length; i++) {
            Link::write8(Link::readFlash8((const uint8_t *)path + i));
        }
    }

    template <class Link, uint8_t CHUNK>
    inline bool Puller<Link, CHUNK>::available() {
        return Link::available();
    }

    template <class Link, uint8_t CHUNK>
    inline bool Puller<Link, CHUNK>::waitAnswer(uint16_t timeoutMs) {
        uint8_t notice = 0;
        return Link::read8(notice, timeoutMs) && notice == 'D';
    }

    template <class Link, uint8_t CHUNK>
    inline bool Puller<Link, CHUNK>::pullLength(uint8_t &length) {
        Link::write8('R');
        return Link::read8(length, PIECE_TIMEOUT_MS) && length >= 1 && length <= CHUNK;
    }

    template <class Link, uint8_t CHUNK>
    inline bool Puller<Link, CHUNK>::receive(uint8_t &byte) {
        return Link::read8(byte, PIECE_TIMEOUT_MS);
    }

    template <class Link, uint8_t CHUNK>
    inline void Puller<Link, CHUNK>::message(const char *text) {
        uint8_t length = 0;
        while (length < 255 && text[length] != '\0') {
            length++;
        }
        Link::write8('M');
        Link::write8(length);
        for (uint8_t i = 0; i < length; i++) {
            Link::write8((uint8_t)text[i]);
        }
    }

    template <class Link, uint8_t CHUNK>
    inline void Puller<Link, CHUNK>::Answer::begin() {
        _length = 0;
        _position = 0;
        _failed = false;
    }

    // nothing else runs while a piece arrives: the UART keeps only 2 bytes
    template <class Link, uint8_t CHUNK>
    inline uint8_t Puller<Link, CHUNK>::Answer::read8() {
        if (_failed) {
            return 0;
        }
        if (_position == _length) {
            uint8_t length = 0;
            if (!pullLength(length)) {
                _failed = true;
                return 0;
            }
            for (uint8_t i = 0; i < length; i++) {
                if (!receive(_buffer[i])) {
                    _failed = true;
                    return 0;
                }
            }
            _length = length;
            _position = 0;
        }
        return _buffer[_position++];
    }

    template <class Link, uint8_t CHUNK>
    inline uint16_t Puller<Link, CHUNK>::Answer::read16() {
        uint16_t value = read8();
        return (uint16_t)(value | ((uint16_t)read8() << 8));
    }

    template <class Link, uint8_t CHUNK>
    inline uint32_t Puller<Link, CHUNK>::Answer::read32() {
        uint32_t value = 0;
        for (uint8_t i = 0; i < 4; i++) {
            value |= (uint32_t)read8() << (8 * i);
        }
        return value;
    }

    template <class Link, uint8_t CHUNK>
    inline bool Puller<Link, CHUNK>::Answer::failed() const {
        return _failed;
    }
}

#endif
