# ST7789/module

English | [中文](讀我.md)

One file per screen module (the board you actually buy). A module includes only the `basic/` and `tool/` it needs; `ST7789.h` then includes every module, so a program only needs `#include "ST7789.h"`.

## Files

```
module/
└── M130T.h    M130T<Bus, ColorFormat>: 1.3 inch IPS 240×240, no CS (SPI mode 3)
```

## Adding a module

A module decides three things: the panel size, the offset in the controller's 240×320 memory, and the initialization sequence. Following `M130T.h`:

```cpp
#ifndef ST7789_XXX_H
#define ST7789_XXX_H

#include "../basic/datasheet.h"
#include "../basic/window.h"
#include "../tool/init.h"

namespace ST7789 {
    template <class Bus, class ColorFormat>
    class XXX : public Window<Bus, ColorFormat> {
    public:
        inline XXX();
        inline void init(uint8_t blk = 255);   // Bus init + reset + the module's initialization sequence
    };

    // ================= definitions =================

    template <class Bus, class ColorFormat>
    inline XXX<Bus, ColorFormat>::XXX()
        : Window<Bus, ColorFormat>(240, 280) {   // panel size
    }

    template <class Bus, class ColorFormat>
    inline void XXX<Bus, ColorFormat>::init(uint8_t blk) {
        Bus::init(blk);
        this->hwReset();
        this->swReset();
        this->slpOut();
        this->setOffset(0, 20);     // when the panel is not at the top left of the memory
        this->madctl(Window<Bus, ColorFormat>::MADCTL_ORDER);   // the color order follows ColorFormat
        this->colmod(Window<Bus, ColorFormat>::COLMOD_VALUE);   // 12 / 16 / 18 bit follows ColorFormat
        // ... the maker's voltage and gamma settings
        this->dispOn();
    }
}

#endif
```

Then add a line `#include "module/XXX.h"` at the end of `ST7789.h`.

## Caveats

| Situation | What to do |
|---|---|
| file name | the module's model, keeping its case (e.g. `M130T.h`) |
| where the values come from | the module's specification or the maker's example code; without them, try `initDefault()` of `tool/init.h` first |
| inverted colors on an IPS panel | add `invOn()` to the initialization |
| checking it | run `tests[0]`, `tests[1]` of `src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp`: is the color order right, and does the border touch all four edges |
