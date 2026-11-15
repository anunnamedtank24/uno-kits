# graphics

圖形和文字，`namespace graphics`。畫在任何 Screen 上（例如 `ST7789::M130T`、`SSD1306::OLED128x64`），只用到它的 `fillWindow`，所以裁切也交給 Screen。沒有顏色常數，顏色在 [color](../color/README.md)；字型在 [font](../font/README.md)。

## 檔案

```
lib/graphics/
├── graphics.h    入口：Canvas<Screen> = Text<Shapes<Screen>>
├── shapes.h      Shapes<Screen>：點、線、矩形、三角形、圓
└── text.h        Text<Screen>：文字（下面必須是 Shapes）
```

## 用法

```cpp
#include "graphics.h"
#include "font.h"

using Screen = graphics::Canvas<ST7789::M130T<Bus>>;
Screen screen;

screen.drawFilledRectSize(10, 10, 60, 40, color);
screen.drawHollowCircleRadius(120, 120, 50, color, 3);
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
screen.drawTexts(10, 200, color, "x = ", 12, ' ', 3.14159);
```

完整的例子：`src/week_3/test_1.cpp`（ST7789）、`src/week_4/test_1.cpp`（OLED）。

## 介面需求

| 參數 | 需要的函式 | 現成的實作 |
|---|---|---|
| `Screen` | `width()`、`height()`、`fillWindow(x, y, w, h, color)`（會裁切）；文字另外要型別 `BusType`，它有 `readFlash8`、`readFlash16`、型別 `FlashString` | `ST7789::M130T`、`ST7789::Window`、`SSD1306::OLED128x64`、`SSD1306::Frame` |

## 函式

### 圖形（Shapes）

函式名稱的後綴代表參數的種類：

| 後綴 | 參數 |
|---|---|
| `Size` | 左上角 + 寬高 |
| `Points` | 兩個端點或角 |
| `Center` | 中心 + 寬高 |
| `Delta` | 起點 + 位移 |
| `Angle` | 起點 + 長度 + 角度 |
| `Radius` | 圓心 + 半徑 |

| 函式 | 說明 |
|---|---|
| `drawPoint(x, y, color)` | 畫一個像素 |
| `drawHLineSize/Points/Center`、`drawVLineSize/Points/Center` | 畫橫線、直線 |
| `drawLinePoints/Delta/Angle` | 畫斜線 |
| `drawHollowRect…`、`drawFilledRect…`（Size / Points / Center） | 畫空心、實心矩形 |
| `drawFilledScreen(color)` | 填滿整個螢幕 |
| `drawHollowTriangle…`、`drawFilledTriangle…`（Points / Size / Center） | 畫空心、實心三角形 |
| `drawHollowCircleRadius/Points`、`drawFilledCircleRadius/Points` | 畫空心、實心圓 |

座標都是像素，x 往右、y 往下，(0, 0) 是左上角。每個函式畫出的範圍：

| 函式 | 畫出的範圍 | 細節 |
|---|---|---|
| `drawPoint(x, y)` | 1 個像素 | 1 個窗口 |
| `drawHLineSize(x, y, w)` | x ～ x + w − 1 | w ≤ 0 不畫（不會反向） |
| `drawHLinePoints(x0, x1, y)` | x0 ～ x1，包含兩端 | 順序不拘 |
| `drawHLineCenter(cx, y, w)` | cx − w / 2 起共 w 個 | w 是偶數時，cx 在中間偏右 |
| `drawVLine…` | 同上，換成 y | — |
| `drawLinePoints(x0, y0, x1, y1)` | 兩端都包含 | Bresenham，每步一個像素（8 連通）；水平、垂直改用一個窗口 |
| `drawLineDelta(x, y, dx, dy)` | (x, y) ～ (x + dx, y + dy) | 同上 |
| `drawLineAngle(x, y, length, degrees)` | 終點 = (x + round(length·cos), y − round(length·sin)) | 0° 向右、90° 向上（逆時針）；用浮點數 |
| `…RectSize(x, y, w, h)` | x ～ x + w − 1，y ～ y + h − 1 | — |
| `…RectPoints(x0, y0, x1, y1)` | 兩個對角，包含 | 順序不拘 |
| `…RectCenter(cx, cy, w, h)` | 左上角 = (cx − w / 2, cy − h / 2) | 偶數時偏右下 |
| `…TrianglePoints(三個角)` | 三個角都包含 | 順序不拘；三點共線畫成線，三點重合畫成點 |
| `…TriangleSize(x, y, w, h)` | 等腰：頂點 (x + (w − 1) / 2, y)，底邊 y + h − 1 從 x 到 x + w − 1 | w 或 h ≤ 0 不畫 |
| `…TriangleCenter(cx, cy, w, h)` | 同 Size，左上角 = (cx − w / 2, cy − h / 2) | — |
| `…CircleRadius(cx, cy, r)` | 寬高 2r + 1 | r = 0 是一個點 |
| `…CirclePoints(cx, cy, px, py)` | 半徑 = 圓心到 (px, py) 的距離，四捨五入 | (px, py) 不一定剛好在圓上 |

