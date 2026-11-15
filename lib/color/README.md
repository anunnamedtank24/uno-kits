# color

顏色，`namespace color`。

```cpp
#include "color.h"
namespace RGB565 = color::RGB565;
screen.drawFilledScreen(RGB565::BLUE);
uint16_t orange = RGB565::color(255, 128, 0);
```

| 檔案 | 內容 |
|---|---|
| `RGB565.h` | `color::RGB565::`：常用顏色常數（BLACK、WHITE、RED、GREEN、BLUE、CYAN、MAGENTA、YELLOW、ORANGE、GRAY），以及 `color(r, g, b)`、`red(c)`、`green(c)`、`blue(c)` |

## 輸入限制

| 函式 | 參數 | 範圍 |
|---|---|---|
| `color(r, g, b)` | `uint8_t` ×3 | 0 ～ 255；低位會被丟掉（紅、藍留 5 bit，綠留 6 bit） |
| `red(c)`、`green(c)`、`blue(c)` | `uint16_t` | 任意 RGB565，回傳 0 ～ 255 |

都是 `constexpr`，參數是常數時在編譯時就算好。

RGB565 是 16 bit 的顏色：紅 5 bit、綠 6 bit、藍 5 bit，也就是 ST7789 在 `COLMOD_16BIT` 模式下的像素格式。
