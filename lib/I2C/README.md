# I2C

I2C 介面的 OLED 螢幕驅動，控制器是 SSD1306，`namespace SSD1306`。只依賴一個 Bus，例如 `Uno::UnoI2C<SCL, SDA>`，所以換主板只要換 Bus。

參考資料：Solomon Systech SSD1306 datasheet Rev 1.1（8.1.5 I2C 介面、第 9 章指令表 Table 9-1、第 10 章指令說明），以及 datasheet 附的 charge pump 應用說明（指令 8Dh）。

## 入口

```cpp
#include "module/OLED_128x64.h"   // 有對應的模組：只 include 模組，它包含 I2C.h 的全部
#include "I2C.h"                  // 沒有對應的模組：basic/，自己決定尺寸和初始化
```

## 檔案

| 資料夾 | 檔案 | 類別 | 內容 |
|---|---|---|---|
| `basic/` | `datasheet.h` | `SSD1306::Datasheet<Bus>` | Table 9-1 的所有指令，照 datasheet 的分類和順序；外加 charge pump、讀狀態 |
| `basic/` | `frame.h` | `SSD1306::Frame<Bus, WIDTH, HEIGHT>` | 板子 RAM 裡的整張畫面、`fillWindow`、`writeWindow`、`display()` |
| `module/` | `OLED_128x64.h` | `SSD1306::OLED128x64<Bus>` | 0.96 吋 128×64 模組：尺寸 + `init()` |
| `module/` | `OLED_128x32.h` | `SSD1306::OLED128x32<Bus>` | 0.91 吋 128×32 模組 |

繼承順序：`Datasheet` → `Frame` → `OLED128x64`，再往上是 `graphics::Shapes`、`graphics::Text`，所以圖形和文字的函式跟 ST7789 完全一樣。

## 接線

| OLED | UNO |
|---|---|
| GND | GND |
| VCC | 5V 或 3.3V（模組上有穩壓） |
| SCL | 任何數位腳，例如 D13 |
| SDA | 任何數位腳，例如 D11 |

- **任何腳都可以**：`Uno::UnoI2C` 是軟體 I2C（bit-bang）。UNO 的硬體 I2C 只在 A4 / A5，這裡不用它。
- **上拉電阻**：模組板上通常已經有 4.7 ～ 10 kΩ。`UnoI2C` 預設也會開 UNO 內部約 35 kΩ 的上拉。
- **位址**：大部分模組是 0x3C（模組背面常寫 0x78，那是位址左移一位後的值）；SA0 接 VCC 的是 0x3D。
- **跟 M130T 共用 D11 / D13**：這兩支是 M130T 的 SPI 腳，所以兩個螢幕不能同時接在同一組腳上。

## 用法

```cpp
#include "Uno_I2C.h"
#include "module/OLED_128x64.h"
#include "graphics.h"
#include "font.h"

using Bus = Uno::UnoI2C<13, 11>;                             // SCL = D13, SDA = D11
using Screen = graphics::Canvas<SSD1306::OLED128x64<Bus>>;
Screen screen;

void setup() {
    if (!screen.init()) {                                    // false：0x3C 沒有回應
        return;
    }
    screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
    screen.drawHollowRectSize(0, 0, 128, 64, 1);
    screen.drawTexts(10, 26, 1, "hello ", 42);
    screen.display();                                        // 畫完才送出去
}
```

