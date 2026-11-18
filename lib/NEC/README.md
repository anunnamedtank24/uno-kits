# NEC

| 需要 | 得到 |
|---|---|
| 接收：`InterruptPin`：`init(onChange)`、`read()`、`nowUs()`…（`Uno::UnoInterruptPin<PIN>`） | `NEC::HX1838`：收到的碼（哪個遙控器、哪個鍵、是不是按住），不會漏 |
| 遙控器的指令表 | `NEC::Remote17KeyNumbersTop`、`Remote17KeyArrowsTop`：碼換成鍵（`KEY_1`、`KEY_UP`…） |
| 發射：`Carrier`：38 kHz 開、關（`Uno::UnoIrCarrier<3>`） | `NEC::IRLED`：用紅外線 LED 當遙控器 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

紅外線遙控器的 NEC 協定，`namespace NEC`。接收器（`module/HX1838.h`）收到遙控器的光，這裡把它解碼成「哪個遙控器、哪個鍵、是不是按住」，再由遙控器的模組（`module/remote_17_key.h`）換成意思固定的按鍵（`KEY_1`、`KEY_UP`…）。也能反過來**發射**：用紅外線 LED 當遙控器（`transmitter.h`）。資料夾用通訊協定命名：所有 NEC 遙控器、接收器、發射器都用同一份協定。

## 用法

```cpp
#include "Uno_interrupt_pin.h"   // 只能在一個 .cpp 裡 include（裡面定義了中斷）
#include "NEC.h"

using Remote = NEC::Remote17KeyNumbersTop;                    // 你的遙控器（見遙控器）
constexpr uint8_t IR_PIN = 2;                                 // 接收器的 OUT（S）
using Receiver = NEC::HX1838<Uno::UnoInterruptPin<IR_PIN>>;

void setup() {
    Receiver::init();
}

void loop() {
    NEC::Code code;
    while (Receiver::read(code, 3)) {                          // 3：按住約 0.3 秒後才開始連續
        NEC::Key key = Remote::toKey(code);
        if (key == NEC::KEY_UP) {
            // 剛按下（code.repeats == 0），或按住中（code.repeats >= 3）
        }
    }
}
```

完整的例子：`src/week_5_NEC/tests/1_receiver.cpp`（把收到的碼印到串口，找出你的遙控器）、`src/week_5_NEC/assignments/`。

## 發射

```cpp
#include "Uno_ir_carrier.h"
#include "NEC.h"

constexpr uint8_t IR_LED_PIN = 3;                     // 紅外線 LED：D3 或 D11（38 kHz 由 Timer2 產生）
using Sender = NEC::IRLED<Uno::UnoIrCarrier<IR_LED_PIN>>;

Sender::init();
Sender::send(0x00, 0x45);                              // 位址、指令
Sender::sendKey<NEC::Remote17KeyNumbersTop>(NEC::KEY_OK);   // 用按鍵的名字
Sender::hold(0x00, 0x1C, 1000);                        // 按住 1 秒：一次按下，之後每 108 ms 一個重複碼
```

| 函式 | 說明 |
|---|---|
| `init()` | LED 熄滅 |
| `send(address, command)`、`send(code)` | 按一下：67.5 ms（會等）；位址 > 255 用擴充 NEC（16 位元位址） |
| `sendKey<Remote>(key)` | 遙控器的某個鍵；那個遙控器沒有這個鍵時回傳 false |
| `sendRepeat()` | 一個重複碼（「還按著」），要在上一次的 108 ms 後送 |
| `hold(address, command, ms)`、`holdKey<Remote>(key, ms)` | 按住 ms：按下 + 每 108 ms 一個重複碼 |

| 接法 | 腳 | UNO |
|---|---|---|
| 裸的紅外線 LED（940 nm，透明或淡藍色） | 長腳 | 220 Ω → **D3**（或 D11） |
| | 短腳 | GND |
| KY-005 板子 | S | 220 Ω → **D3**（很多板子上沒有電阻） |
| | 中間、- | 5V（有些板子沒接）、GND |

