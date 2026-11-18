#ifndef SSD1306_H
#define SSD1306_H

// I2C OLED entry point (SSD1306 controller): include this file only (it includes every module)
//
// basic/   built directly on the datasheet
//   datasheet.h  SSD1306::Datasheet<Bus>                              all datasheet commands, status read
//   frame.h      SSD1306::Frame<Bus, ColorFormat, WIDTH, HEIGHT>      picture in RAM, fillWindow / writeWindow /
//                                                                     writePixel, display(); ColorFormat: color::BW<>
//
// module/  the boards you buy: size, init sequence
//   OLED_128x64.h  SSD1306::OLED128x64<Bus, ColorFormat>   0.96" 128x64
//   OLED_128x32.h  SSD1306::OLED128x32<Bus, ColorFormat>   0.91" 128x32
//
// inheritance: Datasheet -> Frame -> OLED128x64, then graphics::Shapes / Text on top
#include "basic/datasheet.h"
#include "basic/frame.h"
#include "module/OLED_128x64.h"
#include "module/OLED_128x32.h"

#endif
