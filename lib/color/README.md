# color

RGB565 顏色，`namespace color`。不依賴任何東西。

RGB565 是 16 bit 的顏色：紅 5 bit、綠 6 bit、藍 5 bit，也就是 ST7789 在 `COLMOD_16BIT` 模式下的像素格式。OLED 上 0 是暗、其他都是亮。

## 檔案

```
lib/color/
├── color.h     入口
└── RGB565.h    color::RGB565：常用顏色常數和轉換
```

## 用法

```cpp
#include "color.h"
namespace RGB565 = color::RGB565;

screen.drawFilledScreen(RGB565::BLUE);
uint16_t orange = RGB565::color(255, 128, 0);
```

## 函式

| 函式 / 常數 | 說明 |
|---|---|
| `BLACK`、`WHITE`、`RED`、`GREEN`、`BLUE`、`CYAN`、`MAGENTA`、`YELLOW`、`ORANGE`、`GRAY` | 常用顏色 |
| `color(r, g, b)` | 把 0 ～ 255 的紅綠藍轉成 RGB565 |
| `red(c)`、`green(c)`、`blue(c)` | 從 RGB565 取出 0 ～ 255 的紅、綠、藍 |

都是 `constexpr`：參數是常數時，在編譯時就算好。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `color(r, g, b)` | 各 0 ～ 255 | —（低位被丟掉：紅、藍留 5 bit，綠留 6 bit） |
| `red(c)`、`green(c)`、`blue(c)` | 任意 RGB565 | — |
