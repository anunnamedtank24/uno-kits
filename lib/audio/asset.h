#ifndef AUDIO_ASSET_H
#define AUDIO_ASSET_H

#include <stdint.h>

// MELODY(path, options): a sound (.wav / .mp3 / .m4a / .ogg / .aiff ...) from anywhere, converted when the program is built
//
//   using Title = MELODY("songs/tune.mp3", "--sound passive --quality high");
//   speaker.play<MELODY("songs/tune.mp3")>();
//
// path:    absolute, ~/..., or relative to the .cpp file that names it (also tried: the project folder,
//          lib/audio/assets/); any file the converter reads
// options: one string, exactly the options of lib/audio/source/convert.py (see its README); "" or left out:
//          its defaults (--target flash --sound passive --quality medium)
//
// how: before every build, tools/assets.py (platformio.ini: extra_scripts) finds each MELODY(...) in src/,
// looks in lib/audio/output/ for the .h made from that file with those options (its first lines say so),
// runs convert.py only if there is none (the same output as by hand: lib/audio/output/<name>.h), and writes
// output/registry.h, which gives melody::Asset<key>::Type for each one; the key is a hash of the two
// strings, computed here at compile time the same way
// the name: the file name for a file of lib/audio/assets/, else its folders first (e.g. src_photos_cat_...),
// so files with the same name never mix; the struct can also be used by that name, melody::<name>
// both arguments must be plain string literals (no variables, no macros): the script reads the source text
// nothing converted is compiled in if it is not used (like every type in output/)
namespace melody {
    // FNV-1a (32 bits) of path, 0x1F, options: the same as tools/assets.py
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
        static_assert(KEY != KEY, "MELODY(...): not converted. Build with PlatformIO: tools/assets.py "
                                  "(platformio.ini extra_scripts) converts it first; see its messages above");
        using Type = void;
    };
}

#define MELODY(path, ...) ::melody::Asset<::melody::assetKey(path, "" __VA_ARGS__)>::Type

#include "output/registry.h"

#endif
