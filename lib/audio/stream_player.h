#ifndef AUDIO_STREAM_PLAYER_H
#define AUDIO_STREAM_PLAYER_H

#include <stdint.h>
#include "format.h"

namespace audio {
    // plays notes or a rhythm (--sound passive / active) sent by the computer (lib/stream/source/stream.py),
    // so a melody of any length takes no flash; open it, then call update() every loop
    //
    // Sound: as for Player (HAS_PITCH, init, start, stop, nowMs, readFlash16)
    // Stream: the link to stream.py (lib/stream: stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>, CHUNK 4 ~ 255:
    //   bytes asked for at a time; a piece is pulled when a step needs it, at most ~1 ms of waiting),
    //   a struct providing CHUNK_SIZE, OK, ERROR_NOT_FOUND ~ ERROR_BROKEN, begin(), write8(), writePath(),
    //   waitAnswer(), and a class Answer: begin(), read8(), read16(), failed()
    //
    // GAP_MS: as for Player
    //
    // protocol (after Stream's own, see lib/stream/puller.h):
    //   board: 'A', CHUNK, path length, path
    //   answer: 'H', kind, encoding, rate (uint16), count (uint32), rhythmHz (uint16), data size (uint32), data
    //           or 'E', code, message length, message
    template <class Sound, class Stream, uint8_t GAP_MS = 20>
    class StreamPlayer {
    public:
        static_assert(Stream::CHUNK_SIZE >= 4, "audio::StreamPlayer: the Stream's CHUNK must be 4 ~ 255");

        // results of open() and error(): Stream's
        static constexpr uint8_t OK = Stream::OK;
        static constexpr uint8_t ERROR_NOT_FOUND = Stream::ERROR_NOT_FOUND;
        static constexpr uint8_t ERROR_UNREADABLE = Stream::ERROR_UNREADABLE;
        static constexpr uint8_t ERROR_WRONG_KIND = Stream::ERROR_NOT_STREAMABLE;   // a PCM sound
        static constexpr uint8_t ERROR_NO_ANSWER = Stream::ERROR_NO_ANSWER;
        static constexpr uint8_t ERROR_BROKEN = Stream::ERROR_BROKEN;

        // silent
        inline void init();

        // asks stream.py for a melody (any --target; --target stream is made for it), waits for the answer
        // (at most 3 s) and starts it; returns OK or an error
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

        // the last error (OK if none): a broken stream stops the melody
        inline uint8_t error() const;

    private:
        const char *_path = nullptr;
        bool _repeat = false;
        bool _playing = false;
        uint8_t _error = OK;
        uint8_t _encoding = NOTES_HZ_MS;
        uint16_t _rhythmHz = 0;
        uint32_t _count = 0;
        uint32_t _index = 0;
        uint16_t _hz = 0;
        uint32_t _stepStartMs = 0;
        uint16_t _stepMs = 0;
        uint16_t _soundMs = 0;

        typename Stream::Answer _answer;

        inline uint8_t request();
        inline bool startStep();
        inline void sound(uint16_t hz);
        inline void silence();
        inline void fail(uint8_t error);
    };

    // ================= definitions =================

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Stream, GAP_MS>::setVolume(uint8_t volume) {
        Sound::setVolume(volume);
        // from 0 back up in the middle of a note: the Sound had stopped, start it again
        if (volume != 0 && _hz != 0) {
            Sound::start(_hz);
        }
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline uint8_t StreamPlayer<Sound, Stream, GAP_MS>::volume() const {
        return Sound::volume();
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Stream, GAP_MS>::init() {
        Sound::init();
        _playing = false;
        _hz = 0;
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    template <class Melody>
    inline uint8_t StreamPlayer<Sound, Stream, GAP_MS>::open(bool repeat) {
        static_assert(Melody::kind != KIND_PCM,
                      "audio::StreamPlayer: a PCM sound (--sound pcm) is played by audio::StreamSamplePlayer");
        static_assert(warn::NotForStream<!(Melody::target & TARGET_STREAM)>::ok, "");
        static_assert(warn::PitchLost<(Melody::kind == KIND_NOTES && !Sound::HAS_PITCH)>::ok, "");
        _path = Melody::getPath();
        _repeat = repeat;
        return request();
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline uint8_t StreamPlayer<Sound, Stream, GAP_MS>::request() {
        silence();
        _playing = false;
        _error = OK;
        if (_path == nullptr) {
            _error = ERROR_NO_ANSWER;
            return _error;
        }
        Stream::begin();
        Stream::write8('A');
        Stream::write8(Stream::CHUNK_SIZE);
        Stream::writePath(_path);
        if (!Stream::waitAnswer()) {
            _error = ERROR_NO_ANSWER;
            return _error;
        }
        _answer.begin();
        uint8_t answer = _answer.read8();
        if (answer == 'E') {
            uint8_t code = _answer.read8();
            _error = _answer.failed() ? ERROR_BROKEN : code;
            return _error;
        }
        uint8_t kind = _answer.read8();
        _encoding = _answer.read8();
        _answer.read16();                   // rate: PCM only
        _count = _answer.read32();
        _rhythmHz = _answer.read16();
        _answer.read32();                   // data size: the computer knows where it ends
        if (_answer.failed() || answer != 'H' || _encoding > RHYTHM_MS) {
            _error = ERROR_BROKEN;
            return _error;
        }
        if (kind == KIND_PCM) {
            _error = ERROR_WRONG_KIND;
            return _error;
        }
        if (_count == 0) {
            return OK;
        }
        _index = 0;
        _playing = true;
        _stepStartMs = Sound::nowMs();
        if (!startStep()) {
            return _error;
        }
        return OK;
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Stream, GAP_MS>::update() {
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

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Stream, GAP_MS>::stop() {
        _playing = false;
        silence();
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline bool StreamPlayer<Sound, Stream, GAP_MS>::isPlaying() const {
        return _playing;
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline uint16_t StreamPlayer<Sound, Stream, GAP_MS>::currentHz() const {
        return _hz;
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline uint32_t StreamPlayer<Sound, Stream, GAP_MS>::noteIndex() const {
        return _index;
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline uint8_t StreamPlayer<Sound, Stream, GAP_MS>::error() const {
        return _error;
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline bool StreamPlayer<Sound, Stream, GAP_MS>::startStep() {
        uint16_t first = _answer.read16();
        uint16_t second = _encoding == NOTES_HZ_MS ? _answer.read16() : 0;
        if (_answer.failed()) {
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

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Stream, GAP_MS>::sound(uint16_t hz) {
        if (hz != _hz) {
            Sound::start(hz);
            _hz = hz;
        }
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Stream, GAP_MS>::silence() {
        Sound::stop();
        _hz = 0;
    }

    template <class Sound, class Stream, uint8_t GAP_MS>
    inline void StreamPlayer<Sound, Stream, GAP_MS>::fail(uint8_t error) {
        _error = error;
        stop();
    }
}

#endif
