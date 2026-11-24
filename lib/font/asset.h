#ifndef FONT_ASSET_H
#define FONT_ASSET_H

#include <stdint.h>

// FONT(path, options): a font (.ttf / .otf) from anywhere, converted when the program is built
//
//   using Title = FONT("~/Fonts/Inter.ttf", "--height 16");
//   text.setFont<FONT("~/Fonts/Inter.ttf", "--height 16")>();
//
// path:    absolute, ~/..., or relative to the .cpp file that names it (also tried: the project folder,
//          lib/font/assets/); any file the converter reads
// options: one string, exactly the options of lib/font/source/convert.py (see its README); "" or left out:
//          its defaults (--height 16)
//
// how: before every build, src/tools/assets.py (platformio.ini: extra_scripts) finds each FONT(...) in src/,
// looks in lib/font/output/ for the .h made from that file with those options (its first lines say so),
// runs convert.py only if there is none (the same output as by hand: lib/font/output/<name>.h), and writes
// output/registry.h, which gives font::Asset<key>::Type for each one; the key is a hash of the two
// strings, computed here at compile time the same way
// the name: the file name for a file of lib/font/assets/, else its folders first (e.g. src_photos_cat_...),
// so files with the same name never mix; the struct can also be used by that name, font::<name>
// both arguments must be plain string literals (no variables, no macros): the script reads the source text
// nothing converted is compiled in if it is not used (like every type in output/)
namespace font {
    // FNV-1a (32 bits) of path, 0x1F, options: the same as src/tools/assets.py
    constexpr uint32_t assetHash(const char *text, uint32_t hash) {
        while (*text != '\0') {
            hash = (hash ^ (uint8_t)*text) * 16777619UL;
            text++;
        }
        return hash;
    }

    constexpr uint32_t assetKey(const char *path, const char *options) {
        return assetHash(options, (assetHash(path, 2166136261UL) ^ 0x1FUL) * 16777619UL);
    }

    // filled in by output/registry.h; an unknown key: not converted (yet)
    template <uint32_t KEY>
    struct Asset {
        static_assert(KEY != KEY, "FONT(...): not converted. Build with PlatformIO: src/tools/assets.py "
                                  "(platformio.ini extra_scripts) converts it first; see its messages above");
        using Type = void;
    };
}

#define FONT(path, ...) ::font::Asset<::font::assetKey(path, "" __VA_ARGS__)>::Type

#include "output/registry.h"

#endif
