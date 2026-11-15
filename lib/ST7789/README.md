# ST7789

ST7789 螢幕控制器的驅動，`namespace ST7789`。只依賴一個 Bus，Bus 例如 `Uno::UnoST7789`，所以換主板只要換 Bus。

## 入口

```cpp
#include "module/M130T.h"    // 有對應的模組：只 include 模組，它包含 ST7789.h 的全部
#include "ST7789.h"          // 沒有對應的模組：basic/ + tool/，自己設定尺寸和初始化
```

## 檔案

| 資料夾 | 檔案 | 類別 | 內容 |
|---|---|---|---|
| `basic/` | `datasheet.h` | `ST7789::Datasheet<Bus>` | datasheet 第 9 章所有指令（Table 1、Table 2），照 datasheet 的順序排；外加模組尺寸和偏移量 |
| `basic/` | `window.h` | `ST7789::Window<Bus>` | `setWindow`、`fillWindow`、`writeWindow`，後兩個會裁切 |
| `module/` | `M130T.h` | `ST7789::M130T<Bus>` | 1.3 吋 240×240、沒有 CS 的模組：尺寸 + `init()` 初始化流程 |
| `tool/` | `init.h` | `ST7789::initDefault()` | 適用任何面板的最精簡初始化 |

繼承順序：`Datasheet` → `Window` → `M130T`，再往上是 `graphics::Shapes`、`graphics::Text`。

## Bus 介面

`datasheet.h` 開頭有完整清單。Bus 是一個只有 static 函式的 struct：

- `init`、`setBacklight`、`delayMs`、`nowMs`、`readFlash8/16`
- `cmdMode`、`dataMode`（DC 腳）、`select`、`deselect`（CS 腳）
- `HAS_RES`、`rstHigh`、`rstLow`（RES 腳）
- `write8`、`write16`：可以在 byte 送完之前就返回，但 DC、CS 切換前必須等它送完
- `readMode`、`writeMode`、`readDummy`、`read8`：模組只能寫的話就留空

## 用法

```cpp
using Bus = Uno::UnoST7789<8, 10, Uno::NONE, 9>;
ST7789::M130T<Bus> screen;
screen.init();                               // Bus init + 重置 + M130T 的初始化流程
screen.fillWindow(0, 0, 240, 240, 0xF800);   // 整個螢幕填紅色
```

## 輸入限制

### 裁切

`fillWindow`、`writeWindow` 會裁切，其他東西（graphics 的圖形、文字）都經過它們，所以只要這裡裁一次就夠了：

1. 寬或高 ≤ 0：不畫。
2. 左邊超出（x < 0）：寬度減掉超出的部分，x 移到 0；上面同理。
3. 右邊超出（x + w > width）：寬度縮成 width − x；下面同理。
4. 剩下的寬或高 ≤ 0（整個在螢幕外）：不畫。
5. `writeWindow` 另外跳過陣列裡被切掉的顏色：每一列從 (可見的 y − y) × w + (可見的 x − x) 開始讀。

`setWindow` 不裁切：它是直接設定控制器的窗口，給 GIF 這種自己送像素的用。超出時控制器會把位址繞回去或忽略，畫面會錯，但不會損壞任何東西。


| 函式 | 參數 | 範圍 | 超出時 |
|---|---|---|---|
| `setWindow(x0, y0, x1, y1)` | `uint16_t`，包含兩端 | 0 ≤ x0 ≤ x1 < width，0 ≤ y0 ≤ y1 < height | **不裁切**，畫面錯誤 |
| `fillWindow(x, y, w, h, color)` | `int16_t` | 任意，可以在螢幕外 | 裁掉螢幕外的部分；w 或 h ≤ 0：不畫 |
| `writeWindow(x, y, w, h, colors)` | `int16_t` + RAM 裡的 `uint16_t` 陣列 | 同上，陣列有 w×h 個，一列一列 | 裁掉螢幕外的部分；陣列不夠長會讀到別的 RAM |
| `init(blk)` | `uint8_t` | 背光 0 ～ 255 | — |
| `setOffset(x, y)` | `uint16_t` | 面板在控制器 240×320 記憶體裡的位置 | 畫面錯誤 |
| 顏色 | `uint16_t` | RGB565（`COLMOD_16BIT`） | — |

### 不檢查的事

| 操作 | 會怎樣 | 該怎麼做 |
|---|---|---|
| `madctl()` 旋轉畫面（MV、MX、MY） | `width()`、`height()`、偏移量都不會跟著變，裁切和座標會錯 | 旋轉 90° 時自己交換寬高、重設 `setOffset` |
| `colmod()` 改成 12 / 18 bit | `fillWindow` 等還是送 16 bit 顏色，畫面錯亂 | 保持 `COLMOD_16BIT` |
| `Datasheet` 的所有指令 | 照 datasheet 原樣送出，不檢查參數 | 參數範圍看 datasheet 第 9 章 |
| NVM 指令（`nvmSet` 等） | **燒錄面板的一次性記憶體，改不回來** | 除非確定，否則不要呼叫 |
| `ramRd()`、`rddid()` 等讀取 | UNO 的 Bus 沒有讀取線，一律回傳 0 | — |
| 建構子 `M130T()` | 固定 240×240；`Datasheet(width, height)` 可以自訂 | 換模組時寫新的 module 檔 |

### 低層函式

`setWindow` 之後用 `writeData16()` 一個一個送像素，最後 `endWrite()`。GIF 解碼器就是這樣用的：

```cpp
screen.setWindow(x0, y0, x1, y1);   // 包含兩端，不裁切
for (uint16_t i = 0; i < count; i++) {
    screen.writeData16(color);
}
screen.endWrite();
```

- 一個窗口 = CASET + RASET + RAMWR，約 480 週期；之後每個像素約 44 週期。
- 送的像素數超過窗口大小時，控制器會從窗口的開頭繼續寫。

## 備註

- **NVM 指令**：會燒錄面板的一次性記憶體（OTP），寫了就改不回來，宣告旁有警告。
- **讀取指令**：都有寫，但 UNO 的 Bus 沒有讀取線，所以會回傳 0。
- **座標**：`setWindow` 會加上模組的偏移量（`setOffset`）。M130T 的 240×240 面板位在控制器 240×320 記憶體的左上角。
- **畫面撕裂**：M130T 沒有拉出 TE 腳，沒辦法跟面板刷新同步，大面積變化時可能看得到。
