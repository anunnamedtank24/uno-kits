# Uno

Arduino UNO（ATmega328P）的暫存器層程式，`namespace Uno`。

## 檔案

| 檔案 | 內容 | 角色 |
|---|---|---|
| `Uno_ST7789.h` | `Uno::UnoST7789<DC, RES, CS, BLK, MODE>` | **接口**：ST7789 螢幕的 Bus |
| `Uno_serial.h` | `Uno::UnoSerial<BAUD>` | **接口**：USB 串口的 Link（`GIF::StreamPlayer` 用） |
| `Uno_one_wire.h` | `Uno::UnoOneWire<PIN, INTERNAL_PULLUP>` | **接口**：1-Wire 匯流排的 Wire（`temp_sensor::DS18B20` 用） |
| `Uno_I2C.h` | `Uno::UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>` | **接口**：軟體 I2C 的 Bus（`SSD1306` OLED 用） |
| `Uno_pin.h` | `Uno::UnoPin<PIN, INTERNAL_PULLUP>` | **接口**：一隻數位輸入腳的 Pin（`switches::Switch` 用） |
| `Uno.h` | 腳位、SPI、UART、PWM、Flash、時間、腳位檢查 | 底層工具，上面的接口都會 include 它 |

要用哪個接口就 include 哪個，`Uno.h` 不會反過來 include 接口：

