#ifndef IMAGE_H
#define IMAGE_H

// pictures (still images): entry point, include this file only
//   decoder.h        image::Decoder<Screen, SEGMENT>                    pixel format, draws a picture
//   flash_image.h    image::FlashImage<Screen, SEGMENT>                 pictures in flash
//   stream_image.h   image::StreamImage<Screen, Link, CHUNK, SEGMENT>   pictures sent by the computer
//   output/selected.h  picture::                                       the pictures the program can use
//
// tools (lib/image/source):
//   config.json ranges and defaults of the two targets
//   convert.py  .jpg / .jpeg / .png / .webp / .heic / .heif -> output/*.h, added to output/selected.h
//               --target uno: pixels in the .h (FlashImage)
//               --target mac: pixels in output/mac/*.h (never compiled in), the .h has size + path (StreamImage)
//               --target uno mac: pixels in the .h, within the ranges of both
//   check.py    checks output/selected.h (every include exists, current format, source unchanged)
//   stream.py   no arguments: finds the board, sends whatever picture it asks for (pip install pyserial)
#include "decoder.h"
#include "flash_image.h"
#include "stream_image.h"
#include "output/selected.h"

#endif
