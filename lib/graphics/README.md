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
| 矩形 | `drawRect…`、`fillRect…`（Size/Points/Center）、`fillScreen` |
| 三角形 | `drawTriangle…`、`fillTriangle…`（Points/Size/Center） |
| 圓 | `drawCircleRadius/Points`、`fillCircleRadius/Points` |

全部最後都會呼叫 `fillWindow`，超出螢幕的部分會被裁掉。

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
- **只畫字的部分**：背景保持原樣。字型範圍外的字元會畫成字型的預設字元。
- **畫法**：每個字是一組形狀（點、橫線、直線、矩形），由 `lib/font/source/convert.py` 事先算好最快的組合。
