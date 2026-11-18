#ifndef NEC_REMOTE_17_KEY_H
#define NEC_REMOTE_17_KEY_H

#include "../key.h"
#include "../receiver.h"

namespace NEC {
    // the slim 17-key remotes of Arduino kits (address 0x00, a CR2025 coin cell):
    // 0 ~ 9, *, #, the four arrows and OK
    // they come in two layouts that send different commands: check yours with src/week_5_NEC/tests/1_receiver.cpp
    //
    // Remote: a struct providing
    //   static constexpr uint16_t ADDRESS
    //   Key toKey(const Code &code)       KEY_NONE for another address or an unknown command
    //   bool toCommand(Key key, uint8_t &command)   the other way, to send it (Transmitter); false: no such key

    //   1   2   3          the commands (hex) of this layout:
    //   4   5   6            45 46 47
    //   7   8   9            44 40 43
    //   *   0   #            07 15 09
    //       ▲                16 19 0D
    //   ◀  OK   ▶               18
    //       ▼                08 1C 5A
    //                           52
    struct Remote17KeyNumbersTop {
        static constexpr uint16_t ADDRESS = 0x00;

        static inline Key toKey(const Code &code);

        // the other way (to send it, NEC::Transmitter): false if this remote has no such key
        static inline bool toCommand(Key key, uint8_t &command);
    };

    //       ▲              the commands (hex) of this layout:
    //   ◀  OK   ▶               46
    //       ▼                44 40 43
    //   1   2   3               15
    //   4   5   6            16 19 0D
    //   7   8   9            0C 18 5E
    //   *   0   #            08 1C 5A
    //                        42 52 4A
    struct Remote17KeyArrowsTop {
        static constexpr uint16_t ADDRESS = 0x00;

        static inline Key toKey(const Code &code);

        // the other way (to send it, NEC::Transmitter): false if this remote has no such key
        static inline bool toCommand(Key key, uint8_t &command);
    };

    namespace remote_17_key {
        // the same keys, in the same order, for both layouts
        constexpr Key KEYS[17] = {
            KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_STAR, KEY_0, KEY_HASH,
            KEY_UP, KEY_LEFT, KEY_OK, KEY_RIGHT, KEY_DOWN,
        };

        // the commands of each layout, in the order of KEYS
        constexpr uint8_t NUMBERS_TOP[17] = {
            0x45, 0x46, 0x47, 0x44, 0x40, 0x43, 0x07, 0x15, 0x09, 0x16, 0x19, 0x0D,
            0x18, 0x08, 0x1C, 0x5A, 0x52,
        };
        constexpr uint8_t ARROWS_TOP[17] = {
            0x16, 0x19, 0x0D, 0x0C, 0x18, 0x5E, 0x08, 0x1C, 0x5A, 0x42, 0x52, 0x4A,
            0x46, 0x44, 0x40, 0x43, 0x15,
        };

        // the key whose command is in commands[] (in the order of KEYS); KEY_NONE for another address
        inline Key find(const Code &code, uint16_t address, const uint8_t *commands);

        // the command of key in commands[]; false if not a key of KEYS
        inline bool commandOf(Key key, const uint8_t *commands, uint8_t &command);
    }

    // ================= definitions =================

    namespace remote_17_key {
        inline Key find(const Code &code, uint16_t address, const uint8_t *commands) {
            if (code.address != address) {
                return KEY_NONE;
            }
            for (uint8_t i = 0; i < 17; i++) {
                if (commands[i] == code.command) {
                    return KEYS[i];
                }
            }
            return KEY_NONE;
        }
    }

    namespace remote_17_key {
        inline bool commandOf(Key key, const uint8_t *commands, uint8_t &command) {
            for (uint8_t i = 0; i < 17; i++) {
                if (KEYS[i] == key) {
                    command = commands[i];
                    return true;
                }
            }
            return false;
        }
    }

    inline Key Remote17KeyNumbersTop::toKey(const Code &code) {
        return remote_17_key::find(code, ADDRESS, remote_17_key::NUMBERS_TOP);
    }

    inline bool Remote17KeyNumbersTop::toCommand(Key key, uint8_t &command) {
        return remote_17_key::commandOf(key, remote_17_key::NUMBERS_TOP, command);
    }

    inline Key Remote17KeyArrowsTop::toKey(const Code &code) {
        return remote_17_key::find(code, ADDRESS, remote_17_key::ARROWS_TOP);
    }

    inline bool Remote17KeyArrowsTop::toCommand(Key key, uint8_t &command) {
        return remote_17_key::commandOf(key, remote_17_key::ARROWS_TOP, command);
    }
}

#endif
