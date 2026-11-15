#ifndef NEC_RECEIVER_H
#define NEC_RECEIVER_H

#include <stdint.h>

namespace NEC {
    // what a remote sent
    struct Code {
        uint16_t address;   // which remote: 0 ~ 255 (NEC), or 16 bits (extended NEC)
        uint8_t command;    // which key (see module/remote_*.h)
        uint8_t repeats;    // 0 = the press itself; 1, 2, ... = repeat codes while the key is held (stops at 255)
    };

    // the NEC protocol, read from the OUT of an infrared receiver (see module/HX1838.h)
    //
    // the receiver takes the 38 kHz light of the remote and gives OUT = LOW while the light is on
    // (ACTIVE_LOW; a receiver whose OUT is HIGH while lit: ACTIVE_LOW = false);
    // NEC (the protocol of nearly every cheap remote) is told by the lengths of the LOW (mark) and HIGH (space) parts:
    //   frame:   mark 9 ms, space 4.5 ms, 32 bits, mark 0.56 ms     (67.5 ms, once per press)
    //   bit:     mark 0.56 ms, then space 0.56 ms = 0, 1.69 ms = 1  (least significant bit first)
    //   32 bits: address, ~address (or the high byte of a 16-bit address), command, ~command
    //   repeat:  mark 9 ms, space 2.25 ms, mark 0.56 ms              (every 108 ms while the key is held)
    //
    // every change of OUT runs an interrupt that measures the part just ended and decodes it,
    // so nothing is lost while the program is busy (e.g. 64 ms of OLED display()); the codes wait in a queue
    //
    // InterruptPin: a struct providing these static functions (see lib/Uno/Uno_interrupt_pin.h)
    //   void init(void (*onChange)())   input; onChange() runs inside an interrupt on every change
    //   bool read()                     true = HIGH
    //   uint32_t nowUs()                usable inside onChange()
    //   uint32_t nowMs()
    //   uint8_t stopInterrupts()        returns what restoreInterrupts() needs
    //   void restoreInterrupts(uint8_t saved)
    //
    // everything is static (the interrupt has no object to call): one Receiver per InterruptPin type
    template <class InterruptPin, bool ACTIVE_LOW = true, uint8_t QUEUE = 4>
    class Receiver {
    public:
        // a repeat code later than this after the last code is not believed (the press it repeats was missed)
        static constexpr uint16_t REPEAT_TIMEOUT_MS = 200;

        // a key counts as held this long after its last code (repeats come every 108 ms)
        static constexpr uint16_t HOLD_TIMEOUT_MS = 150;

        // sets the pin and its interrupt
        static inline void init();

        // the oldest code not read yet; false if none
        static inline bool read(Code &code);

        // the same, but skips repeats 1 ~ repeatDelay - 1 (like a keyboard: a press, a pause, then repeats);
        // a short press often brings one repeat code (the second one comes 108 ms after the press):
        // repeatDelay = 3 makes a key held for ~0.3 s start repeating; 255 = presses only
        static inline bool read(Code &code, uint8_t repeatDelay);

        // true while a key is held (its codes keep coming)
        static inline bool isHeld();

        // frames that had the NEC timing but failed the ~command check (noise, another protocol)
        static inline uint16_t badCount();

        // forgets the codes not read yet
        static inline void clear();

        // how many times the pin changed since init() (stops at 65535): to tell wiring problems
        // (0 while a key is pressed) from protocol problems (changes, but no code)
        static inline uint16_t changeCount();

        // the interrupt function init() gives the pin: public so a test can wrap it
        // (e.g. record the pulse lengths, then call this)
        static inline void onChange();

    private:
        enum State : uint8_t { WAIT_LEADER, LEADER_SPACE, BIT_MARK, BIT_SPACE };

        static inline volatile Code _queue[QUEUE];
        static inline volatile uint8_t _head = 0;      // next to read
        static inline volatile uint8_t _count = 0;
        static inline volatile uint32_t _lastCodeMs = 0;
        static inline volatile bool _hasLast = false;
        static inline volatile uint16_t _badCount = 0;
        static inline volatile uint16_t _changeCount = 0;

        // used only inside the interrupt
        static inline uint32_t _lastEdgeUs = 0;
        static inline State _state = WAIT_LEADER;
        static inline uint8_t _bits = 0;
        static inline uint32_t _data = 0;
        static inline Code _last = {0, 0, 0};

        // a mark (LOW) or a space (HIGH) of this many µs just ended
        static inline void decode(bool mark, uint32_t us);

        static inline void finishFrame();
        static inline void push(const Code &code);
        static inline bool between(uint32_t us, uint16_t low, uint16_t high);
    };

