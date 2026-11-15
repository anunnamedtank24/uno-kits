# Uno

Arduino UNO（ATmega328P）的暫存器層程式，`namespace Uno`。

## 檔案

| 檔案 | 內容 | 角色 |
|---|---|---|
| `Uno_ST7789.h` | `Uno::UnoST7789<DC, RES, CS, BLK, MODE>` | **接口**：ST7789 螢幕的 Bus |
| `Uno_serial.h` | `Uno::UnoSerial<BAUD>` | **接口**：USB 串口的 Link（`GIF::StreamPlayer` 用） |
| `Uno.h` | 腳位、SPI、UART、PWM、Flash、時間、腳位檢查 | 底層工具，上面兩個接口都會 include 它 |

要用哪個接口就 include 哪個，`Uno.h` 不會反過來 include 接口：

```cpp
#include "Uno_ST7789.h"
#include "Uno_serial.h"   // 需要串流時才加
```

## UnoST7789：螢幕的 Bus

```cpp
// DC = D8, RES = D10, 沒有 CS, BLK = D9, SPI mode 3（預設）
using Bus = Uno::UnoST7789<8, 10, Uno::NONE, 9>;
```

| 參數 | 說明 |
|---|---|
| `DC` | 必填 |
| `RES` | `Uno::NONE` = 只用軟體重置 |
| `CS` | `Uno::NONE` = 模組沒有 CS 腳（這時必須用 SPI mode 3） |
| `BLK` | `Uno::NONE` = 背光接 VCC；接 PWM 腳（3、5、6、9、10、11）可以調亮度 |
| `MODE` | SPI mode 0～3 |

- 固定接線：SCL 接 D13、SDA 接 D11（硬體 SPI，8 MHz）。
- 提供給上層的函式：`init`、`setBacklight`、`delayMs`、`nowMs`、`readFlash8/16`、`cmdMode`、`dataMode`、`select`、`deselect`、`rstHigh`、`rstLow`、`write8`、`write16`，以及讀取用的空函式（模組只能寫，不能讀）。
- **寫入不等待**：`write8`/`write16` 送出後馬上返回，CPU 可以趁傳送時準備下一個 byte。切換 DC、CS 之前會先等最後一個 byte 送完。

## UnoSerial：USB 串口

```cpp
using Link = Uno::UnoSerial<1000000>;   // 1 Mbaud（預設），D0 / D1
```

- **用輪詢，不用中斷**：所以不能同時使用 Arduino 的 `Serial`。
- **函式**：`init`、`available`、`read8(byte, timeoutMs)`、`write8`、`clearInput`。
- **鮑率**：在 16 MHz 下，1000000 和 2000000 剛好整除沒有誤差；115200 有 2.1% 誤差，會出警告。
- **板子會重置**：電腦打開串口時板子會重置（DTR），bootloader 大約要等 1 秒。

## 編譯期檢查

腳位都是 template 參數，編譯時就會檢查：

| 類型 | 情況 |
|---|---|
| **錯誤**（編譯失敗） | 腳位不存在；DC/RES/CS/BLK 用到 SPI 腳（D11、D12、D13）；沒有 CS 卻不是 mode 3；鮑率誤差超過 5% |
| **警告**（可以編譯） | 兩個訊號用同一隻腳；用到 D0/D1（USB 串口）；BLK 不是 PWM 腳；BLK 用 D5/D6（Timer0，頻率固定）；鮑率誤差超過 2% |

警告的寫法：`static_assert(Uno::warn::Xxx<條件>::ok, "")`。條件成立時會用到一個標了 deprecated 的成員，編譯器就會印出警告訊息。

## 輸入限制

| 參數 | 型別 | 範圍 | 超出時 |
|---|---|---|---|
| 腳位（template） | `uint8_t` | 0 ～ 19（D0～D13、A0～A5 = 14～19），或 `Uno::NONE` | 編譯錯誤或警告（上表） |
| `MODE` | `uint8_t` | 0 ～ 3 | 編譯錯誤 |
| `BAUD` | `uint32_t` | 2000 ～ 2000000，誤差 ≤ 5% | 編譯錯誤 |
| `init(blk)`、`setBacklight(blk)` | `uint8_t` | 0 ～ 255；BLK 不是 PWM 腳時只有 0 = 關、其他 = 開 | — |
| `read8(byte, timeoutMs)` | `uint16_t` ms | 0 ～ 65535 | — |
| `readFlash8/16(p)` | 指標 | 必須指向 `PROGMEM` 的資料 | 讀到錯的資料 |
| `delayMs(ms)` | `uint16_t` | 0 ～ 65535 | — |

## 佔用的硬體

| 資源 | 誰用 | 影響 |
|---|---|---|
| D11（MOSI）、D13（SCK） | SPI，接螢幕 | 不能當一般腳位 |
| D12（MISO） | SPI 開著時強制當輸入 | 不能當輸出 |
| D10（SS） | `spiSet` 會設成輸出（SS 是輸入又被拉低時，SPI 會變成從機） | 可以拿來當 RES / CS / BLK，但不能當輸入 |
| D0、D1 | `UnoSerial`（USB 串口） | 不能同時用 Arduino 的 `Serial`；燒錄時也會用到 |
| Timer0 | Arduino 的 `millis()`（`nowMs()`） | BLK 接 D5 / D6 時只能用固定頻率約 976 Hz 的 PWM |
| Timer1（D9、D10）、Timer2（D3、D11） | BLK 的 PWM | 會改掉計時器設定，跟 `Servo`、`tone()` 等用同一個計時器的函式庫衝突 |
| 中斷 | 都不用 | SPI、UART 都是輪詢，`update()` 要自己一直呼叫 |

## Uno.h 的工具

| 分類 | 內容 |
|---|---|
| 腳位功能表 | `PIN_FUNCTIONS[20]`，以及 `exists`、`has`、`optional`、`same`、`mask` |
| 數位腳 | `pinOutput<P>()`、`pinHigh<P>()`、`pinLow<P>()`，`P` 是 `NONE` 時什麼都不做 |
| SPI / UART | `spiSet(mode)`：SPI 主模式，f/2 = 8 MHz；`uartSet(ubrr)`、`uartUbrr(baud)`：UART，8N1，倍速模式 |
| PWM | `pwmSet<P>(duty)`：Timer1/Timer2 用 fast PWM；Timer0（D5、D6）只開關輸出，不動它的設定，因為 millis 要用 |
| Flash / 時間 | `readFlash8/16`、`delayMs`（忙等，不靠 Timer0）、`nowMs`（millis） |