```cpp
#include "Uno_ST7789.h"
#include "Uno_serial.h"   // 需要串流時才加
#include "Uno_one_wire.h" // 接 1-Wire 感測器時才加
#include "Uno_I2C.h"      // 接 I2C 裝置（例如 OLED）時才加
#include "Uno_pin.h"      // 接開關時才加
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

## UnoOneWire：1-Wire 匯流排

```cpp
using SensorWire = Uno::UnoOneWire<2>;          // DQ 接 D2，順便開內部上拉
using SensorWire = Uno::UnoOneWire<2, false>;   // 只靠外接的 4.7 kΩ 上拉
```

- **函式**：`init`、`reset`（回傳有沒有裝置回應）、`writeBit`、`readBit`、`write8`、`read8`（低位元先）、`delayMs`、`nowMs`。
- **開汲極**：要輸出 LOW 時才把腳設成輸出；其他時候是輸入，讓上拉電阻拉成 HIGH，不會跟裝置搶線。
- **時序**：reset 480 µs、寫 1 拉低 6 µs、寫 0 拉低 60 µs、讀在拉低後 13 µs 取樣，每個位元約 70 µs。
- **關中斷**：每個位元的時序裡關中斷，避免 `millis()` 的中斷把時序拉長；reset 時線是空閒的，`reset()` 先檢查線是不是 HIGH，是 LOW 就直接回傳 false（沒有上拉或短路）。
- **任何數位腳都可以**：D0 / D1 會出警告（USB 串口）。

## UnoI2C：軟體 I2C

```cpp
using Bus = Uno::UnoI2C<13, 11>;              // SCL = D13, SDA = D11，開內部上拉，目標 400 kHz
using Bus = Uno::UnoI2C<13, 11, false, 100>;  // 只靠外接上拉，100 kHz
```

| 參數 | 說明 |
|---|---|
| `SCL`、`SDA` | 任何兩隻不同的腳；硬體 I2C 只在 A4 / A5，這個用軟體做，所以哪裡都可以 |
| `INTERNAL_PULLUP` | 預設 true：放開線時順便開內部約 35 kΩ 上拉（模組板上通常已經有 4.7 ～ 10 kΩ） |
| `KHZ` | 1 ～ 400，決定每半個時脈至少等多久（LOW ≥ 1.3 µs、HIGH ≥ 0.6 µs，fast mode 的下限） |

- **函式**：`init`、`start(address, read)`、`write8`（回傳 ACK）、`read8(ack)`、`stop`，以及給 `graphics::Text` 用的 `delayMs`、`nowMs`、`readFlash8/16`、`FlashString`。
- **開汲極**：跟 `UnoOneWire` 一樣，輸出 LOW 時才設成輸出，HIGH 是放開讓上拉電阻拉高。
- **等 SCL 真的變 HIGH**：放開 SCL 後會一直讀到 HIGH 才繼續（最多約 200 次），所以上拉太弱、或裝置拉住 SCL（clock stretching）只會變慢，不會出錯。
- **救卡住的匯流排**：`init()` 發現 SDA 被拉低（板子在傳輸途中重置，裝置還在送資料），會先送最多 9 個時脈再送 STOP。
- **實際速度**：`KHZ = 400` 時約 160 kHz。延遲只佔每個位元的 43 個週期，呼叫腳位函式另外要約 56 個週期。
- **不關中斷**：I2C 的時脈由主機決定，中斷只會讓某個時脈變長，不會出錯。

## UnoPin：一隻輸入腳

```cpp
using SwitchPin = Uno::UnoPin<2>;          // 開關模組的 OUT 接 D2（模組自己輸出高低，不開上拉）
using SwitchPin = Uno::UnoPin<2, true>;    // 裸按鈕接 D2 和 GND：開內部上拉，按下是 LOW
```

- **函式**：`init`（設成輸入）、`read`（true = HIGH）、`nowMs`。
- **任何數位腳都可以**：D0 / D1 會出警告（USB 串口）。

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
| `UnoOneWire<PIN>` | `uint8_t` | 0 ～ 19；D0 / D1 警告 | 編譯錯誤 |
| `UnoI2C<SCL, SDA>` | `uint8_t` | 0 ～ 19，兩隻不同；D0 / D1 警告 | 編譯錯誤 |
| `UnoI2C` 的 `KHZ` | `uint16_t` | 1 ～ 400 | 編譯錯誤 |
| `UnoPin<PIN>` | `uint8_t` | 0 ～ 19；D0 / D1 警告 | 編譯錯誤 |
| `readFlash8/16(p)` | 指標 | 必須指向 `PROGMEM` 的資料 | 讀到錯的資料 |
| `delayMs(ms)` | `uint16_t` | 0 ～ 65535 | — |

## 佔用的硬體

| 資源 | 誰用 | 影響 |
|---|---|---|
| D11（MOSI）、D13（SCK） | SPI，接螢幕 | 不能當一般腳位 |
| D12（MISO） | SPI 開著時強制當輸入 | 不能當輸出 |
| D10（SS） | `spiSet` 會設成輸出（SS 是輸入又被拉低時，SPI 會變成從機） | 可以拿來當 RES / CS / BLK，但不能當輸入 |
| D0、D1 | `UnoSerial`（USB 串口） | 不能同時用 Arduino 的 `Serial`；燒錄時也會用到 |
| `UnoI2C` 的兩隻腳 | 軟體 I2C | 例如 D13 / D11 時，就不能同時接 SPI 螢幕；D13 上的板載 LED 會跟著閃 |
| Timer0 | Arduino 的 `millis()`（`nowMs()`） | BLK 接 D5 / D6 時只能用固定頻率約 976 Hz 的 PWM |
| Timer1（D9、D10）、Timer2（D3、D11） | BLK 的 PWM | 會改掉計時器設定，跟 `Servo`、`tone()` 等用同一個計時器的函式庫衝突 |
| 中斷 | 都不用 | SPI、UART 都是輪詢，`update()` 要自己一直呼叫 |

## Uno.h 的工具

| 分類 | 內容 |
|---|---|
| 腳位功能表 | `PIN_FUNCTIONS[20]`，以及 `exists`、`has`、`optional`、`same`、`mask` |
| 數位腳 | `pinOutput<P>()`、`pinInput<P>(pullUp)`、`pinHigh<P>()`、`pinLow<P>()`、`pinRead<P>()`，`P` 是 `NONE` 時什麼都不做（讀到 false） |
| SPI / UART | `spiSet(mode)`：SPI 主模式，f/2 = 8 MHz；`uartSet(ubrr)`、`uartUbrr(baud)`：UART，8N1，倍速模式 |
| PWM | `pwmSet<P>(duty)`：Timer1/Timer2 用 fast PWM；Timer0（D5、D6）只開關輸出，不動它的設定，因為 millis 要用 |
| Flash / 時間 | `readFlash8/16`、`delayMs`（忙等，不靠 Timer0）、`nowMs`（millis） |
