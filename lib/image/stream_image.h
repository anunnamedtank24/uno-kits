#ifndef IMAGE_STREAM_IMAGE_H
#define IMAGE_STREAM_IMAGE_H

#include "decoder.h"

namespace image {
    // draws a picture sent by the computer (lib/stream/source/stream.py)
    //
    // Screen: as for Decoder (ColorFormat, writeWindow)
    // Stream: the link to stream.py (lib/stream: stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>), a struct providing
    //   OK, ERROR_NOT_FOUND ~ ERROR_BROKEN (uint8_t), begin(), write8(), writePath(flash path),
    //   waitAnswer(), and a class Answer: begin(), read8(), read16(), read32(), failed()
    //
    // Picture: a struct in namespace picture with getPath() (any target; --target stream is made for it)
    //   COLORS_256: a compile warning: the palette comes first and is kept in RAM taken at run time
    //   (HeapPalette: colorCount x the screen's color size, e.g. 512 bytes for 256 RGB565 colors), given back
    //   when draw() returns; not enough RAM: ERROR_NO_MEMORY, nothing drawn
    //
    // protocol (after Stream's own, see lib/stream/puller.h):
    //   board: 'I', CHUNK, path length, path
    //   answer: 'H', width, height, scaleX, scaleY, colorMode, encoding, colorCount (0 = 256),
    //           data size (uint32_t), palette (colorCount x RGB565; none for COLORS_565), pixel data
    //           or 'E', code, message length, message
    //
    // draw() waits until the whole picture is drawn (or something fails): a 240 x 240 RGB565
    // picture is ~115 KB, about 1.5 s at 1 Mbaud
    //
    // RAM: Stream::CHUNK_SIZE + the decoder (SEGMENT + 16 colors + ~20 bytes); COLORS_256 also the HeapPalette
    // while drawing
    template <class Screen, class Stream, uint8_t SEGMENT = 32>
    class StreamImage {
    public:
        // results of draw(): Stream's (OK, or ERROR_NOT_FOUND ~ ERROR_BROKEN)
        static constexpr uint8_t OK = Stream::OK;
        static constexpr uint8_t ERROR_NOT_FOUND = Stream::ERROR_NOT_FOUND;
        static constexpr uint8_t ERROR_UNREADABLE = Stream::ERROR_UNREADABLE;
        static constexpr uint8_t ERROR_NOT_STREAMABLE = Stream::ERROR_NOT_STREAMABLE;
        static constexpr uint8_t ERROR_NO_ANSWER = Stream::ERROR_NO_ANSWER;
        static constexpr uint8_t ERROR_BROKEN = Stream::ERROR_BROKEN;
        static constexpr uint8_t ERROR_NO_MEMORY = Stream::ERROR_NO_MEMORY;   // a 256-color palette did not fit in RAM

        inline StreamImage(Screen &screen);

        // top-left at (x, y), clipped: any x, y works; returns OK or an error
        template <class Picture>
        inline uint8_t draw(int16_t x, int16_t y);

    private:
        using ColorFormat = typename Decoder<Screen, SEGMENT>::ColorFormat;
        using Color = typename ColorFormat::Value;

        Decoder<Screen, SEGMENT> _decoder;
        typename Stream::Answer _answer;

        inline uint8_t drawPath(const char *path, int16_t x, int16_t y);
    };

    // ================= definitions =================

    template <class Screen, class Stream, uint8_t SEGMENT>
    inline StreamImage<Screen, Stream, SEGMENT>::StreamImage(Screen &screen) : _decoder(screen) {
    }

    template <class Screen, class Stream, uint8_t SEGMENT>
    template <class Picture>
    inline uint8_t StreamImage<Screen, Stream, SEGMENT>::draw(int16_t x, int16_t y) {
        static_assert(warn::PaletteInRam<Picture::colorMode == COLORS_256>::ok, "");
        static_assert(warn::NotForStream<!(Picture::target & TARGET_STREAM)>::ok, "");
        return drawPath(Picture::getPath(), x, y);
    }

    template <class Screen, class Stream, uint8_t SEGMENT>
    inline uint8_t StreamImage<Screen, Stream, SEGMENT>::drawPath(const char *path, int16_t x, int16_t y) {
        Stream::begin();
        Stream::write8('I');
        Stream::write8(Stream::CHUNK_SIZE);
        Stream::writePath(path);
        if (!Stream::waitAnswer()) {
            return ERROR_NO_ANSWER;
        }
        _answer.begin();
        uint8_t kind = _answer.read8();
        if (kind == 'E') {
            uint8_t code = _answer.read8();
            return _answer.failed() ? ERROR_BROKEN : code;
        }
        if (kind != 'H') {
            return ERROR_BROKEN;
        }
        uint8_t width = _answer.read8();
        uint8_t height = _answer.read8();
        uint8_t scaleX = _answer.read8();
        uint8_t scaleY = _answer.read8();
        uint8_t colorMode = _answer.read8();
        uint8_t encoding = _answer.read8();
        uint8_t countByte = _answer.read8();
        _answer.read32(); // data size: the computer knows when it ends
        uint16_t colorCount = colorMode == COLORS_256 && countByte == 0 ? 256 : countByte;
        if (_answer.failed() || colorMode > COLORS_565 || encoding > ENCODING_RUNS
            || (colorMode != COLORS_256 && colorCount > MAX_RAM_COLORS)) {
            return ERROR_BROKEN;
        }
        // COLORS_256: the palette goes to RAM taken now, given back when this function returns
        HeapPalette<Color> heapPalette;
        if (colorMode == COLORS_256) {
            if (!heapPalette.take(colorCount)) {
                return ERROR_NO_MEMORY;
            }
            for (uint16_t i = 0; i < colorCount; i++) {
                heapPalette.colors()[i] = ColorFormat::fromRGB565(_answer.read16());
            }
            _decoder.setRamPalette(heapPalette.colors());
        } else {
            for (uint8_t i = 0; i < colorCount; i++) {
                _decoder.setColor(i, _answer.read16());
            }
        }
        _decoder.setArea(x, y, width, height, scaleX, scaleY);
        _decoder.setFormat(colorMode, encoding);
        // the rest reads as 0 if the link breaks: the picture is finished with black, draw() reports it
        _decoder.draw(_answer);
        return _answer.failed() ? ERROR_BROKEN : OK;
    }
}

#endif
