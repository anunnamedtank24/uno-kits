#ifndef IMAGE_H
#define IMAGE_H

// pictures (still images): entry point, include this file only
//   decoder.h        image::Decoder<Screen, SEGMENT>, HeapPalette        pixel format, draws a picture
//   flash_image.h    image::FlashImage<Screen, SEGMENT>                 pictures in flash
//   stream_image.h   image::StreamImage<Screen, Stream, SEGMENT>        pictures sent by the computer
//   output/selected.h  picture::                                       the pictures the program can use
//   asset.h            PICTURE("any/path", "options"): a file converted before the build into output/, like convert.py
// colors: the screen's ColorFormat (lib/color); Stream: the link to the computer
// (lib/stream: stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>)
//
// tools (lib/image/source):
//   config.json ranges and defaults of the two targets
//   convert.py  .jpg / .jpeg / .png / .webp / .heic / .heif -> output/*.h, added to output/selected.h
//               --target flash: pixels in the .h (FlashImage)
//               --target stream: size + path for the board, the pixels after it in #if 0 (never compiled in; StreamImage)
//               --target flash stream: pixels in the .h, within the ranges of both
//   check.py    checks output/selected.h (every include exists, current format, source unchanged)
//   stream_handler.py  what lib/stream/source/stream.py does for pictures ('I')
#include "decoder.h"
#include "flash_image.h"
#include "stream_image.h"
#include "output/selected.h"
#include "asset.h"

#endif
