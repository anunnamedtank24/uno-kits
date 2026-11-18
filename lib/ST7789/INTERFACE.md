# ST7789：介面

| 需要 | 得到 |
|---|---|
| 一個 `Bus`（SPI）：`init(blk)`、`cmdMode()`、`dataMode()`、`write8()`、`write16()`…（`Uno::UnoST7789<DC, RES, CS, BLK>`）；一個 `ColorFormat`（`color::RGB565`、`RGB888`、`RGB444`…） | `ST7789::M130T`：一個 Screen（`graphics`、`image`、`video` 都能畫），12 / 16 / 18 bit 顏色，`fillWindow`、`writeWindow`、背光 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 匯流排 | `Bus`：硬體 SPI（D11 / D13 固定）+ DC、RES、CS、BLK；只寫不讀也可以 |
| 顏色 | `ColorFormat` 是 12 bit（RGB444）、16 bit（RGB565 / BGR565）、18 / 24 bit（RGB666、RGB888…）；`init()` 依它設 COLMOD 和 MADCTL 的顏色順序 |
| 一致 | 自己呼叫 `colmod()`、`madctl()` 時要跟 `ColorFormat` 一致（保留 `COLMOD_VALUE`、`MADCTL_ORDER`） |
| 裁切 | `fillWindow`、`writeWindow` 會裁切；`setWindow` + `writePixel` 不會（video 用，開始時自己檢查） |
| 12 bit | 兩個像素擠 3 bytes：一段 `writePixel` 之後一定要 `endWrite()`，才會送出最後半個像素 |

## 檔案

```
lib/ST7789/
├── ST7789.h          入口：basic/ + tool/ + 所有模組（程式只 include 它）
├── basic/
│   ├── datasheet.h   Datasheet<Bus>：datasheet 第 9 章所有指令（Table 1、Table 2），照順序排
│   └── window.h      Window<Bus, ColorFormat>：顏色模式、setWindow、writePixel、fillWindow、writeWindow（後兩個會裁切）
├── module/
│   └── M130T.h       M130T<Bus, ColorFormat>：1.3 吋 240×240、沒有 CS 的模組（尺寸 + init）
└── tool/
    └── init.h        initDefault()：適用任何面板的最精簡初始化
```

繼承順序：`Datasheet` → `Window` → `M130T`，再往上是 `graphics::Shapes`、`graphics::Text`。

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
| `writePixel(color)`、`endWrite()` | 在 `setWindow` 之後一個一個送像素（依顏色模式送 1.5 / 2 / 3 bytes）、結束（12 bit 時補完最後半個像素） |
| `setOffset(x, y)` | 設定面板在控制器 240×320 記憶體裡的位置 |
| `Datasheet` 的指令 | 照 datasheet 原樣送出，例如 `madctl`、`colmod`、`invOn`、`dispOn` |

### 低層的畫法

`video` 的解碼器就是這樣畫的：

```cpp
screen.setWindow(x0, y0, x1, y1);   // 包含兩端，不裁切
for (uint16_t i = 0; i < count; i++) {
    screen.writePixel(color);       // 依顏色模式送 1.5 / 2 / 3 bytes
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
| 顏色 | `ColorFormat` 是 12 bit（R4 G4 B4）、16 bit（R5 G6 B5，RGB 或 BGR）、18 / 24 bit（每色 ≤ 8 bit） | 編譯錯誤 |
| `Datasheet` 的指令參數 | 見 datasheet 第 9 章 | 不檢查：照原樣送出 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `M130T<Bus, ColorFormat>` 約 10 bytes（寬、高、偏移量；12 bit 的半個像素） |
| 時間 | 一個窗口約 480 週期，一個像素約 44 週期，整個螢幕約 160 ms |
| 硬體 | Bus 的腳位（見 [Uno](../Uno/INTERFACE.md#unost7789st7789-螢幕的-bus)） |
