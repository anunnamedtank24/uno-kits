#ifndef GIF_H
#define GIF_H

// GIF players: entry point, include this file only
//   decoder.h            GIF::Decoder<Screen>                    frame format, draws one frame
//   flash_player.h        GIF::FlashPlayer<Screen>                animations in flash
//   stream_player.h       GIF::StreamPlayer<Screen, Link, CHUNK>  animations streamed by the computer
//   output/selected.h    animation::                             the animations the program can use (both players)
//
// tools (lib/GIF/source):
//   config.json ranges and defaults of the two targets
//   convert.py  .gif -> output/*.h, added to output/selected.h
//               --target uno: frames in the .h (FlashPlayer)
//               --target mac: frames in output/mac/*.h (never compiled in), the .h has size + path (StreamPlayer)
//               --target uno mac: frames in the .h, within the ranges of both (both players)
//   check.py    checks output/selected.h (every include exists, current format, source unchanged)
//   stream.py   no arguments: finds the board, sends whatever file it opens (pip install pyserial)
#include "decoder.h"
#include "flash_player.h"
#include "stream_player.h"
#include "output/selected.h"

#endif
