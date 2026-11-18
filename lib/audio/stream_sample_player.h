#ifndef AUDIO_STREAM_SAMPLE_PLAYER_H
#define AUDIO_STREAM_SAMPLE_PLAYER_H

#include <stdint.h>
#include "format.h"

namespace audio {
    // plays a PCM sound (--sound pcm) sent by the computer (lib/stream/source/stream.py): any length,
    // no flash; open it, then call update() often: it refills the buffer whenever CHUNK bytes are free
    //
    // SampleOut: as for SamplePlayer (lib/Uno/Uno_pcm_out.h)
    // Stream: the link to stream.py (lib/stream: stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>), a struct providing
    //   CHUNK_SIZE, OK, ERROR_NOT_FOUND ~ ERROR_BROKEN, begin(), write8(), writePath(), waitAnswer(),
    //   pullLength(), receive(); CHUNK: bytes asked for at a time, at most BUFFER / 2; bigger = fewer round
    //   trips: every piece waits ~4 ms for the UNO's USB chip (measured: 64 bytes a piece = 13 KB/s, too slow
    //   for 16 kHz; 255 is best)
    // BUFFER: bytes of samples kept on the board (RAM), a power of 2, 16 ~ 1024; it must last while the
    //         program does something else and while a piece comes: 512 bytes = 32 ms of 16 kHz PCM_U8
    //
    // protocol: as audio::StreamPlayer ('A' ..., answer 'H' ... or 'E' ...)
    //
    // the buffer running empty (update() too late, or the link too slow): the sample is held
    // (a click), underruns() counts it
    //
    // everything is static (the interrupt has no object to call): one StreamSamplePlayer per SampleOut type
    template <class SampleOut, class Stream, uint16_t BUFFER = 512>
    class StreamSamplePlayer {
    public:
        static_assert(BUFFER >= 16 && BUFFER <= 1024 && (BUFFER & (BUFFER - 1)) == 0,
                      "audio::StreamSamplePlayer: BUFFER must be a power of 2, 16 ~ 1024");
        static_assert(Stream::CHUNK_SIZE <= BUFFER / 2, "audio::StreamSamplePlayer: the Stream's CHUNK must be 1 ~ BUFFER / 2");

        // results of open() and error(): Stream's
        static constexpr uint8_t OK = Stream::OK;
        static constexpr uint8_t ERROR_NOT_FOUND = Stream::ERROR_NOT_FOUND;
        static constexpr uint8_t ERROR_UNREADABLE = Stream::ERROR_UNREADABLE;
        static constexpr uint8_t ERROR_WRONG_KIND = Stream::ERROR_NOT_STREAMABLE;   // notes or a rhythm
        static constexpr uint8_t ERROR_NO_ANSWER = Stream::ERROR_NO_ANSWER;
        static constexpr uint8_t ERROR_BROKEN = Stream::ERROR_BROKEN;

        // silent
        static inline void init();

        // asks stream.py for a sound (any --target; --target stream is made for it), waits for the answer
        // (at most 3 s), fills the buffer and starts; returns OK or an error
        template <class Clip>
        static inline uint8_t open(bool repeat = false);

        // call as often as possible: pulls pieces while CHUNK bytes are free (each ~1 ms at 1 Mbaud),
        // stops (or starts over) at the end
        static inline void update();

        static inline void stop();

        // 0 = silent ~ 255 = as converted (the default), also while playing: every sample is scaled
        // around the middle (128), so 128 = -6 dB; louder than as converted needs convert.py --gain
        static inline void setVolume(uint8_t volume);
        static inline uint8_t volume();

        static inline bool isPlaying();

        // samples played so far
        static inline uint32_t position();

        // times the buffer was empty when a sample was due
        static inline uint32_t underruns();

        // the last error (OK if none): a broken stream stops the sound
        static inline uint8_t error();

        // the rate of the sound playing (from its header), 0 if none
        static inline uint16_t rate();

    private:
        static constexpr uint8_t CHUNK = Stream::CHUNK_SIZE;
        static constexpr uint16_t MASK = BUFFER - 1;

