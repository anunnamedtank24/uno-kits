# ST7789

ST7789 彩色螢幕控制器的驅動，`namespace ST7789`。只依賴一個 Bus（例如 `Uno::UnoST7789`），換主板只要換 Bus。它本身就是 Screen，`graphics`、`image`、`video` 都畫在它上面。

## 檔案

```
lib/ST7789/
├── ST7789.h          入口（沒有對應的模組時）：basic/ + tool/
├── basic/
│   ├── datasheet.h   Datasheet<Bus>：datasheet 第 9 章所有指令（Table 1、Table 2），照順序排
│   └── window.h      Window<Bus>：setWindow、fillWindow、writeWindow（後兩個會裁切）
├── module/
│   └── M130T.h       M130T<Bus>：1.3 吋 240×240、沒有 CS 的模組（尺寸 + init）
└── tool/
    └── init.h        initDefault()：適用任何面板的最精簡初始化
```

繼承順序：`Datasheet` → `Window` → `M130T`，再往上是 `graphics::Shapes`、`graphics::Text`。

## 接線

M130T（沒有 CS）接 UNO：

| 模組 | UNO |
|---|---|
| GND | GND |
| VCC | 3.3V |
| SCL | D13（SPI SCK，固定） |
| SDA | D11（SPI MOSI，固定） |
| RES | D10 |
| DC | D8 |
| BLK | D9（PWM，可以調亮度） |

## 用法

```cpp
#include "Uno_ST7789.h"
#include "module/M130T.h"    // 有對應的模組：只 include 模組，它包含 ST7789.h 的全部

using Bus = Uno::UnoST7789<8, 10, Uno::NONE, 9>;
ST7789::M130T<Bus> screen;

void setup() {
    screen.init();                               // Bus init + 重置 + M130T 的初始化流程
    screen.fillWindow(0, 0, 240, 240, 0xF800);   // 整個螢幕填紅色
}
```

完整的例子：`src/week_3/test_1.cpp`。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Bus` | `init(blk)`、`setBacklight(blk)`、`delayMs`、`nowMs`、`readFlash8/16`、型別 `FlashString`；`cmdMode`、`dataMode`（DC）、`select`、`deselect`（CS）；`HAS_RES`、`rstHigh`、`rstLow`（RES）；`write8`、`write16`；`readMode`、`writeMode`、`readDummy`、`read8`（只能寫的模組留空） | `Uno::UnoST7789` |

- `write8`、`write16` 可以在 byte 送完之前就返回，但 DC、CS 切換前必須等它送完。
- 完整清單在 `basic/datasheet.h` 開頭。

## 函式

| 函式 | 說明 |
|---|---|
| `init(blk)`（模組） | 初始化 Bus、重置、跑模組的初始化流程，背光設成 blk |
| `width()`、`height()` | 回傳面板大小 |
| `fillWindow(x, y, w, h, color)` | 填一個實心矩形，裁切 |
| `writeWindow(x, y, w, h, colors)` | 把 RAM 裡 w×h 個顏色（一列一列）填進矩形，裁切 |
| `setWindow(x0, y0, x1, y1)` | 設定控制器的窗口（包含兩端），不裁切 |
| `writeData16(color)`、`endWrite()` | 在 `setWindow` 之後一個一個送像素、結束 |
| `setOffset(x, y)` | 設定面板在控制器 240×320 記憶體裡的位置 |
| `Datasheet` 的指令 | 照 datasheet 原樣送出，例如 `madctl`、`colmod`、`invOn`、`dispOn` |

### 低層的畫法

`video` 的解碼器就是這樣畫的：

```cpp
screen.setWindow(x0, y0, x1, y1);   // 包含兩端，不裁切
for (uint16_t i = 0; i < count; i++) {
    screen.writeData16(color);
}
screen.endWrite();
```

- 一個窗口 = CASET + RASET + RAMWR，約 480 週期；之後每個像素約 44 週期。
- 送的像素數超過窗口大小時，控制器會從窗口的開頭繼續寫。

## 裁切

`fillWindow`、`writeWindow` 會裁切；`graphics` 的圖形、文字和 `image` 都經過它們，所以只要這裡裁一次：

1. 寬或高 ≤ 0：不畫。
2. 左邊超出（x < 0）：寬度減掉超出的部分，x 移到 0；上面同理。
3. 右邊超出（x + w > width）：寬度縮成 width − x；下面同理。
4. 剩下的寬或高 ≤ 0（整個在螢幕外）：不畫。
5. `writeWindow` 另外跳過陣列裡被切掉的顏色：每一列從（可見的 y − y）× w +（可見的 x − x）開始讀。

`setWindow` 不裁切：它直接設定控制器的窗口，給自己送像素的 `video` 用。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `fillWindow`、`writeWindow` 的 x、y | 任意 `int16_t`，可以在螢幕外 | 裁切 |
| 寬、高 | > 0 | 不畫 |
| `writeWindow` 的陣列 | 至少 w×h 個，一列一列 | 不檢查：讀到別的 RAM |
| `setWindow(x0, y0, x1, y1)` | 0 ≤ x0 ≤ x1 < width，0 ≤ y0 ≤ y1 < height | 不檢查：畫面錯誤 |
| `init(blk)` | 背光 0 ～ 255 | — |
| `setOffset(x, y)` | 面板在 240×320 記憶體裡的位置 | 不檢查：畫面錯誤 |
| 顏色 | RGB565（`COLMOD_16BIT`） | — |
| `Datasheet` 的指令參數 | 見 datasheet 第 9 章 | 不檢查：照原樣送出 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `M130T<Bus>` 8 bytes（寬、高、偏移量） |
| 時間 | 一個窗口約 480 週期，一個像素約 44 週期，整個螢幕約 160 ms |
| 硬體 | Bus 的腳位（見 [Uno](../Uno/README.md#unost7789st7789-螢幕的-bus)） |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| `madctl()` 旋轉畫面（MV、MX、MY） | `width()`、`height()`、偏移量不會跟著變，裁切和座標會錯 | 旋轉 90° 時自己交換寬高、重設 `setOffset` |
| `colmod()` 改成 12 / 18 bit | `fillWindow` 等還是送 16 bit 顏色，畫面錯亂 | 保持 `COLMOD_16BIT` |
| NVM 指令（`nvmSet` 等） | **燒錄面板的一次性記憶體，改不回來** | 除非確定，否則不要呼叫（宣告旁有警告） |
| 讀取指令（`ramRd()`、`rddid()`…） | UNO 的 Bus 沒有讀取線，回傳 0 | — |
| 大面積變化 | 可能看到畫面撕裂：M130T 沒有拉出 TE 腳，沒辦法跟刷新同步 | 每次只改小範圍 |
| 換別的模組 | `M130T` 固定 240×240 | 在 `module/` 加一個檔案，見 [module/README.md](module/README.md) |
