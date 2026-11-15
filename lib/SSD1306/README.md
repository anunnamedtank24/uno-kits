# SSD1306

I2C 介面的 OLED 螢幕驅動，控制器是 SSD1306，`namespace SSD1306`。只依賴一個 Bus（例如 `Uno::UnoI2C<SCL, SDA>`），換主板只要換 Bus。它本身就是 Screen，`graphics`、`image`、`video` 的函式跟 ST7789 完全一樣。

參考資料：Solomon Systech SSD1306 datasheet Rev 1.1（8.1.5 I2C 介面、第 9 章指令表 Table 9-1、第 10 章指令說明），以及 datasheet 附的 charge pump 應用說明（指令 8Dh）。

## 檔案

```
lib/SSD1306/
├── SSD1306.h           入口：basic/ + 所有模組（程式只 include 它）
├── basic/
│   ├── datasheet.h     Datasheet<Bus>：Table 9-1 的所有指令，照 datasheet 的分類和順序；charge pump、讀狀態
│   └── frame.h         Frame<Bus, WIDTH, HEIGHT>：板子 RAM 裡的整張畫面、fillWindow、writeWindow、display
└── module/
    ├── OLED_128x64.h   OLED128x64<Bus>：0.96 吋 128×64（尺寸 + init）
    └── OLED_128x32.h   OLED128x32<Bus>：0.91 吋 128×32
```

繼承順序：`Datasheet` → `Frame` → `OLED128x64`，再往上是 `graphics::Shapes`、`graphics::Text`。

## 接線

| OLED | UNO |
|---|---|
| GND | GND |
| VCC | 5V 或 3.3V（模組上有穩壓） |
| SCL | 任何數位腳，例如 D13 |
| SDA | 任何數位腳，例如 D11 |

- **任何腳都可以**：`Uno::UnoI2C` 是軟體 I2C。UNO 的硬體 I2C 只在 A4 / A5，這裡不用它。
- **上拉電阻**：模組板上通常已經有 4.7 ～ 10 kΩ；`UnoI2C` 預設也會開 UNO 內部約 35 kΩ 的上拉。
- **位址**：大部分模組是 0x3C（模組背面常寫 0x78，那是左移一位後的值）；SA0 接 VCC 的是 0x3D。

## 用法

```cpp
#include "Uno_I2C.h"
#include "SSD1306.h"              // 只 include 入口，它包含 basic/ 和所有模組
#include "graphics.h"
#include "font.h"

using Bus = Uno::UnoI2C<13, 11>;                             // SCL = D13、SDA = D11
using Screen = graphics::Canvas<SSD1306::OLED128x64<Bus>>;
Screen screen;                                               // 0x3D 的模組：Screen screen(0x3D);

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

- **畫完要 `display()`**：圖形和文字只改板子 RAM 裡的畫面，`display()` 才把改過的部分送到螢幕。
- **顏色**：0 = 暗，其他任何值 = 亮，所以 `color::RGB565::BLACK` / `WHITE` 也可以用。
- **圖片**：`lib/image` 用 `--palette 2`（黑白），見 [image](../image/README.md#在-oled-上畫)。
- **影片**：`video::FlashPlayer<SSD1306::OLED128x64<Bus>>` 可以直接播，轉換時加 `--mono`；每畫完一幀呼叫 `display()`。

完整的例子：`src/week_4/test_1.cpp`（每一類指令）、`src/week_4/assignment_2.cpp`（影片）。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Bus` | `init()`、`start(address, read)`（回傳 ACK）、`write8(byte)`（回傳 ACK）、`read8(ack)`、`stop()`；給 `graphics::Text` 的 `delayMs`、`nowMs`、`readFlash8/16`、型別 `FlashString` | `Uno::UnoI2C` |

完整清單在 `basic/datasheet.h` 開頭。

## 函式

### 畫面（`Frame`）

| 函式 | 說明 |
|---|---|
| `init(flip)`（模組） | 照 datasheet 的流程初始化，回傳位址有沒有回應；`flip = true` 轉 180° |
| `width()`、`height()` | 回傳 128 × 64（或 32） |
| `fillWindow(x, y, w, h, color)` | 填一個實心矩形，裁切；一次處理整個 byte（8 列） |
| `writeWindow(x, y, w, h, colors)` | 填顏色陣列（一列一列），裁切 |
| `getPixel(x, y)` | 回傳板子 RAM 裡的點，螢幕外回傳 false |
| `clear()` | 全部變暗 |
| `setWindow(x0, y0, x1, y1)`、`writeData16(color)`、`endWrite()` | 跟 ST7789 一樣一個一個送像素，這裡是寫進 RAM 裡的畫面（螢幕外的像素丟掉）；`video` 用這三個畫 |
| `display()` | 只送改過的部分，回傳螢幕有沒有回應 |
| `displayAll()` | 全部重送（例如捲動停止之後） |

### 傳輸（`Datasheet`）

