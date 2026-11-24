#ifndef FONT_H
#define FONT_H

// font entry point: include this file
// every font is a struct in namespace font, made by source/convert.py into output/ (the ones the program
// names are listed in output/selected.h by src/tools/assets.py), or straight from a file:
//   FONT("any/path.ttf", "--height 16")   converted before the build into output/ (asset.h)
#include "output/selected.h"
#include "asset.h"

#endif
