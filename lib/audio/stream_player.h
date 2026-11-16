#ifndef AUDIO_STREAM_PLAYER_H
#define AUDIO_STREAM_PLAYER_H

#include <stdint.h>
#include "format.h"
#include "stream_link.h"

namespace audio {
    // plays notes or a rhythm (--sound passive / active) sent by the computer (lib/audio/source/stream.py),
    // so a melody of any length takes no flash; open it, then call update() every loop
    //
    // Sound: as for Player (HAS_PITCH, init, start, stop, nowMs, readFlash16), plus
    //   uint8_t readFlash8(const uint8_t *)   reads the path
    // Link: see stream_link.h (Uno::UnoSerial)
    // CHUNK: bytes asked for at a time (RAM); a piece is pulled when a step needs it (at most ~1 ms of waiting)
    //
    // GAP_MS: as for Player
    template <class Sound, class Link, uint8_t CHUNK = 32, uint8_t GAP_MS = 20>
    class StreamPlayer {
    public:
        static_assert(CHUNK >= 4, "audio::StreamPlayer: CHUNK must be 4 ~ 255");

        // silent
        inline void init();

        // asks stream.py for a melody (any --target; --target mac is made for it), waits for the answer
        // (at most 3 s) and starts it; returns STREAM_OK or an error (stream_link.h)
        template <class Melody>
        inline uint8_t open(bool repeat = false);

        // call as often as possible: the next step when its time comes (pulling a piece if needed)
        inline void update();

        inline void stop();

        // 0 = silent ~ 255 = the loudest (the default after init()), also while playing:
        // a passive buzzer gets a narrower pulse (-6 dB at 128); an active buzzer has only 0 = muted, else on
        inline void setVolume(uint8_t volume);
        inline uint8_t volume() const;

        inline bool isPlaying() const;
        inline uint16_t currentHz() const;

        // the step playing now: 0 = first
        inline uint32_t noteIndex() const;

        // the last error (STREAM_OK if none): a broken stream stops the melody
        inline uint8_t error() const;

    private:
        using Stream = StreamLink<Link>;

        const char *_path = nullptr;
        bool _repeat = false;
        bool _playing = false;
        uint8_t _error = STREAM_OK;
        uint8_t _encoding = NOTES_HZ_MS;
        uint16_t _rhythmHz = 0;
        uint32_t _count = 0;
        uint32_t _index = 0;
        uint16_t _hz = 0;
        uint32_t _stepStartMs = 0;
        uint16_t _stepMs = 0;
        uint16_t _soundMs = 0;

        uint8_t _buffer[CHUNK];
        uint8_t _length = 0;
        uint8_t _position = 0;
        bool _failed = false;

        inline uint8_t request();
        inline uint8_t read8();
        inline uint16_t read16();
        inline bool startStep();
        inline void sound(uint16_t hz);
        inline void silence();
        inline void fail(uint8_t error);
    };

