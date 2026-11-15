# graphics

圖形和文字，`namespace graphics`。疊在任何有 `fillWindow` 的螢幕上，例如 `ST7789::Window` 或 `ST7789::M130T`。這個 lib 裡沒有任何顏色常數，顏色在 `lib/color`。

## 入口

```cpp
#include "graphics.h"
using Screen = graphics::Canvas<ST7789::M130T<Bus>>;   // = Text<Shapes<M130T<Bus>>>
Screen screen;
```

| 檔案 | 類別 | 內容 |
|---|---|---|
| `shapes.h` | `graphics::Shapes<Screen>` | 點、線、矩形、三角形、圓 |
| `text.h` | `graphics::Text<Screen>` | 文字，字型來自 `lib/font`，下面必須是 Shapes |
| `graphics.h` | `graphics::Canvas<Screen>` | 照正確順序把 Shapes 和 Text 疊起來 |

## Shapes

函式名稱的後綴代表參數的種類：

| 後綴 | 參數 |
|---|---|
| `Size` | 左上角 + 寬高 |
| `Points` | 兩個端點或角 |
| `Center` | 中心 + 寬高 |
| `Delta` | 起點 + 位移 |
| `Angle` | 起點 + 長度 + 角度 |
| `Radius` | 圓心 + 半徑 |

| 圖形 | 函式 |
|---|---|
| 點 | `drawPoint` |
| 橫線 / 直線 | `drawHLineSize/Points/Center`、`drawVLineSize/Points/Center` |
| 斜線 | `drawLinePoints/Delta/Angle` |
| 矩形 | `drawHollowRect…`、`drawFilledRect…`（Size/Points/Center）、`drawFilledScreen` |
| 三角形 | `drawHollowTriangle…`、`drawFilledTriangle…`（Points/Size/Center） |
| 圓 | `drawHollowCircleRadius/Points`、`drawFilledCircleRadius/Points` |

### 每個函式畫出什麼

座標都是像素，x 往右、y 往下，(0, 0) 是左上角。

| 函式 | 畫出的範圍 | 細節 |
|---|---|---|
| `drawPoint(x, y)` | 1 個像素 | 1 個窗口 |
| `drawHLineSize(x, y, w)` | x ～ x + w − 1 | w ≤ 0 不畫（不會反向） |
| `drawHLinePoints(x0, x1, y)` | x0 ～ x1，包含兩端 | 順序不拘 |
| `drawHLineCenter(cx, y, w)` | cx − w / 2 起共 w 個 | w 是偶數時，cx 在中間偏右 |
| `drawVLine…` | 同上，換成 y | — |
| `drawLinePoints(x0, y0, x1, y1)` | 兩端都包含 | Bresenham，每步一個像素（8 連通）；水平、垂直會改用一個窗口 |
| `drawLineDelta(x, y, dx, dy)` | (x, y) ～ (x + dx, y + dy) | 同上 |
| `drawLineAngle(x, y, length, degrees)` | 終點 = (x + round(length·cos), y − round(length·sin)) | 0° 向右、90° 向上（逆時針）；用浮點數 |
| `…RectSize(x, y, w, h)` | x ～ x + w − 1，y ～ y + h − 1 | — |
| `…RectPoints(x0, y0, x1, y1)` | 兩個對角，包含 | 順序不拘 |
| `…RectCenter(cx, cy, w, h)` | 左上角 = (cx − w / 2, cy − h / 2) | 偶數時偏右下 |
| `drawFilledScreen(color)` | 整個螢幕 | 1 個窗口 |
| `…TrianglePoints(三個角)` | 三個角都包含 | 順序不拘；三點共線時畫成一條線，三點重合時畫成一個點 |
| `…TriangleSize(x, y, w, h)` | 等腰：頂點 (x + (w − 1) / 2, y)，底邊 y + h − 1 從 x 到 x + w − 1 | w 或 h ≤ 0 不畫 |
| `…TriangleCenter(cx, cy, w, h)` | 同 Size，左上角 = (cx − w / 2, cy − h / 2) | — |
| `…CircleRadius(cx, cy, r)` | 寬高 2r + 1 | r = 0 是一個點 |
| `…CirclePoints(cx, cy, px, py)` | 半徑 = 圓心到 (px, py) 的距離，四捨五入成整數 | (px, py) 不一定剛好在圓上 |

### 空心和實心

