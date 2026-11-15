#ifndef NEC_KEY_H
#define NEC_KEY_H

#include <stdint.h>

namespace NEC {
    // keys by meaning, the same for every remote: a remote (module/remote_*.h) turns its codes into these,
    // so a program written for one remote works with another by changing one line
    // a remote with more keys adds them at the end
    enum Key : uint8_t {
        KEY_NONE,           // not a key of this remote (or another remote)
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK,
        KEY_STAR,           // *
        KEY_HASH,           // #
    };

    // 0 ~ 9 for KEY_0 ~ KEY_9, -1 for the others
    inline int8_t keyDigit(Key key);

    // a short name ("1", "UP", "*" ...) for printing
    inline const char *keyName(Key key);

    // ================= definitions =================

    inline int8_t keyDigit(Key key) {
        if (key >= KEY_0 && key <= KEY_9) {
            return (int8_t)(key - KEY_0);
        }
        return -1;
    }

    inline const char *keyName(Key key) {
        static const char *const NAMES[] = {
            "none", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
            "UP", "DOWN", "LEFT", "RIGHT", "OK", "*", "#",
        };
        if (key > KEY_HASH) {
            return "?";
        }
        return NAMES[key];
    }
}

#endif
