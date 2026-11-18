# Uno

| 需要 | 得到 |
|---|---|
| Arduino 的 `millis()`、`micros()`（Arduino UNO，ATmega328P） | 其他函式庫要的介面的 UNO 版本：Bus（`UnoST7789`、`UnoI2C`）、Link（`UnoSerial`）、Wire（`UnoOneWire`）、Pin、Pwm、InterruptPin、AnalogPin（`UnoInputPin`、`UnoPwmPin`、`UnoInterruptPin`、`UnoAnalogPin`）、Tone（`UnoTone`）、Carrier（`UnoIrCarrier`）、SampleOut（`UnoPcmOut`）、背景執行（`UnoBackground`）；接錯腳在編譯時就擋下 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

Arduino UNO（ATmega328P）的暫存器層，`namespace Uno`，四大類裡的「主板」。這是最底層：別的函式庫要的介面（「需要」那一欄）在這裡有 UNO 的版本，每個的函式見[提供的介面](INTERFACE.md#提供的介面)。換主板時，照同樣的函式為新的主板寫一份。

## 用法

用到哪個介面就 include 哪個檔案，`Uno.h` 不會反過來 include 介面：

```cpp
#include "Uno_ST7789.h"   // ST7789 螢幕
#include "Uno_I2C.h"      // I2C 裝置（OLED）
#include "Uno_serial.h"   // 從電腦串流圖片、影片
#include "Uno_one_wire.h" // 1-Wire 感測器
#include "Uno_pin.h"      // 開關、有源蜂鳴器、LED、馬達驅動
#include "Uno_analog_pin.h"  // 類比感測器（ADC）
#include "Uno_interrupt_pin.h" // 紅外線接收器、超音波（只能在一個 .cpp 裡 include）
#include "Uno_background.h"    // 背景執行 update()（只能在一個 .cpp 裡 include）
#include "Uno_tone.h"     // 無源蜂鳴器（只能在一個 .cpp 裡 include）
#include "Uno_pcm_out.h"  // 取樣（PCM）：人聲、真正的波形（只能在一個 .cpp 裡 include）

constexpr uint8_t DC_PIN = 8;
constexpr uint8_t RES_PIN = 10;
constexpr uint8_t BLK_PIN = 9;
using Bus = Uno::UnoST7789<DC_PIN, RES_PIN, Uno::NONE, BLK_PIN>;    // 沒有 CS
```

完整的例子：`src/` 的每個程式（parts 段落）；`src/week_7_L298N/tests/7_background_remote.cpp`（`UnoBackground`）、`src/week_1_analog/tests/1_sensors.cpp`（`UnoAnalogPin`、`UnoChip`）。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 腳位（template） | 0 ～ 19，或 `Uno::NONE`（只有選填的腳） | 編譯錯誤；D0 / D1 編譯警告 |
| `UnoPwmPin` 的腳位 | 3、5、6、9、10、11 | 編譯錯誤 |
| `UnoAnalogPin` 的腳位 | 14 ～ 19（A0 ～ A5） | 編譯錯誤 |
| `UnoPcmOut` 的腳位 | 9、10 | 編譯錯誤 |
| `UnoIrCarrier` 的腳位 | 3、11 | 編譯錯誤 |
| `MODE` | 0 ～ 3 | 編譯錯誤 |
| `BAUD` | 2000 ～ 2000000，誤差 ≤ 5% | 編譯錯誤；誤差 > 2% 編譯警告 |
| `UnoI2C` 的 `KHZ` | 1 ～ 400 | 編譯錯誤 |
| `init(blk)`、`setBacklight(blk)` | 0 ～ 255；BLK 不是 PWM 腳時只有 0 = 關、其他 = 開 | — |
| `read8(byte, timeoutMs)` | 0 ～ 65535 ms | — |
| `UnoTone::start(hz)` | 16 ～ 20000，0 = 停止 | 夾到範圍內 |
| `UnoPcmOut::start(rate, next)` | 1954 ～ 32000 Hz | 夾到範圍內 |
| `readAnalogAverage(count)` | 1 ～ 64 | 夾到範圍內 |
| `UnoBackground::add` | 最多 6 個函式 | 回傳 false |
| `readFlash8/16(p)` | 必須指向 `PROGMEM` 的資料 | 不檢查：讀到錯的資料 |
| `delayMs(ms)` | 0 ～ 65535 | — |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 電腦打開串口 | 板子重置（DTR），bootloader 約等 1 秒 | 先開電腦上的程式，再燒錄 |
| `Uno_tone.h`、`Uno_interrupt_pin.h`、`Uno_pcm_out.h`、`Uno_background.h` 被兩個 .cpp include | 中斷函式重複定義，連結錯誤 | 只在一個 .cpp 裡 include（這個專案一次只有一個 .cpp） |
| 同一組腳位（D0 ～ D7、D8 ～ D13、A0 ～ A5）有兩個 `UnoInterruptPin` | 後 `init` 的蓋掉前一個 | 放在不同組 |
| `UnoTone` 和 D9 / D10 的 `pwmSet` 同時用（例如 `UnoST7789` 的 BLK 接 D9） | 兩邊互改 Timer1，頻率和亮度都錯 | BLK 改接 D3、D5、D6、D11，或 `Uno::NONE` |
| `UnoTone` 在 D9 / D10 以外的腳，同時用 `UnoOneWire` | 關中斷時方波暫停，聲音抖一下 | 蜂鳴器接 D9 或 D10（不用中斷） |
| `REFERENCE_EXTERNAL` 以外的參考電壓，但 AREF 腳接了電壓 | 內部參考和外部電壓短路，可能燒壞 | AREF 有接東西時，全部用 `REFERENCE_EXTERNAL` |
| 背景函式太慢（超過 1 ms） | 下一輪被跳過（`skipped()` 會增加） | 背景只放 `update()` 這類不會等的函式 |
| 一隻腳接耗電的東西 | 一隻腳建議 ≤ 20 mA（絕對上限 40 mA），整顆晶片 ≤ 200 mA | 透過電晶體推 |