```cpp
screen.drawFilledRectSize(10, 10, 60, 40, color);
screen.drawHollowRectSize(10, 10, 60, 40, color);        // 邊界 1
screen.drawHollowRectSize(10, 10, 60, 40, color, 5);     // 邊界 5
```

- **`Hollow` 最後一個參數是邊界寬度**，預設 1。
- **座標是外框**：矩形的 x、y、w、h，三角形的三個角，圓的半徑，都是最外圈；邊界往裡面長。同樣座標的空心和實心，外緣完全重疊。
- **邊界太寬**：超過一半時就等於實心。
- **哪些像素屬於圖形**：
  - 圓：像素中心離圓心小於 r + 0.5。空心 = 半徑 r 的圓減掉半徑 r − 邊界的圓，任何寬度都不會有破洞。
  - 三角形：像素中心在每條邊外面都不超過 0.5。空心再挖掉「在每條邊裡面都超過 邊界 − 0.5」的像素。
- **畫法和速度**（一個窗口約 480 週期，一個像素約 44 週期）：

| 圖形 | 窗口數 | 計算 |
|---|---|---|
| 點、橫線、直線、實心矩形 | 1 | 無 |
| 空心矩形 | 4（邊界太寬時 1） | 無 |
| 斜線 | 長度 + 1（每個點一個窗口） | 整數 |
| 實心圓 | 2r + 1（每行一個） | 整數，每行幾次乘法 |
| 空心圓 | 每行 1～2 個（有洞的行上下各一個），左右兩邊 | 整數 |
| 三角形 | 每列每段連續的點一個（實心 1、空心最多 2） | 外框方塊在螢幕上的每個點約 100 週期，**最慢** |

## 輸入限制

