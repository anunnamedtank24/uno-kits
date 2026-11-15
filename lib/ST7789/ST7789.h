#ifndef ST7789_H
#define ST7789_H

// ST7789 entry point for any panel: include this file, or a module instead (it includes this file)
//
// basic/   built directly on the datasheet
//   datasheet.h  ST7789::Datasheet<Bus>  all datasheet commands + module size / offset
//   window.h     ST7789::Window<Bus>     setWindow, fillWindow, writeWindow
// tool/    optional helpers
//   init.h       ST7789::initDefault()   minimal init sequence for any panel
//
// module/  a specific module: size, offset, init sequence; every module is its own entry point
//          and a superset of this file
//   M130T.h      ST7789::M130T<Bus>      1.3" 240x240: #include "module/M130T.h" only
//
// inheritance: Datasheet -> Window -> M130T
#include "basic/datasheet.h"
#include "basic/window.h"
#include "tool/init.h"

#endif