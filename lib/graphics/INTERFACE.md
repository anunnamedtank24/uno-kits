# graphics：介面

| 需要 | 得到 |
|---|---|
| 一個 `Screen`：`width()`、`height()`、`fillWindow(x, y, w, h, color)` 和它的顏色格式 `ColorFormat`（`ST7789::M130T<Bus, color::RGB565>`、`SSD1306::OLED128x64<Bus, color::BW<>>`） | `graphics::Canvas<Screen>`：點、線、矩形、三角形、圓（空心、實心），文字和數字（`drawTexts`） |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [讀我.md](讀我.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 螢幕 | `Screen`：`width()`、`height()`、會裁切的 `fillWindow(x, y, w, h, color)`，和顏色格式 `ColorFormat`（顏色的型別是 `ColorFormat::Value`） |
| 文字 | `Screen::BusType` 要有 `readFlash8/16`（字型在 Flash）、`FlashString`（`F("…")`） |
| 裁切 | graphics 自己不裁切：全部交給 `fillWindow` |
| 浮點 | `drawLineAngle`、畫 `float` / `double` 才會用到浮點（多約 0.6 ～ 1.3 KB Flash） |
| OLED | 畫完要 `display()`：graphics 只改 RAM 裡的畫面 |

## 檔案

```
lib/graphics/
├── graphics.h    入口：Canvas<Screen> = Text<Shapes<Screen>>
├── shapes.h      Shapes<Screen>：點、線、矩形、三角形、圓
└── text.h        Text<Screen>：文字（下面必須是 Shapes）
```

## 介面需求

| 參數 | 需要的函式 | 現成的實作 |
|---|---|---|
| `Screen` | 型別 `ColorFormat`（顏色的型別 `ColorFormat::Value`）；`width()`、`height()`、`fillWindow(x, y, w, h, color)`（會裁切）；文字另外要型別 `BusType`，它有 `readFlash8`、`readFlash16`、型別 `FlashString` | `ST7789::M130T`、`ST7789::Window`、`SSD1306::OLED128x64`、`SSD1306::Frame` |

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

## 一個像素一個顏色

`Shapes` 的函式都只有一個顏色。要依陣列一個像素一個顏色，用 Screen 本身的 `writeWindow(x, y, w, h, colors)`：它把 w×h 個螢幕格式的顏色（`Screen::ColorFormat::Value`，RAM 裡的陣列，一列一列）填進矩形，超出螢幕的部分會被裁掉。

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
