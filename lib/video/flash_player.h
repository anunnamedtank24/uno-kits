#ifndef VIDEO_FLASH_PLAYER_H
#define VIDEO_FLASH_PLAYER_H

#include "decoder.h"

namespace video {
    // plays an animation stored in flash (made by lib/video/source/convert.py), looping
    //
    // Screen: as for Decoder (ColorFormat, setWindow, writePixel ...), plus BusType::readFlash8 / readFlash16 / nowMs
    //
    // Animation: a struct providing (every struct in namespace animation, see lib/video/output/selected.h)
    //   target, width, height, scaleX, scaleY, colorMode, colorCount, frameCount
    //   getPalette(), getOffsets(), getFrames()
    //   made with --target flash (or flash stream); --target stream only has no frames here: an error
    //
    // timing: the board's own clock (Bus::nowMs), every frame is due at the sum of the delays before it
    // late by more than one frame: jumps to the last I frame that is already due (if any),
    // otherwise draws the next frame at once (the animation slows down instead of breaking)
    template <class Screen>
    class FlashPlayer {
    public:
        inline explicit FlashPlayer(Screen &screen);

        // top-left at (x, y); false if it does not fit the screen, or a scaled row is wider than
        // MAX_SCALED_WIDTH (nothing will be drawn; animations are not clipped)
        template <class Animation>
        inline bool start(int16_t x, int16_t y);

        // call as often as possible: draws the next frame when it is due, true if a frame was drawn
        inline bool update();

        inline uint16_t frame() const;

        // frames jumped over since start(), to catch up
        inline uint16_t skippedFrames() const;

    private:
        using Bus = typename Screen::BusType;

        // reads frame bytes from flash
        class Reader {
        public:
            inline explicit Reader(const uint8_t *address);
            inline uint8_t read8();

        private:
            const uint8_t *_address;
        };

        Decoder<Screen> _decoder;
        const uint8_t *_frames = nullptr;
        const uint16_t *_offsets = nullptr;
        uint16_t _frameCount = 0;
        uint16_t _frame = 0;
        uint32_t _dueMs = 0;
        bool _started = false;
        bool _playing = false;
        uint16_t _skippedFrames = 0;

        inline const uint8_t *frameAddress(uint16_t index) const;
        inline uint16_t frameDelay(uint16_t index) const;
        inline bool isIFrame(uint16_t index) const;
    };

    // ================= definitions =================

    template <class Screen>
    inline FlashPlayer<Screen>::Reader::Reader(const uint8_t *address) : _address(address) {
    }

    template <class Screen>
    inline uint8_t FlashPlayer<Screen>::Reader::read8() {
        uint8_t byte = Bus::readFlash8(_address);
        _address++;
        return byte;
    }

    template <class Screen>
    inline FlashPlayer<Screen>::FlashPlayer(Screen &screen) : _decoder(screen) {
    }

    template <class Screen>
    template <class Animation>
    inline bool FlashPlayer<Screen>::start(int16_t x, int16_t y) {
        static_assert(Animation::target & TARGET_FLASH,
                      "video::FlashPlayer: made with --target stream only (the frames stay on the computer): "
                      "play it with video::StreamPlayer, or make it with --target flash");
        static_assert(Animation::colorMode != COLORS_16 || Animation::colorCount <= MAX_RAM_COLORS,
                      "video::FlashPlayer: COLORS_16 has at most 16 colors");
        _frames = Animation::getFrames();
        _offsets = Animation::getOffsets();
        _frameCount = Animation::frameCount;
        _decoder.setColorMode(Animation::colorMode);
        if (Animation::colorMode == COLORS_16) {
            for (uint8_t i = 0; i < Animation::colorCount; i++) {
                _decoder.setColor(i, Bus::readFlash16(Animation::getPalette() + i));
            }
        } else if (Animation::colorMode == COLORS_256) {
            _decoder.setFlashPalette(Animation::getPalette());
        }
        _playing = _decoder.setArea(x, y, Animation::width, Animation::height, Animation::scaleX, Animation::scaleY);
        _started = false;
        _frame = 0;
        _skippedFrames = 0;
        return _playing;
    }

    template <class Screen>
    inline bool FlashPlayer<Screen>::update() {
        if (!_playing || _frameCount == 0) {
            return false;
        }
        uint32_t now = Bus::nowMs();

        // first frame: an I frame, due now
        if (!_started) {
            _started = true;
            _frame = 0;
            _dueMs = now;
        } else {
            if ((int32_t)(now - _dueMs) < 0) {
                return false;
            }
            uint16_t next = (uint16_t)((_frame + 1) % _frameCount);

            // late: look for the last I frame that is already due
            uint32_t due = _dueMs;
            uint16_t index = next;
            uint16_t jumpTo = next;
            uint32_t jumpDue = _dueMs;
            for (uint16_t steps = 0; steps < _frameCount; steps++) {
                uint32_t afterDue = due + frameDelay(index);
                if ((int32_t)(now - afterDue) < 0) {
                    break;
                }
                due = afterDue;
                index = (uint16_t)((index + 1) % _frameCount);
                if (isIFrame(index)) {
                    jumpTo = index;
                    jumpDue = due;
                }
            }
            if (jumpTo != next) {
                _skippedFrames += (uint16_t)((jumpTo + _frameCount - next) % _frameCount);
                _dueMs = jumpDue;
            } else if (index != next) {
                // late by a frame or more, no I frame to jump to: start the timing again from now
                _dueMs = now;
            }
            _frame = jumpTo;
        }

        Reader reader(frameAddress(_frame));
        _decoder.drawFrame(reader);
        _dueMs += frameDelay(_frame);
        return true;
    }

    template <class Screen>
    inline uint16_t FlashPlayer<Screen>::frame() const {
        return _frame;
    }

    template <class Screen>
    inline uint16_t FlashPlayer<Screen>::skippedFrames() const {
        return _skippedFrames;
    }

    template <class Screen>
    inline const uint8_t *FlashPlayer<Screen>::frameAddress(uint16_t index) const {
        return _frames + Bus::readFlash16(_offsets + index);
    }

    // the first 2 bytes of a frame
    template <class Screen>
    inline uint16_t FlashPlayer<Screen>::frameDelay(uint16_t index) const {
        const uint8_t *address = frameAddress(index);
        return Bus::readFlash8(address) | ((uint16_t)Bus::readFlash8(address + 1) << 8);
    }

    // the 3rd byte of a frame
    template <class Screen>
    inline bool FlashPlayer<Screen>::isIFrame(uint16_t index) const {
        return Bus::readFlash8(frameAddress(index) + 2) == TYPE_I;
    }
}

#endif
