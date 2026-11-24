#ifndef VIDEO_H
#define VIDEO_H

// video players: entry point, include this file only
//   decoder.h            video::Decoder<Screen>, HeapPalette           frame format, draws one frame
//   flash_player.h       video::FlashPlayer<Screen>                    animations in flash
//   stream_player.h      video::StreamPlayer<Screen, Stream>           animations streamed by the computer
//   output/selected.h    animation::                             the animations the program can use (both players)
//   asset.h            ANIMATION("any/path", "options"): a file converted before the build into output/, like convert.py
// colors: the screen's ColorFormat (lib/color); Stream: the link to the computer
// (lib/stream: stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>)
//
// tools (lib/video/source):
//   config.json ranges and defaults of the two targets
//   convert.py  .gif / .webp / .png, videos (.mp4 / .mov / .webm ..., ffmpeg) -> output/*.h, added to output/selected.h
//               --target flash: frames in the .h (FlashPlayer)
//               --target stream: size + path for the board, the frames after it in #if 0 (never compiled in; StreamPlayer)
//               --target flash stream: frames in the .h, within the ranges of both (both players)
//   check.py    checks output/selected.h (every include exists, current format, source unchanged)
//   stream_handler.py  what lib/stream/source/stream.py does for animations ('O' ...)
#include "decoder.h"
#include "flash_player.h"
#include "stream_player.h"
#include "output/selected.h"
#include "asset.h"

#endif
