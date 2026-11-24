#ifndef AUDIO_SAMPLE_PLAYER_H
#define AUDIO_SAMPLE_PLAYER_H

#include <stdint.h>
#include "format.h"

namespace audio {
    // plays a PCM sound (--sound pcm --target flash) from flash, without waiting: start it, then call update()
    //
    // SampleOut: a struct providing these static functions (see lib/Uno/Uno_pcm_out.h)
    //   void init()
    //   void start(uint16_t rate, uint8_t (*next)())   next() is called rate times a second (interrupt)
    //   void stop()
    //   uint16_t realRate()
    //   uint8_t stopInterrupts()  /  void restoreInterrupts(uint8_t saved)
    //   uint8_t readFlash8(const uint8_t *)  /  uint16_t readFlash16(const uint16_t *)
    //
    // everything is static (the interrupt has no object to call): one SamplePlayer per SampleOut type
    template <class SampleOut>
    class SamplePlayer {
    public:
        // silent
        static inline void init();

        // a sound of output/selected.h: play<melody::hello_flash_pcm_8000_adpcm>()
        template <class Clip>
        static inline void play(bool repeat = false);

        // the same from a pointer: count samples of an encoding (PCM_U8 / PCM_U4 / PCM_ADPCM) at rate;
        // steps: the step table of PCM_ADPCM (the sound's getSteps()), silent without it
        static inline void play(const uint8_t *samples, uint32_t count, uint8_t encoding, uint16_t rate, bool repeat,
                                const uint16_t *steps = nullptr);

        // stops the output when the sound has ended (the interrupt only marks it); call every loop
        static inline void update();

        static inline void stop();

        // 0 = silent ~ 255 = as converted (the default), also while playing: every sample is scaled
        // around the middle (128), so 128 = -6 dB; louder than as converted needs convert.py --gain
        static inline void setVolume(uint8_t volume);
        static inline uint8_t volume();

        // true until the last sample has been played (and stop() was not called)
        static inline bool isPlaying();

        // samples played so far
        static inline uint32_t position();

    private:
        static inline const uint8_t *_samples = nullptr;
        static inline uint32_t _count = 0;
        static inline uint8_t _encoding = PCM_U8;
        static inline bool _repeat = false;
        static inline volatile uint32_t _position = 0;
        static inline volatile bool _ended = true;
        static inline bool _running = false;
        static inline uint8_t _byte = 0;
        static inline Adpcm _adpcm;
        static inline volatile uint8_t _volume = 255;

        // the interrupt: the next sample
        static inline uint8_t next();
    };

    // ================= definitions =================

    template <class SampleOut>
    inline void SamplePlayer<SampleOut>::setVolume(uint8_t volume) {
        _volume = volume;
    }

    template <class SampleOut>
    inline uint8_t SamplePlayer<SampleOut>::volume() {
        return _volume;
    }

    template <class SampleOut>
    inline void SamplePlayer<SampleOut>::init() {
        SampleOut::init();
        _running = false;
        _ended = true;
    }

    template <class SampleOut>
    template <class Clip>
    inline void SamplePlayer<SampleOut>::play(bool repeat) {
        static_assert(Clip::kind == KIND_PCM,
                      "audio::SamplePlayer: not a PCM sound (--sound pcm): notes and rhythms are played by audio::Player");
        static_assert(Clip::target & TARGET_FLASH,
                      "audio::SamplePlayer: made with --target stream only (no samples on the board): use audio::StreamSamplePlayer");
        play(Clip::getSamples(), Clip::count, Clip::encoding, Clip::rate, repeat, stepsOf<Clip>());
    }

    template <class SampleOut>
    inline void SamplePlayer<SampleOut>::play(const uint8_t *samples, uint32_t count, uint8_t encoding,
                                              uint16_t rate, bool repeat, const uint16_t *steps) {
        stop();
        if (samples == nullptr || count == 0 || encoding < PCM_U8 || encoding > PCM_ADPCM
            || (encoding == PCM_ADPCM && steps == nullptr)) {
            return;
        }
        _samples = samples;
        _count = count;
        _encoding = encoding;
        _repeat = repeat;
        _position = 0;
        _ended = false;
        _adpcm.reset(steps);
        _running = true;
        SampleOut::start(rate, &next);
    }

    template <class SampleOut>
    inline void SamplePlayer<SampleOut>::update() {
        if (_running && _ended) {
            stop();
        }
    }

    template <class SampleOut>
    inline void SamplePlayer<SampleOut>::stop() {
        SampleOut::stop();
        _running = false;
        _ended = true;
    }

    template <class SampleOut>
    inline bool SamplePlayer<SampleOut>::isPlaying() {
        return _running && !_ended;
    }

    template <class SampleOut>
    inline uint32_t SamplePlayer<SampleOut>::position() {
        uint8_t saved = SampleOut::stopInterrupts();
        uint32_t position = _position;
        SampleOut::restoreInterrupts(saved);
        return position;
    }

    template <class SampleOut>
    inline uint8_t SamplePlayer<SampleOut>::next() {
        if (_ended) {
            return PCM_SILENCE;
        }
        uint32_t position = _position;
        if (position >= _count) {
            if (!_repeat) {
                _ended = true;
                return PCM_SILENCE;
            }
            position = 0;
            _adpcm.restart();
        }
        uint8_t sample;
        if (_encoding == PCM_U8) {
            sample = SampleOut::readFlash8(_samples + position);
        } else {
            // two samples a byte, the low nibble first
            if ((position & 1) == 0) {
                _byte = SampleOut::readFlash8(_samples + (position >> 1));
            }
            uint8_t code = (position & 1) ? (uint8_t)(_byte >> 4) : (uint8_t)(_byte & 0x0F);
            sample = _encoding == PCM_U4 ? (uint8_t)(code * 17) : _adpcm.template decode<&SampleOut::readFlash16>(code);
        }
        if (_volume != 255) {
            sample = (uint8_t)(128 + (((int16_t)sample - 128) * (int16_t)(_volume + 1) >> 8));
        }
        _position = position + 1;
        return sample;
    }
}

#endif