- **畫完要 `display()`**：圖形和文字只改板子 RAM 裡的那份畫面，`display()` 才把改過的部分送到螢幕。
- **顏色**：0 = 暗，其他任何值 = 亮，所以 `color::RGB565::BLACK` / `WHITE` 也可以用。
- **字型**：`menlo_height_23_first_32_last_126_default_63`（每字 8×23，一行 16 個字、2 行多）。12 px 的字（4×12，一行 32 個字、5 行）也能用，但筆畫太少，看起來很粗糙。
- **圖片**：`lib/image` 的圖片也能畫上去，用 `--palette 2`（黑白），見 [image/README.md](../image/README.md#在-oled-上畫)。
- **GIF**：`GIF::FlashPlayer<SSD1306::OLED128x64<Bus>>` 可以直接播。轉換時加 `--mono`（只有黑白），每畫完一幀呼叫 `display()`。實測 64×64 的 orbit 跑滿 16.7 fps，沒有跳幀，最慢一幀（I 幀）66 ms。範例：`src/week_4/assignment_2.cpp`。

## 為什麼要在 RAM 留一整張畫面

| 問題 | 原因 | 做法 |
|---|---|---|
| 不能只改一個點 | 螢幕 RAM 一個 byte 是同一欄的 8 列（一個 page），改一個點要先知道其他 7 個點 | 在板子 RAM 留一份（128×64 / 8 = 1024 bytes） |
| 不能讀螢幕 RAM | I2C 模式下讀出來的只有狀態位元組（datasheet 8.1.5） | 同上 |
| 送整張很慢 | 1024 bytes 經過軟體 I2C 要 64 ms | `display()` 只送改過的部分：每個 page 從第一個改過的欄到最後一個 |

`display()` 每送一個 page 要多一次設定窗口（`21h` 欄位址、`22h` page 位址），所以改一個點也要送大約 15 個 byte。

## Bus 介面

`basic/datasheet.h` 開頭有完整清單。Bus 是一個只有 static 函式的 struct：

| 函式 | 作用 |
|---|---|
| `init()` | 放開兩條線；有裝置卡在半個 byte 時，先送最多 9 個時脈再送 STOP |
| `start(address, read)` | START + 7 位元位址 + 讀寫位元，回傳有沒有 ACK |
| `write8(byte)` | 高位元先送，回傳有沒有 ACK |
| `read8(ack)` | 讀一個 byte；`ack = false` 表示這是最後一個 |
| `stop()` | STOP |
| `delayMs`、`nowMs`、`readFlash8/16`、`FlashString` | 給 `graphics::Text` 用（跟 ST7789 的 Bus 一樣） |

## 指令（`Datasheet`）

### I2C 傳輸格式（8.1.5）

| 開頭 | 意思 |
|---|---|
| 位址 `0111 10 SA0` + R/W | 0x3C 或 0x3D |
| 控制位元組 `0x00` | 後面全部是指令（和指令的參數） |
| 控制位元組 `0x40` | 後面全部是畫面資料，寫進目前的位址窗口 |
| 讀 | 只能讀到狀態位元組：bit 6 = 1 表示顯示關閉 |

低階函式：`beginCommands()`、`beginData()`、`writeByte()`、`end()`、`sendCommand(…)`、`readStatus(status)`、`ok()`（上一次傳輸有沒有全部 ACK）。

### Table 9-1

| 分類 | 指令 | 函式 | 參數範圍（重置值） |
|---|---|---|---|
| 1. 基本 | 81h | `setContrast(c)` | 0 ～ 255（0x7F） |
| | A4h / A5h | `setEntireDisplayOn(on)` | true = 全亮，RAM 不顯示但保留 |
| | A6h / A7h | `setInverse(inverse)` | true = 反相 |
| | AEh / AFh | `setDisplayOn(on)` | false = 睡眠（重置），RAM 保留 |
| 2. 捲動 | 26h / 27h | `setupHorizontalScroll(left, start, interval, end)` | page 0 ～ 7；`interval` 用 `SCROLL_x_FRAMES` |
| | 29h / 2Ah | `setupDiagonalScroll(left, start, interval, end, offset)` | 垂直位移 1 ～ 63 列 |
| | 2Eh / 2Fh | `deactivateScroll()`、`activateScroll()` | 停止後要重送 RAM |
| | A3h | `setVerticalScrollArea(top, rows)` | 上方固定列數、捲動列數 |
| 3. 定址 | 00h ～ 1Fh | `setPageModeColumn(column)` | 0 ～ 127，只用在 page 模式 |
| | 20h | `setAddressingMode(mode)` | `ADDRESSING_HORIZONTAL` / `VERTICAL` / `PAGE`（重置） |
| | 21h | `setColumnAddress(start, end)` | 0 ～ 127（0, 127） |
| | 22h | `setPageAddress(start, end)` | 0 ～ 7（0, 7） |
| | B0h ～ B7h | `setPageStart(page)` | 0 ～ 7，只用在 page 模式 |
| 4. 硬體設定 | 40h ～ 7Fh | `setStartLine(line)` | 0 ～ 63（0）：畫面往上捲 |
| | A0h / A1h | `setSegmentRemap(remap)` | true = 左右鏡像 |
| | A8h | `setMultiplexRatio(rows)` | 16 ～ 64（64） |
| | C0h / C8h | `setComScanRemap(remap)` | true = 上下鏡像 |
| | D3h | `setDisplayOffset(offset)` | 0 ～ 63（0） |
| | DAh | `setComPins(alternative, remap)` | 128×64：`true`；128×32：`false` |
| 5. 時序 | D5h | `setClock(divide, frequency)` | 1 ～ 16（1）、0 ～ 15（8） |
| | D9h | `setPrecharge(phase1, phase2)` | 各 1 ～ 15（2, 2）；內建 charge pump 用 1, 15 |
| | DBh | `setVcomhDeselect(level)` | `VCOMH_0_65` / `VCOMH_0_77`（重置）/ `VCOMH_0_83` |
| | E3h | `nop()` | — |
| charge pump | 8Dh | `setChargePump(enable)` | 3.3 / 5 V 模組都要開；下一次 `setDisplayOn(true)` 才生效 |
| 6. 進階 | 23h | `setFadeBlink(mode, interval)` | `FADE_BLINK_OFF` / `FADE_OUT` / `BLINK`；每 8 × (interval + 1) 幀改一階亮度 |
| | D6h | `setZoomIn(zoom)` | true = 每列顯示兩次（上半部放大到整個螢幕），需要 `setComPins(true, …)` |

### 模組的 `init(flip)`

照 datasheet 的軟體初始化流程，依序是：

1. 關顯示
2. multiplex 64
3. 位移 0
4. 起始列 0
5. 左右、上下鏡像（`flip = false` 時排針在上方）
6. COM 腳設定
7. 對比 0xCF
8. 跟隨 RAM、正常顯示
9. 時脈 0x80
10. pre-charge 0xF1
11. VCOMH 0.77
12. charge pump 開
13. 水平定址
14. 停止捲動
15. 清空並送出整張畫面（上電時 RAM 是亂的）
16. 開顯示

| 模組 | multiplex | COM 腳 | 對比 |
|---|---|---|---|
| `OLED128x64` | 64 | alternative | 0xCF |
| `OLED128x32` | 32 | sequential | 0x8F |

回傳 `false` 表示位址沒有回應：檢查接線、電源、位址。

## `Frame`

| 函式 | 說明 |
|---|---|
| `width()`、`height()` | 128 × 64（或 32） |
| `fillWindow(x, y, w, h, color)` | 實心矩形，裁切；一次處理整個 byte（8 列） |
| `writeWindow(x, y, w, h, colors)` | 顏色陣列（逐列），裁切 |
| `getPixel(x, y)` | 板子 RAM 裡的點，螢幕外回傳 false |
| `clear()` | 全暗 |
| `setWindow(x0, y0, x1, y1)`、`writeData16(color)`、`endWrite()` | 跟 ST7789 的窗口一樣一個一個送像素，這裡是寫進 RAM 裡的畫面（螢幕外的像素丟掉）。`GIF::FlashPlayer` 就是用這三個畫的 |
| `display()` | 只送改過的部分，回傳螢幕有沒有回應 |
| `displayAll()` | 全部重送（例如捲動停止之後） |

## 速度（實測，16 MHz UNO）

| 動作 | 時間 |
|---|---|
| `displayAll()`：1024 bytes | 64 ms（約 16 fps） |
| `display()`：只改了一行字 | 約 3 ms |
| `init()` | 68 ms |
| 匯流排實際速度 | 約 160 kHz（`KHZ = 400` 時） |

- **限制在程式本身**：每個位元的延遲只有 43 個週期，但呼叫 `Uno.h` 的腳位函式另外要 56 個週期。
- **強制展開也沒用**：實測用 `always_inline` 強制展開，整張畫面反而變成 71 ms，所以沒有採用。
- **動畫要快**：讓每一幀改的範圍小一點，`display()` 就只送那一小塊。`test_1` 的彈跳球就是這樣做的。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `Frame` 的 `WIDTH` | 1 ～ 128 | 編譯錯誤 |
| `Frame` 的 `HEIGHT` | 8 ～ 64，8 的倍數 | 編譯錯誤 |
| 圖形、文字、`fillWindow`、`writeWindow` | 任何座標 | 裁切：只畫螢幕上的部分 |
| 寬或高 ≤ 0 | — | 不畫 |
| 指令參數 | 見上表 | 多出的位元被遮掉（例如 page 只取低 3 位元）；`setMultiplexRatio`、`setClock`、`setPrecharge` 夾到範圍內 |
| 位址沒有回應 | — | `init()` 回傳 false；`ok()`、`display()`、`readStatus()` 回傳 false；不會卡住 |
| 捲動中呼叫 `display()` | datasheet 說捲動中不要改 RAM | 不檢查：先 `deactivateScroll()`，再 `displayAll()` |
| `writeWindow` 的陣列 | 至少 w × h | 不檢查 |
| SH1106（1.3 吋常見） | 132 欄、沒有 20h / 21h / 22h | 不支援：畫面會偏 2 欄或亂掉 |
| 一個螢幕的 RAM | 128×64：1024 bytes 畫面 + 19 bytes 其他 | UNO 只有 2048 bytes，剩下的要留給堆疊 |
