#ifndef COLOR_H
#define COLOR_H

// color formats, entry point: include this file only
//   packed.h  color::Packed<R, G, B, ORDER>   any packed RGB format, and the common ones:
//             RGB565, BGR565 (ST7789 16 bit), RGB666, RGB888 (ST7789 18 bit), RGB444 (ST7789 12 bit),
//             BGR888, GBR888, RGB332; color::convert<To, From>(c)
//   BW.h      color::BW<THRESHOLD>            black / white (SSD1306 OLED)
// a format is the ColorFormat a screen declares (ST7789::M130T<Bus, color::RGB565>, SSD1306::OLED128x64<Bus, color::BW<>>);
// graphics, font, image and video then draw with Screen::ColorFormat
#include "packed.h"
#include "BW.h"

#endif