| 函式 | 說明 |
|---|---|
| `beginCommands()`、`beginData()` | 開始一串指令（控制位元組 0x00）、畫面資料（0x40） |
| `writeByte(byte)`、`end()` | 送一個 byte、結束 |
| `sendCommand(…)` | 送一個指令和它的參數（0 ～ 2 個，或一個陣列） |
| `readStatus(status)` | 讀狀態位元組（bit 6 = 1 表示顯示關閉），回傳有沒有回應 |
| `ok()` | 回傳上一次傳輸有沒有全部 ACK |

### 指令（Table 9-1）

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
| 6. 進階 | 23h | `setFadeBlink(mode, interval)` | `FADE_BLINK_OFF` / `FADE_OUT` / `BLINK`；每 8 ×（interval + 1）幀改一階亮度 |
| | D6h | `setZoomIn(zoom)` | true = 每列顯示兩次（上半部放大到整個螢幕），需要 `setComPins(true, …)` |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Frame` 的 `WIDTH`、`HEIGHT` | 模組決定 | 1 ～ 128；8 ～ 64，8 的倍數 |
| 建構子的位址 | 0x3C | `Screen screen(0x3D);` 改成 0x3D |

## I2C 傳輸格式（8.1.5）

| 開頭 | 意思 |
|---|---|
| 位址 `0111 10 SA0` + R/W | 0x3C 或 0x3D |
| 控制位元組 `0x00` | 後面全部是指令（和參數） |
| 控制位元組 `0x40` | 後面全部是畫面資料，寫進目前的位址窗口 |
| 讀 | 只能讀到狀態位元組：bit 6 = 1 表示顯示關閉 |

## 為什麼要在 RAM 留一整張畫面

| 問題 | 原因 | 做法 |
|---|---|---|
| 不能只改一個點 | 螢幕 RAM 一個 byte 是同一欄的 8 列（一個 page），改一個點要先知道其他 7 個點 | 在板子 RAM 留一份（128×64 / 8 = 1024 bytes） |
| 不能讀螢幕 RAM | I2C 模式下讀到的只有狀態位元組（8.1.5） | 同上 |
| 送整張很慢 | 1024 bytes 經過軟體 I2C 要 64 ms | `display()` 只送改過的部分：每個 page 從第一個改過的欄到最後一個 |

`display()` 每送一個 page 要多一次設定窗口（21h、22h），所以改一個點也要送大約 15 個 byte。

## 模組的 init(flip)

照 datasheet 的軟體初始化流程：

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

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `Frame` 的 `WIDTH` | 1 ～ 128 | 編譯錯誤 |
| `Frame` 的 `HEIGHT` | 8 ～ 64，8 的倍數 | 編譯錯誤 |
| 圖形、文字、`fillWindow`、`writeWindow` 的座標 | 任意 | 裁切 |
| 寬或高 | > 0 | 不畫 |
| 指令參數 | 見指令表 | 多出的位元被遮掉（例如 page 只取低 3 位元）；`setMultiplexRatio`、`setClock`、`setPrecharge` 夾到範圍內 |
| 位址沒有回應 | — | 回傳錯誤：`init()`、`display()`、`readStatus()`、`ok()` 回傳 false，不會卡住 |
| `writeWindow` 的陣列 | 至少 w × h 個 | 不檢查 |

## 資源

| 項目 | 用量（實測，16 MHz UNO） |
|---|---|
| RAM | `OLED128x64<Bus>` 1043 bytes：畫面 1024 + 每個 page 改過的範圍 16 + 位址、狀態 3 |
| `displayAll()`：1024 bytes | 64 ms（約 16 fps） |
| `display()`：只改一行字 | 約 3 ms |
| `init()` | 68 ms |
| 匯流排速度 | 約 160 kHz（`KHZ = 400` 時，見 [Uno](../Uno/README.md#unoi2ci2c-的-bus)） |
| 硬體 | Bus 的兩隻腳 |

- **限制在程式本身**：每個位元的延遲只有 43 個週期，呼叫腳位函式另外要 56 個週期。實測用 `always_inline` 強制展開，整張反而變成 71 ms，所以沒有採用。
- **動畫要快**：讓每一幀改的範圍小一點，`display()` 就只送那一小塊（`test_1` 的彈跳球）。

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 一片黑 | `init()` 失敗 | 檢查 VCC / GND、SCL / SDA 接反；位址可能是 0x3D |
| 只有上半部、每隔一列 | 其實是 128×32 的面板 | 改用 `SSD1306::OLED128x32` |
| 左右或上下鏡像 | 模組裝反了 | `init(true)` 轉 180° |
| 右邊有 2 欄雜點、整體偏移 | 控制器其實是 SH1106（1.3 吋常見，132 欄、沒有 20h ～ 22h） | 不支援 |
| 偶爾亂掉 | 線太長或上拉太弱 | 降低速度：`Uno::UnoI2C<13, 11, true, 100>` |
| 捲動中呼叫 `display()` | datasheet 說捲動中不要改 RAM | 不檢查：先 `deactivateScroll()`，再 `displayAll()` |
| 跟 M130T 用同樣的 D11 / D13 | 兩個螢幕不能同時接在同一組腳 | OLED 改接別的腳 |
| RAM 不夠 | 一個 OLED 就佔 1 KB，UNO 只有 2 KB | 其他物件盡量放在函式裡（用完就還） |
