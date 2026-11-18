# SSD1306

| 需要 | 得到 |
|---|---|
| 一個 `Bus`（I2C）：`init()`、`start(address, read)`、`write8()`、`stop()`…（`Uno::UnoI2C<SCL, SDA>`） | `SSD1306::OLED128x64`、`OLED128x32`：一個 Screen（`graphics`、`image`、`video` 都能畫），RAM 裡一整張畫面，`display()` 只送改變的部分；面板自己的指令（對比、捲動、淡出…） |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

I2C 介面的 OLED 螢幕驅動，控制器是 SSD1306，`namespace SSD1306`。它本身就是 Screen，`graphics`、`image`、`video` 的函式跟 ST7789 完全一樣。

參考資料：Solomon Systech SSD1306 datasheet Rev 1.1（8.1.5 I2C 介面、第 9 章指令表 Table 9-1、第 10 章指令說明），以及 datasheet 附的 charge pump 應用說明（指令 8Dh）。

## 用法

```cpp
#include "Uno_I2C.h"
#include "SSD1306.h"              // 只 include 入口，它包含 basic/ 和所有模組
#include "graphics.h"
#include "font.h"
#include "color.h"

constexpr uint8_t SCL_PIN = 13;                              // 任何兩隻腳都可以
constexpr uint8_t SDA_PIN = 11;
using Bus = Uno::UnoI2C<SCL_PIN, SDA_PIN>;
using ScreenColor = color::BW<128>;                          // 黑白：亮度 ≥ 128 的點亮
using Screen = graphics::Canvas<SSD1306::OLED128x64<Bus, ScreenColor>>;
Screen screen;                                               // 0x3D 的模組：Screen screen(0x3D);

void setup() {
    if (!screen.init()) {                                    // false：0x3C 沒有回應
        return;
    }
    screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
    screen.drawHollowRectSize(0, 0, 128, 64, ScreenColor::WHITE);
    screen.drawTexts(10, 26, ScreenColor::WHITE, "hello ", 42);
    screen.display();                                        // 畫完才送出去
}
```

- **畫完要 `display()`**：圖形和文字只改板子 RAM 裡的畫面，`display()` 才把改過的部分送到螢幕。
- **顏色**：顏色格式一定是 1 bit 的 `color::BW<門檻>`（給彩色的格式會編譯錯誤）；畫圖用 `ScreenColor::WHITE` / `BLACK`（= `ON` / `OFF`）。彩色的圖片、動畫依亮度和門檻決定每一點亮不亮。
- **圖片**：`lib/image` 用 `--palette 2`（黑白），見 [image](../image/README.md#在-oled-上畫)。
- **影片**：`video::FlashPlayer<Screen>` 可以直接播，轉換時加 `--mono`；每畫完一幀呼叫 `display()`。

完整的例子：`src/week_4_SSD1306_switch_buzzer_audio/tests/1_oled.cpp`（每一類指令）、`src/week_4_SSD1306_switch_buzzer_audio/assignments/2_animation.cpp`（影片）。

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

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 一片黑 | `init()` 失敗 | 檢查 VCC / GND、SCL / SDA 接反；位址可能是 0x3D |
| 只有上半部、每隔一列 | 其實是 128×32 的面板 | 改用 `SSD1306::OLED128x32` |
| 左右或上下鏡像 | 模組裝反了 | `init(true)` 轉 180° |
| 右邊有 2 欄雜點、整體偏移 | 控制器其實是 SH1106（1.3 吋常見，132 欄、沒有 20h ～ 22h） | 不支援 |
| 偶爾亂掉 | 線太長或上拉太弱 | 降低速度：`Uno::UnoI2C<SCL_PIN, SDA_PIN, true, 100>` |
| 捲動中呼叫 `display()` | datasheet 說捲動中不要改 RAM | 不檢查：先 `deactivateScroll()`，再 `displayAll()` |
| 跟 M130T 用同樣的 D11 / D13 | 兩個螢幕不能同時接在同一組腳 | OLED 改接別的腳 |
| RAM 不夠 | 一個 OLED 就佔 1 KB，UNO 只有 2 KB | 其他物件盡量放在函式裡（用完就還） |