    // ================= definitions =================

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Link, CHUNK, GAP_MS>::setVolume(uint8_t volume) {
        Sound::setVolume(volume);
        // from 0 back up in the middle of a note: the Sound had stopped, start it again
        if (volume != 0 && _hz != 0) {
            Sound::start(_hz);
        }
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline uint8_t StreamPlayer<Sound, Link, CHUNK, GAP_MS>::volume() const {
        return Sound::volume();
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Link, CHUNK, GAP_MS>::init() {
        Sound::init();
        _playing = false;
        _hz = 0;
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    template <class Melody>
    inline uint8_t StreamPlayer<Sound, Link, CHUNK, GAP_MS>::open(bool repeat) {
        static_assert(Melody::kind != KIND_PCM,
                      "audio::StreamPlayer: a PCM sound (--sound pcm) is played by audio::StreamSamplePlayer");
        static_assert(warn::NotForMac<!(Melody::target & TARGET_MAC)>::ok, "");
        static_assert(warn::PitchLost<(Melody::kind == KIND_NOTES && !Sound::HAS_PITCH)>::ok, "");
        _path = Melody::getPath();
        _repeat = repeat;
        return request();
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline uint8_t StreamPlayer<Sound, Link, CHUNK, GAP_MS>::request() {
        silence();
        _playing = false;
        _error = STREAM_OK;
        _failed = false;
        _length = 0;
        _position = 0;
        if (_path == nullptr || !Stream::template request<&Sound::readFlash8>(_path, CHUNK)) {
            _error = ERROR_NO_ANSWER;
            return _error;
        }
        uint8_t answer = read8();
        if (answer == 'E') {
            uint8_t code = read8();
            _error = _failed ? ERROR_BROKEN : code;
            return _error;
        }
        uint8_t kind = read8();
        _encoding = read8();
        read16();                           // rate: PCM only
        _count = read16();
        _count |= (uint32_t)read16() << 16;
        _rhythmHz = read16();
        read16();                           // data size: the computer knows where it ends
        read16();
        if (_failed || answer != 'H' || _encoding > RHYTHM_MS) {
            _error = ERROR_BROKEN;
            return _error;
        }
        if (kind == KIND_PCM) {
            _error = ERROR_WRONG_KIND;
            return _error;
        }
        if (_count == 0) {
            return STREAM_OK;
        }
        _index = 0;
        _playing = true;
        _stepStartMs = Sound::nowMs();
        if (!startStep()) {
            return _error;
        }
        return STREAM_OK;
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Link, CHUNK, GAP_MS>::update() {
        if (!_playing) {
            return;
        }
        uint32_t elapsed = Sound::nowMs() - _stepStartMs;
        if (_hz != 0 && elapsed >= _soundMs) {
            silence();
        }
        if (elapsed < _stepMs) {
            return;
        }
        _stepStartMs += _stepMs;
        _index++;
        if (_index >= _count) {
            if (_repeat) {
                request();
            } else {
                stop();
            }
            return;
        }
        startStep();
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Link, CHUNK, GAP_MS>::stop() {
        _playing = false;
        silence();
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline bool StreamPlayer<Sound, Link, CHUNK, GAP_MS>::isPlaying() const {
        return _playing;
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline uint16_t StreamPlayer<Sound, Link, CHUNK, GAP_MS>::currentHz() const {
        return _hz;
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline uint32_t StreamPlayer<Sound, Link, CHUNK, GAP_MS>::noteIndex() const {
        return _index;
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline uint8_t StreamPlayer<Sound, Link, CHUNK, GAP_MS>::error() const {
        return _error;
    }

    // the next byte of the answer, pulling a piece when the buffer is used up; 0 after a failure
    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline uint8_t StreamPlayer<Sound, Link, CHUNK, GAP_MS>::read8() {
        if (_failed) {
            return 0;
        }
        if (_position == _length) {
            uint8_t length = 0;
            if (!Stream::pullLength(CHUNK, length)) {
                _failed = true;
                return 0;
            }
            for (uint8_t i = 0; i < length; i++) {
                if (!Stream::receive(_buffer[i])) {
                    _failed = true;
                    return 0;
                }
            }
            _length = length;
            _position = 0;
        }
        return _buffer[_position++];
    }

    // little-endian
    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline uint16_t StreamPlayer<Sound, Link, CHUNK, GAP_MS>::read16() {
        uint16_t value = read8();
        return (uint16_t)(value | ((uint16_t)read8() << 8));
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline bool StreamPlayer<Sound, Link, CHUNK, GAP_MS>::startStep() {
        uint16_t first = read16();
        uint16_t second = _encoding == NOTES_HZ_MS ? read16() : 0;
        if (_failed) {
            fail(ERROR_BROKEN);
            return false;
        }
        uint16_t hz;
        decodeStep(_encoding, first, second, _rhythmHz, hz, _stepMs);
        bool gap = _encoding != RHYTHM_MS && _stepMs >= 2 * GAP_MS;
        _soundMs = gap ? (uint16_t)(_stepMs - GAP_MS) : _stepMs;
        if (hz != REST && _soundMs > 0) {
            sound(hz);
        } else {
            silence();
        }
        return true;
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Link, CHUNK, GAP_MS>::sound(uint16_t hz) {
        if (hz != _hz) {
            Sound::start(hz);
            _hz = hz;
        }
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Link, CHUNK, GAP_MS>::silence() {
        Sound::stop();
        _hz = 0;
    }

    template <class Sound, class Link, uint8_t CHUNK, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Link, CHUNK, GAP_MS>::fail(uint8_t error) {
        _error = error;
        stop();
    }
}

#endif
