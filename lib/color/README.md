# color

顏色，`namespace color`。

```cpp
#include "color.h"
namespace RGB565 = color::RGB565;
screen.fillScreen(RGB565::BLUE);
uint16_t orange = RGB565::color(255, 128, 0);
```

| 檔案 | 內容 |
|---|---|
| `RGB565.h` | `color::RGB565::`：常用顏色常數（BLACK、WHITE、RED、GREEN、BLUE、CYAN、MAGENTA、YELLOW、ORANGE、GRAY），以及 `color(r, g, b)`、`red(c)`、`green(c)`、`blue(c)` |

RGB565 是 16 bit 的顏色：紅 5 bit、綠 6 bit、藍 5 bit，也就是 ST7789 在 `COLMOD_16BIT` 模式下的像素格式。
