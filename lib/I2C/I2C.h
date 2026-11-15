#ifndef I2C_H
#define I2C_H

// I2C OLED entry point (SSD1306 controller): include this file, or a module instead (it includes this file)
//
// basic/   built directly on the datasheet
//   datasheet.h  SSD1306::Datasheet<Bus>                 all datasheet commands, status read
//   frame.h      SSD1306::Frame<Bus, WIDTH, HEIGHT>      picture in RAM, fillWindow / writeWindow, display()
//
// module/  a specific module: size, init sequence; every module is its own entry point
//          and a superset of this file
//   OLED_128x64.h  SSD1306::OLED128x64<Bus>   0.96" 128x64: #include "module/OLED_128x64.h" only
//   OLED_128x32.h  SSD1306::OLED128x32<Bus>   0.91" 128x32
//
// inheritance: Datasheet -> Frame -> OLED128x64, then graphics::Shapes / Text on top
#include "basic/datasheet.h"
#include "basic/frame.h"

#endif