    // ================= definitions =================

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline void Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::init() {
        static_assert(QUEUE >= 1, "NEC::Receiver: QUEUE must be at least 1");
        clear();
        _state = WAIT_LEADER;
        _lastEdgeUs = InterruptPin::nowUs();
        InterruptPin::init(&onChange);
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline bool Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::read(Code &code) {
        uint8_t saved = InterruptPin::stopInterrupts();
        bool found = _count > 0;
        if (found) {
            code.address = _queue[_head].address;
            code.command = _queue[_head].command;
            code.repeats = _queue[_head].repeats;
            _head = (uint8_t)((_head + 1) % QUEUE);
            _count--;
        }
        InterruptPin::restoreInterrupts(saved);
        return found;
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline bool Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::read(Code &code, uint8_t repeatDelay) {
        while (read(code)) {
            if (code.repeats == 0 || code.repeats >= repeatDelay) {
                return true;
            }
        }
        return false;
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline bool Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::isHeld() {
        uint8_t saved = InterruptPin::stopInterrupts();
        bool held = _hasLast && InterruptPin::nowMs() - _lastCodeMs < HOLD_TIMEOUT_MS;
        InterruptPin::restoreInterrupts(saved);
        return held;
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline uint16_t Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::badCount() {
        uint8_t saved = InterruptPin::stopInterrupts();
        uint16_t count = _badCount;
        InterruptPin::restoreInterrupts(saved);
        return count;
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline void Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::clear() {
        uint8_t saved = InterruptPin::stopInterrupts();
        _head = 0;
        _count = 0;
        InterruptPin::restoreInterrupts(saved);
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline uint16_t Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::changeCount() {
        uint8_t saved = InterruptPin::stopInterrupts();
        uint16_t count = _changeCount;
        InterruptPin::restoreInterrupts(saved);
        return count;
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline void Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::onChange() {
        if (_changeCount < 65535) {
            _changeCount++;
        }
        uint32_t now = InterruptPin::nowUs();
        uint32_t us = now - _lastEdgeUs;
        _lastEdgeUs = now;
        // back to the idle level (HIGH for ACTIVE_LOW) = a mark (light) just ended
        decode(InterruptPin::read() == ACTIVE_LOW, us);
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline void Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::decode(bool mark, uint32_t us) {
        // a 9 ms mark starts a frame or a repeat in any state (a broken frame is dropped)
        if (mark && between(us, 7000, 11000)) {
            _state = LEADER_SPACE;
            return;
        }
        switch (_state) {
        case LEADER_SPACE:
            _state = WAIT_LEADER;
            if (mark) {
                return;
            }
            if (between(us, 3500, 5500)) {
                _bits = 0;
                _data = 0;
                _state = BIT_MARK;
            } else if (between(us, 1700, 2800)) {
                uint32_t nowMs = InterruptPin::nowMs();
                if (_hasLast && nowMs - _lastCodeMs < REPEAT_TIMEOUT_MS) {
                    if (_last.repeats < 255) {
                        _last.repeats++;
                    }
                    _lastCodeMs = nowMs;
                    push(_last);
                }
            }
            return;
        case BIT_MARK:
            _state = (mark && between(us, 300, 900)) ? BIT_SPACE : WAIT_LEADER;
            return;
        case BIT_SPACE:
            if (mark) {
                _state = WAIT_LEADER;
                return;
            }
            if (between(us, 300, 900)) {
                // a 0: nothing to set
            } else if (between(us, 1200, 2200)) {
                _data |= 1UL << _bits;
            } else {
                _state = WAIT_LEADER;
                return;
            }
            _bits++;
            if (_bits == 32) {
                finishFrame();
                _state = WAIT_LEADER;
            } else {
                _state = BIT_MARK;
            }
            return;
        default:
            // WAIT_LEADER: anything but a leader mark is noise or the end mark of a frame
            return;
        }
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline void Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::finishFrame() {
        uint8_t low = (uint8_t)_data;
        uint8_t high = (uint8_t)(_data >> 8);
        uint8_t command = (uint8_t)(_data >> 16);
        uint8_t inverse = (uint8_t)(_data >> 24);
        if ((uint8_t)(command ^ inverse) != 0xFF) {
            if (_badCount < 65535) {
                _badCount++;
            }
            return;
        }
        // NEC: the second byte is ~address; extended NEC: it is the high byte of a 16-bit address
        _last.address = (uint8_t)(low ^ high) == 0xFF ? low : (uint16_t)((high << 8) | low);
        _last.command = command;
        _last.repeats = 0;
        _hasLast = true;
        _lastCodeMs = InterruptPin::nowMs();
        push(_last);
    }

    // a full queue drops the new code
    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline void Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::push(const Code &code) {
        if (_count >= QUEUE) {
            return;
        }
        uint8_t tail = (uint8_t)((_head + _count) % QUEUE);
        _queue[tail].address = code.address;
        _queue[tail].command = code.command;
        _queue[tail].repeats = code.repeats;
        _count++;
    }

    template <class InterruptPin, bool ACTIVE_LOW, uint8_t QUEUE>
    inline bool Receiver<InterruptPin, ACTIVE_LOW, QUEUE>::between(uint32_t us, uint16_t low, uint16_t high) {
        return us >= low && us <= high;
    }
}

#endif
