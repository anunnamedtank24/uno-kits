#ifndef AUDIO_STREAM_LINK_H
#define AUDIO_STREAM_LINK_H

#include <stdint.h>

namespace audio {
    // results of StreamPlayer::open / StreamSamplePlayer::open (and error())
    constexpr uint8_t STREAM_OK = 0;
    constexpr uint8_t ERROR_NOT_FOUND = 1;      // stream.py has no such file
    constexpr uint8_t ERROR_UNREADABLE = 2;     // the file is not a sound of convert.py
    constexpr uint8_t ERROR_WRONG_KIND = 3;     // notes / rhythm to a sample player, or PCM to StreamPlayer
    constexpr uint8_t ERROR_NO_ANSWER = 4;      // stream.py not running, or the port is used
    constexpr uint8_t ERROR_BROKEN = 5;         // the data stopped or made no sense

    // the board's side of lib/audio/source/stream.py (the computer never sends more than 1 byte unasked):
    //   board: 'A', CHUNK, path length, path         computer: 'D' when the answer is ready
    //   board: 'R' for every piece                   computer: [length 1 ~ CHUNK][bytes]
    //   answer: 'H', kind, encoding, rate (uint16), count (uint32), rhythmHz (uint16), data size (uint32), data
    //           or 'E', code, message length, message
    //   board: 'M', length, text                     computer: prints it (for tests)
    //
    // Link: a struct providing (see lib/Uno/Uno_serial.h)
    //   void init()
    //   bool read8(uint8_t &byte, uint16_t timeoutMs)   false: nothing came in time
    //   void write8(uint8_t byte)
    //   void clearInput()
    template <class Link>
    struct StreamLink {
        static constexpr uint16_t ANSWER_TIMEOUT_MS = 3000;
        static constexpr uint16_t CHUNK_TIMEOUT_MS = 200;
        static constexpr uint8_t HEADER_SIZE = 14;   // after 'H'

        // asks for the file at path (in flash, read with readFlash8); true when stream.py has the answer ready
        template <uint8_t (*readFlash8)(const uint8_t *)>
        static inline bool request(const char *path, uint8_t chunk);

        // asks for the next piece; length: 1 ~ chunk bytes to read with receive(); false: nothing / nonsense came
        static inline bool pullLength(uint8_t chunk, uint8_t &length);

        // one byte of the piece; false: it stopped
        static inline bool receive(uint8_t &byte);

        // a line of text that stream.py prints (at most 255 characters, in RAM)
        static inline void message(const char *text);
    };

    // ================= definitions =================

    template <class Link>
    template <uint8_t (*readFlash8)(const uint8_t *)>
    inline bool StreamLink<Link>::request(const char *path, uint8_t chunk) {
        Link::init();
        Link::clearInput();
        uint8_t length = 0;
        while (readFlash8((const uint8_t *)path + length) != '\0' && length < 255) {
            length++;
        }
        Link::write8('A');
        Link::write8(chunk);
        Link::write8(length);
        for (uint8_t i = 0; i < length; i++) {
            Link::write8(readFlash8((const uint8_t *)path + i));
        }
        uint8_t notice = 0;
        return Link::read8(notice, ANSWER_TIMEOUT_MS) && notice == 'D';
    }

    template <class Link>
    inline bool StreamLink<Link>::pullLength(uint8_t chunk, uint8_t &length) {
        Link::write8('R');
        return Link::read8(length, CHUNK_TIMEOUT_MS) && length >= 1 && length <= chunk;
    }

    template <class Link>
    inline bool StreamLink<Link>::receive(uint8_t &byte) {
        return Link::read8(byte, CHUNK_TIMEOUT_MS);
    }

    template <class Link>
    inline void StreamLink<Link>::message(const char *text) {
        uint8_t length = 0;
        while (text[length] != '\0' && length < 255) {
            length++;
        }
        Link::write8('M');
        Link::write8(length);
        for (uint8_t i = 0; i < length; i++) {
            Link::write8((uint8_t)text[i]);
        }
    }
}

#endif
