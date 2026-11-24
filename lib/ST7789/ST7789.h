#ifndef ST7789_H
#define ST7789_H

// ST7789 entry point: include this file only (it includes every module)
//
// basic/   built directly on the datasheet
//   datasheet.h  ST7789::Datasheet<Bus>               all datasheet commands + module size / offset
//   window.h     ST7789::Window<Bus, ColorFormat>     color mode, setWindow, writePixel, fillWindow, writeWindow
// tool/    optional helpers
//   init.h       ST7789::initDefault(screen, blk)     minimal init sequence for any panel
//
// module/  the boards you buy: size, offset, init sequence
//   M130T.h      ST7789::M130T<Bus, ColorFormat>      1.3" 240x240
//
// inheritance: Datasheet -> Window -> M130T
#include "basic/datasheet.h"
#include "basic/window.h"
#include "tool/init.h"
#include "module/M130T.h"

#endif
