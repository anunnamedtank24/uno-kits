#ifndef NEC_PROTOCOL_H
#define NEC_PROTOCOL_H

#include <stdint.h>

namespace NEC {
    // the NEC timings (both ways: Receiver reads them with ± 30 %, Transmitter sends them)
    //   frame:   mark 9 ms, space 4.5 ms, 32 bits, mark 0.56 ms     (67.5 ms, once per press)
    //   bit:     mark 0.56 ms, then space 0.56 ms = 0, 1.69 ms = 1  (least significant bit first)
    //   32 bits: address, ~address (or the high byte of a 16-bit address), command, ~command
    //   repeat:  mark 9 ms, space 2.25 ms, mark 0.56 ms              (every 108 ms while the key is held)
    //   mark = the LED flashing at 38 kHz; space = off
    namespace timing {
        constexpr uint16_t LEADER_MARK_US = 9000;
        constexpr uint16_t LEADER_SPACE_US = 4500;
        constexpr uint16_t REPEAT_SPACE_US = 2250;
        constexpr uint16_t BIT_MARK_US = 560;
        constexpr uint16_t ZERO_SPACE_US = 560;
        constexpr uint16_t ONE_SPACE_US = 1690;
        constexpr uint16_t PERIOD_MS = 108;       // from one frame / repeat start to the next
        constexpr uint8_t CARRIER_KHZ = 38;
    }
}

#endif
