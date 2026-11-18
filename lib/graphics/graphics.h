#ifndef GRAPHICS_H
#define GRAPHICS_H

// Graphics entry point: include this file only
//   shapes.h  graphics::Shapes<Screen>  point, lines, rectangle, triangle, circle
//   text.h    graphics::Text<Screen>    text with fonts from lib/font (needs Shapes below it)
//   Canvas    graphics::Canvas<Screen>  Shapes + Text in the right order
#include "shapes.h"
#include "text.h"

namespace graphics {
    // a screen with every Graphics feature: Text needs Shapes below it
    // e.g. graphics::Canvas<ST7789::M130T<Bus, color::RGB565>>; colors: Canvas::ColorFormat::RED ...
    template <class Screen>
    using Canvas = Text<Shapes<Screen>>;
}

#endif