- **為什麼只能 D3 或 D11**：38 kHz 由 Timer2 直接從這兩隻腳輸出，不佔 CPU。同時不能用 D3 / D11 的 PWM、`UnoPcmOut`。
- **距離**：220 Ω 約 15 mA，1 ～ 2 m；100 Ω 約 30 mA（腳位上限 40 mA，不能再小）。要像真的遙控器 5 ～ 10 m，用 NPN 電晶體（2N2222、S8050）推約 100 mA。
- **看不到光**：用手機相機看，會看到 LED 在閃。
- **送的時候會等**：一次按下 67.5 ms，`loop()` 那段時間不動；中斷照常（`millis`、串口）。
- 例子：`src/week_5_NEC/tests/3_transmitter.cpp`（在串口按鍵就發射，同時用接收器收回來檢查）。

## 接線

| 接收器 | UNO |
|---|---|
| OUT（模組標 S，或晶片面對自己時的左腳） | 任何數位腳，例如 D2 |
| GND（-） | GND |
| VCC（+） | 5V |

- **晶片本身的腳位**：HX1838 / VS1838B 圓頂面對自己、腳朝下時，由左到右是 **OUT、GND、VCC**（2.7 ～ 5.5 V）。
- **焊在小板子上的（KY-022 等）**：板子重新拉過線，順序常常不同（中間常是 +），**照板子印的字接**（S = OUT、+ = VCC、- = GND）。
- **不用電阻**：輸出已經是 0 / 5 V。
- **遙控器**：17 鍵的用一顆 CR2025 鈕扣電池，第一次用要抽掉電池旁邊的透明片。

## 名稱

賣家常把好幾種名稱混在一起寫，它們其實是不同層級的東西：