### 文字（Text）

| 函式 | 說明 |
|---|---|
| `setFont<Font>()` | 設定字型（`font::*`） |
| `drawTexts(x, y, color, 值…)` | 畫任意多個值，回傳最後一行的 x |
| `fontHeight()` | 回傳字型的高度 |
| `setDecimals(n)` | 設定小數位數，預設 2 |
| `setSeparator(text)` | 設定陣列元素之間的字串，預設空字串 |
| `setLineSpacing(n)` | 設定額外的行距，預設 0 |
| `setAutoLineBreak(on)` | 開啟後 `\n` 會換行，超出螢幕寬度也換行，新的一行從 `drawTexts` 的起始 x 開始 |

```cpp
int16_t x = screen.drawTexts(10, 10, color, "x = ", 12, ' ', 3.14159, F(" flash"), true);
```

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

## 一個像素一個顏色

`Shapes` 的函式都只有一個顏色。要依陣列一個像素一個顏色，用 Screen 本身的 `writeWindow(x, y, w, h, colors)`：它把 w×h 個 RGB565（RAM 裡的陣列，一列一列）填進矩形，超出螢幕的部分會被裁掉。

## 輸入限制

**超出螢幕的部分會被裁掉**：所有圖形和文字最後都呼叫 Screen 的 `fillWindow`，它把窗口和螢幕取交集再畫（見 [ST7789](../ST7789/README.md#裁切)）。代價是螢幕外的部分照樣計算，只是不送出去。

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
| 顏色 | RGB565（OLED：0 = 暗，其他 = 亮） | — |
| 文字的 x、y | 左上角，可以在螢幕外 | 裁切 |
| 文字寬度 | 沒開 `setAutoLineBreak`：不換行 | 裁切 |
| 文字高度 | 換行不檢查底部 | 裁切 |
| 字元 | 字型的 first ～ last | 畫成預設字元 |
| `float` / `double` | 絕對值 ≤ 4294967040 | 畫成 `ovf`；nan、inf 也會畫出來 |
| `setDecimals` | 0 ～ 9 比較實際（float 只有約 7 位有效數字） | — |
| `setSeparator` | 只存指標，字串要一直存在 | 不檢查 |
| 字型 | 必須先 `setFont<…>()` | 不畫 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `Canvas` 比 Screen 多 19 bytes（字型指標、文字設定） |
| Flash | 只有用到的函式會編譯進去；`drawLineAngle` 約 0.6 KB、畫 `float` 約 1.3 KB（浮點程式庫） |

畫圖的時間（ST7789：一個窗口約 480 週期，一個像素約 44 週期）：

| 圖形 | 窗口數 | 計算 |
|---|---|---|
| 點、橫線、直線、實心矩形 | 1 | 無 |
| 空心矩形 | 4（邊界太寬時 1） | 無 |
| 斜線 | 長度 + 1（每個點一個窗口） | 整數 |
| 實心圓 | 2r + 1（每行一個） | 整數，每行幾次乘法 |
| 空心圓 | 每行 1 ～ 2 個（有洞的行上下各一個），左右兩邊 | 整數 |
| 三角形 | 每列每段連續的點一個（實心 1、空心最多 2） | 外框方塊裡每個點約 100 週期，**最慢** |
| 文字 | 每個字一組形狀（由 `font` 事先算好最快的組合） | 無 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 文字疊在舊字上 | 只畫字的部分，背景保持原樣 | 先自己畫背景蓋掉舊字 |
| OLED 上畫圖 | 只改板子 RAM 裡的畫面 | 畫完呼叫 `display()` |
| 畫 Arduino `String` | 會帶進 malloc，約 1.2 KB Flash | 用 `const char *` 或 `F("...")` |
