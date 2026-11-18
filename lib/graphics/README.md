# graphics

| 需要 | 得到 |
|---|---|
| 一個 `Screen`：`width()`、`height()`、`fillWindow(x, y, w, h, color)` 和它的顏色格式 `ColorFormat`（`ST7789::M130T<Bus, color::RGB565>`、`SSD1306::OLED128x64<Bus, color::BW<>>`） | `graphics::Canvas<Screen>`：點、線、矩形、三角形、圓（空心、實心），文字和數字（`drawTexts`） |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

圖形和文字，`namespace graphics`。只用到 Screen 的 `fillWindow`，所以裁切也交給 Screen。沒有顏色常數：顏色的型別和常數跟著螢幕的顏色格式（`Screen::ColorFormat`，見 [color](../color/README.md)）；字型在 [font](../font/README.md)。

## 用法

```cpp
#include "graphics.h"
#include "font.h"

using Screen = graphics::Canvas<ST7789::M130T<Bus, color::RGB565>>;
Screen screen;
auto color = Screen::ColorFormat::ORANGE;                      // 螢幕的顏色格式：RGB565 的橘色

screen.drawFilledRectSize(10, 10, 60, 40, color);
screen.drawHollowCircleRadius(120, 120, 50, color, 3);
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
screen.drawTexts(10, 200, color, "x = ", 12, ' ', 3.14159);
```

完整的例子：`src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp`（ST7789）、`src/week_4_SSD1306_switch_buzzer_audio/tests/1_oled.cpp`（OLED）。

## 空心和實心

```cpp
screen.drawFilledRectSize(10, 10, 60, 40, color);
screen.drawHollowRectSize(10, 10, 60, 40, color);        // 邊界 1
screen.drawHollowRectSize(10, 10, 60, 40, color, 5);     // 邊界 5
```

- **`Hollow` 的最後一個參數是邊界寬度**，預設 1。
- **座標是外框**：矩形的 x、y、w、h，三角形的三個角，圓的半徑，都是最外圈；邊界往裡面長。同樣座標的空心和實心，外緣完全重疊。
- **邊界太寬**：超過一半時就等於實心。
- **哪些像素屬於圖形**：
  - 圓：像素中心離圓心小於 r + 0.5。空心 = 半徑 r 的圓減掉半徑 r − 邊界的圓，任何寬度都不會有破洞。
  - 三角形：像素中心在每條邊外面都不超過 0.5。空心再挖掉「在每條邊裡面都超過 邊界 − 0.5」的像素。

## 各種值畫成什麼

| 型別 | 畫成 | 例子 |
|---|---|---|
| `char` | 那個字元 | `'A'` → `A` |
| `int8_t`（`signed char`）、`uint8_t` | 數字 | `(int8_t)-5` → `-5`、`(uint8_t)200` → `200` |
| `int`、`long`、`unsigned`… | 十進位數字，負數有 `-` | 最小值也正確：`-2147483648` |
| `long long`、`unsigned long long` | 同上（多約 0.7 KB Flash） | — |
| `bool` | `true` / `false` | — |
| `float`、`double`、`long double` | 小數 `setDecimals()` 位，最後一位四捨五入 | `2.345f` → `2.35`（float 只有約 7 位有效數字） |
| ↳ 特殊值 | `nan`、`inf`、`-inf`、`ovf`（四捨五入後 > 4294967040） | 很小的負數畫成 `-0.00` |
| `const char *`、`char[]` | 字串，到 `'\0'` 為止 | `nullptr`：不畫 |
| `F("...")` | Flash 裡的字串 | 不佔 RAM |
| 有 `c_str()` 的物件 | 字串 | Arduino `String`（會帶進 malloc，約 1.2 KB） |
| 其他陣列 `T[N]` | 每個元素，中間插入 `setSeparator` | `int[]{1, 2, 3}` → `1, 2, 3`（分隔是 `", "` 時）；只認得真正的陣列，指標不行 |
| 其他型別 | — | 編譯錯誤 |

| 情況 | 結果 |
|---|---|
| `'\n'`，沒開 `setAutoLineBreak` | 畫成預設字元 |
| `'\n'`，開了 `setAutoLineBreak` | 換行：x 回到起始 x，y 加 `fontHeight() + setLineSpacing()` |
| 字放不下螢幕寬度，開了 `setAutoLineBreak` | 先換行再畫；已經在一行的開頭就照畫（被裁切），不會無限換行 |
| 回傳值 | 最後一個字後面的 x（在最後一行）；y 不回傳，用 `fontHeight()` 算 |
| 沒傳任何值、沒設定字型 | 什麼都不畫，回傳原本的 x |

## 輸入限制

**超出螢幕的部分會被裁掉**：所有圖形和文字最後都呼叫 Screen 的 `fillWindow`，它把窗口和螢幕取交集再畫（見 [ST7789](../ST7789/INTERFACE.md#裁切)）。代價是螢幕外的部分照樣計算，只是不送出去。

| 項目 | 範圍 | 超出時 |
|---|---|---|
| x、y、角、圓心、端點 | −32768 ～ 32767，可以在螢幕外 | 裁切 |
| 三角形的三個角 | 之間的 x 差、y 差都 < 2048（例如都在 −900 ～ 1100） | 不檢查：邊的計算溢位，畫錯 |
| 線的兩端 | x 差、y 差 < 32768 | 不檢查：溢位，畫錯 |
| w、h（`Size`、`Center`） | > 0 | 不畫 |
| 半徑 r | ≥ 0；0 = 一個點 | 不畫 |
| 邊界 `border` | ≥ 1；超過一半 = 實心 | 不畫 |
| `drawLineAngle` 的角度 | 任意，0 = 右、90 = 上 | — |
| `drawLineAngle` 的長度 | ≥ 0 | 裁切 |
| 顏色 | 螢幕的 `ColorFormat::Value`（RGB565 是 `uint16_t`、BW 是 `uint8_t`、RGB888 是 `uint32_t`） | — |
| 文字的 x、y | 左上角，可以在螢幕外 | 裁切 |
| 文字寬度 | 沒開 `setAutoLineBreak`：不換行 | 裁切 |
| 文字高度 | 換行不檢查底部 | 裁切 |
| 字元 | 字型的 first ～ last | 畫成預設字元 |
| `float` / `double` | 絕對值 ≤ 4294967040 | 畫成 `ovf`；nan、inf 也會畫出來 |
| `setDecimals` | 0 ～ 9 比較實際（float 只有約 7 位有效數字） | — |
| `setSeparator` | 只存指標，字串要一直存在 | 不檢查 |
| 字型 | 必須先 `setFont<…>()` | 不畫 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 文字疊在舊字上 | 只畫字的部分，背景保持原樣 | 先自己畫背景蓋掉舊字 |
| OLED 上畫圖 | 只改板子 RAM 裡的畫面 | 畫完呼叫 `display()` |
| 畫 Arduino `String` | 會帶進 malloc，約 1.2 KB Flash | 用 `const char *` 或 `F("...")` |