| 名稱 | 是什麼 | 這個函式庫 |
|---|---|---|
| **HX1838**、**VS1838B** | 接收器「晶片」：黑色、前面一顆圓頂，3 隻腳。兩個是不同廠商做的同一種東西：收 38 kHz 的紅外線，輸出解調後的訊號 | `module/HX1838.h`（兩個名稱都有，程式一樣） |
| **KY-022** | 「模組」：把上面的晶片焊在小板子上，加一顆 LED 和電阻，標好 S、+、- | 同上 |
| **NEC** | 「通訊協定」：遙控器怎麼用光的長短表示 0 和 1（見[NEC 協定](INTERFACE.md#nec-協定)） | 這個資料夾，`Receiver` |
| 17 鍵遙控器 | 「遙控器」：每個鍵送一個指令碼。**不同遙控器的指令碼不一樣** | `module/remote_17_key.h` |

所以資料夾叫 `NEC`（協定），不叫 `infrared`（太籠統）也不叫 `HX1838`（換一顆接收器程式也一樣）；接收器和遙控器是實際買到的零件，放在 `module/`。

**8 隻腳的 SDA、SCK、MOSI、MISO、IRQ、GND、RST、3.3V 不是紅外線接收器**，那是 **RC522（MFRC522）RFID 讀卡模組**：用 13.56 MHz 的無線電讀卡片和磁扣，接 SPI，而且只能接 3.3V。它跟紅外線無關，以後要用的話會是另一個函式庫（見 [lib/README.md](../README.md#以後加東西放哪)）。

## 遙控器

**指令碼不通用**：每個遙控器自己決定送什麼。同樣叫「17 鍵」的就有兩種排法，指令也不同：

| | `Remote17KeyNumbersTop` | `Remote17KeyArrowsTop` |
|---|---|---|
| 排法 | 上面 1 ～ 9、* 0 #，下面方向鍵和 OK | 上面方向鍵和 OK，下面 1 ～ 9、* 0 # |
| 位址 | 0x00 | 0x00 |
| 1 2 3 | 0x45 0x46 0x47 | 0x16 0x19 0x0D |
| 4 5 6 | 0x44 0x40 0x43 | 0x0C 0x18 0x5E |
| 7 8 9 | 0x07 0x15 0x09 | 0x08 0x1C 0x5A |
| * 0 # | 0x16 0x19 0x0D | 0x42 0x52 0x4A |
| ▲ ◀ OK ▶ ▼ | 0x18 0x08 0x1C 0x5A 0x52 | 0x46 0x44 0x40 0x43 0x15 |

兩種是照網路上常見的碼寫的，**你的遙控器要先跑 `src/week_5_NEC/tests/1_receiver.cpp` 確認**：按幾個鍵，看哪一欄的名稱跟你按的一樣，作業開頭的 `using Remote = …` 就用那一個。兩個都不對時，照 `src/week_5_NEC/tests/1_receiver.cpp` 印出來的指令在 `module/` 加一個檔案。

**換遙控器只改一行**：程式只用 `KEY_1`、`KEY_UP` 這些意思，不直接寫指令碼。電視、冷氣的遙控器也能用，只要它是 NEC：在 `module/` 加一個有 `ADDRESS` 和 `toKey` 的 struct。

## 連按

| 情況 | 會送什麼 |
|---|---|
| 按一下就放 | 一個完整的碼；按超過約 0.1 秒會多一個重複碼（第二個碼在按下後 108 ms 才來） |
| 按住 | 一個完整的碼，接著每 108 ms 一個重複碼（每秒約 9 個），放開就停 |
| 很快連按兩下 | 兩個完整的碼 |

- **看遙控器**：NEC 遙控器（包括這兩種 17 鍵）按住時送重複碼；有些別的協定是一直重送完整的碼，或什麼都不送。
- **一般按一下也常有 1 個重複**：所以要「按一下動一格」時用 `read(code, 3)`，按住約 0.3 秒後才開始連續，像電腦鍵盤一樣。
- **不該連續的鍵**（OK、清除）：看 `code.repeats > 0` 就跳過。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 腳位 | 0 ～ 19；D0 / D1 編譯警告 | 編譯錯誤 |
| 時序誤差 | 約 ±30% | 不收（雜訊或別的協定） |
| 檢查碼 | 指令和指令取反要對得上 | 不收，`badCount()` 加 1 |
| 協定 | NEC、extended NEC | 不收（Sony、RC5、冷氣的長碼…） |
| `read` 的間隔 | 每個碼在佇列裡等，最多 `QUEUE` 個 | 滿了丟掉新的；按住時重複碼一直來，太久沒讀會掉幾個重複 |
| `QUEUE` | 1 ～ 255 | 0：編譯錯誤 |
| 距離、角度 | 約 5 ～ 8 m，要大致對著接收器 | 收不到 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| `Uno_interrupt_pin.h` 被兩個 .cpp include | 中斷函式重複定義，連結錯誤 | 只在一個 .cpp 裡 include |
| 同一組腳位有兩個 `UnoInterruptPin` | 後 `init` 的蓋掉前一個 | 換到別組（D0 ～ D7、D8 ～ D13、A0 ～ A5） |
| 日光、日光燈、手機鏡頭照著接收器 | 偶爾收到雜訊，`badCount()` 增加 | 正常；雜訊不會變成按鍵 |
| 跟 DS18B20（`UnoOneWire`）一起用 | 每個位元關中斷約 70 µs，量到的長度差一點 | 誤差在容許範圍內，可以一起用 |
| 收不到 | — | 用手機相機看遙控器前端，按下時會看到紫光；沒有就換電池或抽掉絕緣片；`src/week_5_NEC/tests/1_receiver.cpp` 有沒有印出東西 |
| 遙控器的鍵對不上 | 用錯 `Remote` | 跑 `src/week_5_NEC/tests/1_receiver.cpp` 確認 |