**超出螢幕的部分會被裁掉。** 裁切只在最底層做一次：所有圖形和文字最後都呼叫螢幕的 `fillWindow`，它把窗口和螢幕取交集再畫（見 [ST7789](../ST7789/README.md#裁切)），所以圖形自己不用檢查。
- 代價：圖形照樣計算螢幕外的部分，只是不送出去。例如三角形逐點檢查時，螢幕外的點也會算，會浪費一點時間。

| 參數 | 型別 | 範圍 | 超出時 |
|---|---|---|---|
| x、y、角、圓心、端點 | `int16_t` | −32768 ～ 32767，可以在螢幕外 | 螢幕外的部分被裁掉 |
| 三角形的角 | `int16_t` | 可以在螢幕外，但三個角之間的 x 差、y 差都要 < 2048（例如都在 −900 ～ 1100） | 超過：邊的計算溢位，畫錯 |
| 線的兩端 | `int16_t` | 兩端的 x 差、y 差 < 32768 | 超過：溢位，畫錯 |
| w、h（`Size`、`Center`） | `int16_t` | > 0 | ≤ 0：不畫 |
| 半徑 r | `int16_t` | ≥ 0 | < 0：不畫；0 = 一個點 |
| 邊界 `border` | `int16_t` | ≥ 1 | ≤ 0：不畫；超過一半：實心 |
| `Points` 的兩點 | `int16_t` | 任何順序都可以 | — |
| `drawLineAngle` 的角度 | `int16_t` | 任意，0 = 右、90 = 上；用浮點數 sin / cos | — |
| `drawLineAngle` 的長度 | `int16_t` | ≥ 0 | 螢幕外的部分被裁掉 |
| `Center` 的寬高 | `int16_t` | 偶數時中心偏右下（左上角 = 中心 − w / 2） | — |
| 顏色 | `uint16_t` | RGB565，見 `lib/color` | — |

### 依列表上色

`Shapes` 的函式都只有一個顏色。要依列表一個像素一個顏色，用螢幕本身的 `writeWindow(x, y, w, h, colors)`：它把 w×h 個 RGB565（RAM 裡的陣列，一列一列）填進矩形，超出螢幕的部分會被裁掉。

## Text

```cpp
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
int16_t x = screen.drawTexts(10, 10, color, "x = ", 12, ' ', 3.14159, F(" flash"), true);
```

- **`drawTexts`**：可以傳任意多個值，畫完回傳最後一行的 x。支援的型別：
  - 字元、字串、`F("...")`、有 `c_str()` 的物件
  - 所有整數、`float`/`double`、`bool`（畫成 true/false）
  - 陣列：每個元素都畫，中間插入 `setSeparator` 設定的分隔字串
- **設定**：
  - `setDecimals`：小數位數，預設 2。
  - `setSeparator`：陣列元素之間的字串，預設空字串。
  - `setLineSpacing`：額外的行距，預設 0。
  - `setAutoLineBreak`：開啟後 `\n` 會換行，超出螢幕寬度也會換行，新的一行從 `drawTexts` 的起始 x 開始。
- **只畫字的部分**：背景保持原樣，要蓋掉舊字得先自己畫背景。字型範圍外的字元會畫成字型的預設字元。
- **畫法**：每個字是一組形狀（點、橫線、直線、矩形），由 `lib/font/source/convert.py` 事先算好最快的組合。

### 各種值畫成什麼

| 型別 | 畫成 | 例子 |
|---|---|---|
| `char` | 那個字元 | `'A'` → `A` |
| `int8_t`（`signed char`）、`uint8_t` | 數字 | `(int8_t)-5` → `-5`、`(uint8_t)200` → `200` |
| `int`、`long`、`unsigned`… | 十進位數字，負數有 `-` | 最小值也正確：`-2147483648` |
| `long long`、`unsigned long long` | 同上（多約 0.7 KB Flash） | — |
| `bool` | `true` / `false` | — |
| `float`、`double`、`long double` | 小數 `setDecimals()` 位，最後一位四捨五入 | `2.345f` → `2.35`（float 只有約 7 位有效數字） |
| ↳ 特殊值 | `nan`、`inf`、`-inf`、`ovf`（四捨五入後 > 4294967040） | 很小的負數會畫成 `-0.00` |
| `const char *`、`char[]` | 字串，到 `'\0'` 為止 | `nullptr`：不畫 |
| `F("...")` | Flash 裡的字串 | 不佔 RAM |
| 有 `c_str()` 的物件 | 字串 | Arduino `String`（會帶進 malloc，約 1.2 KB） |
| 其他陣列 `T[N]` | 每個元素，中間插入 `setSeparator` | `int[]{1, 2, 3}` → `1, 2, 3`（分隔是 `", "` 時）；只認得真正的陣列，指標不行 |
| 其他型別 | — | 編譯錯誤 |

| 情況 | 結果 |
|---|---|
| `'\n'`，沒開 `setAutoLineBreak` | 畫成預設字元 |
| `'\n'`，開了 `setAutoLineBreak` | 換行：x 回到 `drawTexts` 的起始 x，y 加 `fontHeight() + setLineSpacing()` |
| 字放不下螢幕寬度，開了 `setAutoLineBreak` | 先換行再畫；已經在一行的開頭就照畫（被裁切），不會無限換行 |
| 回傳值 | 最後一個字後面的 x（在最後一行）；y 不回傳，要自己用 `fontHeight()` 算 |
| 沒傳任何值 | 什麼都不畫，回傳原本的 x |
| 沒設定字型 | 什麼都不畫，回傳原本的 x |

### 文字的輸入限制

| 參數 | 型別 | 範圍 | 超出時 |
|---|---|---|---|
| x、y | `int16_t` | 文字的左上角，可以在螢幕外 | 螢幕外的部分被裁掉 |
| 寬度 | — | 沒開 `setAutoLineBreak`：不換行 | 超出右邊的部分被裁掉 |
| 寬度 | — | 開了 `setAutoLineBreak`：**會自動換行**，放不下的字移到下一行；新的一行連一個字都放不下時照畫 | 被裁掉 |
| 高度 | — | 換行不檢查底部 | 超出底部的行被裁掉 |
| 字元 | `char` | 字型的 first ～ last | 畫成預設字元 |
| 整數 | 8 / 16 / 32 位元 | 全部範圍 | — |
| 整數 | 64 位元 | 可以用，但會多約 0.7 KB Flash（64 位元除法） | — |
| `float` / `double` | | 絕對值 ≤ 4294967040 | 畫成 `ovf`；nan、inf 也會畫出來 |
| `setDecimals` | `uint8_t` | 0 ～ 9 比較實際（float 只有約 7 位有效數字） | — |
| `setSeparator` | `const char *` | 只存指標，字串要一直存在 | — |
| `setLineSpacing` | `uint8_t` | 0 ～ 255 | — |
| 字型 | `font::*` | 必須先 `setFont<…>()` | 沒設定就畫：什麼都不畫 |
