#ifndef IMAGE_FLASH_IMAGE_H
#define IMAGE_FLASH_IMAGE_H

#include "decoder.h"

namespace image {
    // draws a picture stored in flash (made by lib/image/source/convert.py with --target uno)
    //
    // Screen: as for Decoder (writeWindow, BusType::readFlash8 / readFlash16)
    //
    // Picture: a struct providing (every struct in namespace picture, see lib/image/output/selected.h)
    //   target, width, height, scaleX, scaleY, colorMode, encoding, colorCount
    //   getPalette(), getPixels()
    //   made with --target uno (or uno mac); --target mac only has no pixels here: an error
    //
    // RAM: the decoder (2 x SEGMENT bytes + 32 bytes of palette + ~20); keep it in a function
    // (on the stack) if RAM is short, it is only needed while drawing
    template <class Screen, uint8_t SEGMENT = 32>
    class FlashImage {
    public:
        inline explicit FlashImage(Screen &screen);

        // top-left at (x, y), clipped: any x, y works
        template <class Picture>
        inline void draw(int16_t x, int16_t y);

    private:
        using Bus = typename Screen::BusType;

        // reads pixel bytes from flash
        class Reader {
        public:
            inline explicit Reader(const uint8_t *address);
            inline uint8_t read8();

        private:
            const uint8_t *_address;
        };

        Decoder<Screen, SEGMENT> _decoder;
    };

    // ================= definitions =================

    template <class Screen, uint8_t SEGMENT>
    inline FlashImage<Screen, SEGMENT>::Reader::Reader(const uint8_t *address) : _address(address) {
    }

    template <class Screen, uint8_t SEGMENT>
    inline uint8_t FlashImage<Screen, SEGMENT>::Reader::read8() {
        uint8_t byte = Bus::readFlash8(_address);
        _address++;
        return byte;
    }

    template <class Screen, uint8_t SEGMENT>
    inline FlashImage<Screen, SEGMENT>::FlashImage(Screen &screen) : _decoder(screen) {
    }

    template <class Screen, uint8_t SEGMENT>
    template <class Picture>
    inline void FlashImage<Screen, SEGMENT>::draw(int16_t x, int16_t y) {
        static_assert(Picture::target & TARGET_UNO,
                      "image::FlashImage: made with --target mac only (the pixels stay on the computer): "
                      "draw it with image::StreamImage, or make it with --target uno");
        static_assert(Picture::colorMode == COLORS_256 || Picture::colorMode == COLORS_565
                      || Picture::colorCount <= MAX_RAM_COLORS,
                      "image::FlashImage: COLORS_2 / COLORS_16 have at most 16 colors");
        _decoder.setArea(x, y, Picture::width, Picture::height, Picture::scaleX, Picture::scaleY);
        _decoder.setFormat(Picture::colorMode, Picture::encoding);
        if (Picture::colorMode == COLORS_2 || Picture::colorMode == COLORS_16) {
            for (uint8_t i = 0; i < Picture::colorCount; i++) {
                _decoder.setColor(i, Bus::readFlash16(Picture::getPalette() + i));
            }
        } else if (Picture::colorMode == COLORS_256) {
            _decoder.setFlashPalette(Picture::getPalette());
        }
        Reader reader(Picture::getPixels());
        _decoder.draw(reader);
    }
}

#endif