        static inline uint8_t _buffer[BUFFER];
        static inline volatile uint16_t _head = 0;          // written by update()
        static inline volatile uint16_t _tail = 0;          // read by the interrupt
        static inline uint32_t _bytesLeft = 0;              // not pulled yet
        static inline volatile uint32_t _samplesLeft = 0;
        static inline volatile uint32_t _position = 0;
        static inline volatile uint32_t _underruns = 0;
        static inline volatile bool _ended = true;
        static inline bool _running = false;
        static inline uint8_t _encoding = PCM_U8;
        static inline uint16_t _rate = 0;
        static inline uint8_t _error = OK;
        static inline const char *_path = nullptr;
        static inline bool _repeat = false;
        static inline uint8_t _byte = 0;
        static inline bool _highNibble = false;
        static inline uint8_t _last = PCM_SILENCE;
        static inline Adpcm _adpcm;
        static inline volatile uint8_t _volume = 255;

        static inline uint8_t request();
        static inline uint16_t used();
        static inline bool pull();
        static inline bool popHeader(uint8_t *header, uint8_t count);

        // the interrupt: the next sample
        static inline uint8_t next();
    };

    // ================= definitions =================

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline void StreamSamplePlayer<SampleOut, Stream, BUFFER>::setVolume(uint8_t volume) {
        _volume = volume;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline uint8_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::volume() {
        return _volume;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline void StreamSamplePlayer<SampleOut, Stream, BUFFER>::init() {
        SampleOut::init();
        _running = false;
        _ended = true;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    template <class Clip>
    inline uint8_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::open(bool repeat) {
        static_assert(Clip::kind == KIND_PCM,
                      "audio::StreamSamplePlayer: not a PCM sound (--sound pcm): use audio::StreamPlayer");
        static_assert(warn::NotForStream<!(Clip::target & TARGET_STREAM)>::ok, "");
        _path = Clip::getPath();
        _repeat = repeat;
        return request();
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline uint8_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::request() {
        stop();
        _error = OK;
        _head = 0;
        _tail = 0;
        _bytesLeft = 0xFFFFFFFFUL;      // until the header says
        if (_path == nullptr) {
            _error = ERROR_NO_ANSWER;
            return _error;
        }
        Stream::begin();
        Stream::write8('A');
        Stream::write8(CHUNK);
        Stream::writePath(_path);
        if (!Stream::waitAnswer()) {
            _error = ERROR_NO_ANSWER;
            return _error;
        }
        // the header: 'H' + 14 bytes, or 'E', code
        uint8_t header[15];
        if (!popHeader(header, 2)) {
            _error = ERROR_BROKEN;
            return _error;
        }
        if (header[0] == 'E') {
            _error = header[1];
            return _error;
        }
        if (header[0] != 'H' || !popHeader(header + 2, 13)) {
            _error = ERROR_BROKEN;
            return _error;
        }
        uint8_t kind = header[1];
        _encoding = header[2];
        _rate = (uint16_t)(header[3] | (header[4] << 8));
        uint32_t count = (uint32_t)header[5] | ((uint32_t)header[6] << 8) | ((uint32_t)header[7] << 16)
                       | ((uint32_t)header[8] << 24);
        uint32_t size = (uint32_t)header[11] | ((uint32_t)header[12] << 8) | ((uint32_t)header[13] << 16)
                      | ((uint32_t)header[14] << 24);
        if (kind != KIND_PCM) {
            _error = ERROR_WRONG_KIND;
            return _error;
        }
        if (_encoding < PCM_U8 || _encoding > PCM_ADPCM || _rate == 0) {
            _error = ERROR_BROKEN;
            return _error;
        }
        // the data already pulled with the header counts
        uint16_t got = used();
        _bytesLeft = size > got ? size - got : 0;
        _samplesLeft = count;
        _position = 0;
        _underruns = 0;
        _highNibble = false;
        _last = PCM_SILENCE;
        _adpcm.reset();
        // fill up first: the sound starts with a full buffer
        while (_bytesLeft > 0 && BUFFER - used() >= CHUNK) {
            if (!pull()) {
                _error = ERROR_BROKEN;
                return _error;
            }
        }
        if (count == 0) {
            return OK;
        }
        _ended = false;
        _running = true;
        SampleOut::start(_rate, &next);
        return OK;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline void StreamSamplePlayer<SampleOut, Stream, BUFFER>::update() {
        if (!_running) {
            return;
        }
        if (_ended) {
            if (_repeat) {
                request();
            } else {
                stop();
            }
            return;
        }
        while (_bytesLeft > 0 && BUFFER - used() >= CHUNK) {
            if (!pull()) {
                _error = ERROR_BROKEN;
                stop();
                return;
            }
        }
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline void StreamSamplePlayer<SampleOut, Stream, BUFFER>::stop() {
        SampleOut::stop();
        _running = false;
        _ended = true;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline bool StreamSamplePlayer<SampleOut, Stream, BUFFER>::isPlaying() {
        return _running && !_ended;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline uint32_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::position() {
        uint8_t saved = SampleOut::stopInterrupts();
        uint32_t position = _position;
        SampleOut::restoreInterrupts(saved);
        return position;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline uint32_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::underruns() {
        uint8_t saved = SampleOut::stopInterrupts();
        uint32_t underruns = _underruns;
        SampleOut::restoreInterrupts(saved);
        return underruns;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline uint8_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::error() {
        return _error;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline uint16_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::rate() {
        return _running ? _rate : 0;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline uint16_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::used() {
        uint8_t saved = SampleOut::stopInterrupts();
        uint16_t used = (uint16_t)(_head - _tail);
        SampleOut::restoreInterrupts(saved);
        return used;
    }

    // one piece into the buffer (the caller made sure CHUNK bytes are free)
    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline bool StreamSamplePlayer<SampleOut, Stream, BUFFER>::pull() {
        uint8_t length = 0;
        if (!Stream::pullLength(length)) {
            return false;
        }
        uint16_t head = _head;
        for (uint8_t i = 0; i < length; i++) {
            if (!Stream::receive(_buffer[head & MASK])) {
                return false;
            }
            head++;
        }
        uint8_t saved = SampleOut::stopInterrupts();
        _head = head;
        SampleOut::restoreInterrupts(saved);
        if (_bytesLeft != 0xFFFFFFFFUL) {
            _bytesLeft = length < _bytesLeft ? _bytesLeft - length : 0;
        }
        return true;
    }

    // header bytes come first in the answer: pulled into the buffer, then taken out (the output is not running)
    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline bool StreamSamplePlayer<SampleOut, Stream, BUFFER>::popHeader(uint8_t *header, uint8_t count) {
        for (uint8_t i = 0; i < count; i++) {
            if (_head == _tail && !pull()) {
                return false;
            }
            header[i] = _buffer[_tail & MASK];
            _tail = (uint16_t)(_tail + 1);
        }
        return true;
    }

    template <class SampleOut, class Stream, uint16_t BUFFER>
    inline uint8_t StreamSamplePlayer<SampleOut, Stream, BUFFER>::next() {
        if (_ended) {
            return PCM_SILENCE;
        }
        if (_samplesLeft == 0) {
            _ended = true;
            return PCM_SILENCE;
        }
        uint8_t sample;
        if (_encoding == PCM_U8 || !_highNibble) {
            uint16_t tail = _tail;
            if (tail == _head) {
                // empty: hold the last sample
                _underruns = _underruns + 1;
                return _last;
            }
            _byte = _buffer[tail & MASK];
            _tail = (uint16_t)(tail + 1);
        }
        if (_encoding == PCM_U8) {
            sample = _byte;
        } else {
            uint8_t code = _highNibble ? (uint8_t)(_byte >> 4) : (uint8_t)(_byte & 0x0F);
            _highNibble = !_highNibble;
            sample = _encoding == PCM_U4 ? (uint8_t)(code * 17) : _adpcm.template decode<&SampleOut::readFlash16>(code);
        }
        if (_volume != 255) {
            sample = (uint8_t)(128 + (((int16_t)sample - 128) * (int16_t)(_volume + 1) >> 8));
        }
        _samplesLeft = _samplesLeft - 1;
        _position = _position + 1;
        _last = sample;
        return sample;
    }
}

#endif
