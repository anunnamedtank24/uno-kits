# NEC

紅外線遙控器的 NEC 協定，`namespace NEC`。接收器（`module/HX1838.h`）收到遙控器的光，這裡把它解碼成「哪個遙控器、哪個鍵、是不是按住」，再由遙控器的模組（`module/remote_17_key.h`）換成意思固定的按鍵（`KEY_1`、`KEY_UP`…）。資料夾用通訊協定命名：所有 NEC 遙控器、所有 38 kHz 接收器都用同一份解碼程式。只依賴一個 InterruptPin（例如 `Uno::UnoInterruptPin`），換主板只要換它。

## 檔案

```
lib/NEC/
├── NEC.h                   入口：全部
├── receiver.h              Receiver<InterruptPin, ACTIVE_LOW, QUEUE>：在中斷裡解 NEC，Code = 位址 + 指令 + 第幾次重複
├── key.h                   Key：KEY_0 ～ KEY_9、方向鍵、OK、*、#（每個遙控器都一樣）；keyDigit、keyName
└── module/
    ├── HX1838.h            HX1838<InterruptPin>、VS1838B<InterruptPin>：接收器（KY-022 也是）
    └── remote_17_key.h     Remote17KeyNumbersTop、Remote17KeyArrowsTop：Arduino 套件的 17 鍵遙控器，指令 → Key
```

## 名稱

賣家常把好幾種名稱混在一起寫，它們其實是不同層級的東西：

| 名稱 | 是什麼 | 這個函式庫 |
|---|---|---|
| **HX1838**、**VS1838B** | 接收器「晶片」：黑色、前面一顆圓頂，3 隻腳。兩個是不同廠商做的同一種東西：收 38 kHz 的紅外線，輸出解調後的訊號 | `module/HX1838.h`（兩個名稱都有，程式一樣） |
| **KY-022** | 「模組」：把上面的晶片焊在小板子上，加一顆 LED 和電阻，標好 S、+、- | 同上 |
| **NEC** | 「通訊協定」：遙控器怎麼用光的長短表示 0 和 1（見[NEC 協定](#nec-協定)） | 這個資料夾，`Receiver` |
| 17 鍵遙控器 | 「遙控器」：每個鍵送一個指令碼。**不同遙控器的指令碼不一樣** | `module/remote_17_key.h` |

所以資料夾叫 `NEC`（協定），不叫 `infrared`（太籠統）也不叫 `HX1838`（換一顆接收器程式也一樣）；接收器和遙控器是實際買到的零件，放在 `module/`。

**8 隻腳的 SDA、SCK、MOSI、MISO、IRQ、GND、RST、3.3V 不是紅外線接收器**，那是 **RC522（MFRC522）RFID 讀卡模組**：用 13.56 MHz 的無線電讀卡片和磁扣，接 SPI，而且只能接 3.3V。它跟紅外線無關，以後要用的話會是另一個函式庫（見 [lib/README.md](../README.md#以後加東西放哪)）。

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

## 用法

```cpp
#include "Uno_interrupt_pin.h"   // 只能在一個 .cpp 裡 include（裡面定義了中斷）
#include "NEC.h"

using Remote = NEC::Remote17KeyNumbersTop;                    // 你的遙控器（見遙控器）
using Receiver = NEC::HX1838<Uno::UnoInterruptPin<2>>;        // 接收器的 OUT 接 D2

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

完整的例子：`src/week_5/test_1.cpp`（把收到的碼印到串口，找出你的遙控器）、`src/week_5/assignment_1.cpp` ～ `assignment_3.cpp`。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `InterruptPin` | `init(onChange)`（輸入腳，每次電位改變就在中斷裡呼叫 `onChange()`）、`read()`（true = HIGH）、`nowUs()`（中斷裡也能用）、`nowMs()`、`stopInterrupts()`、`restoreInterrupts(saved)` | `Uno::UnoInterruptPin` |
| `Remote` | `static constexpr uint16_t ADDRESS`、`Key toKey(const Code &code)`（別的位址或不認識的指令回傳 `KEY_NONE`） | `NEC::Remote17KeyNumbersTop`、`Remote17KeyArrowsTop` |

## 函式

`Receiver` 的函式都是 static（中斷沒有物件可以呼叫），所以寫成 `Receiver::read(code)`。

| 函式 | 說明 |
|---|---|
| `Receiver::init()` | 設定腳位和中斷，清空佇列 |
| `Receiver::read(code)` | 拿出最早收到、還沒讀的碼；沒有就回傳 false |
| `Receiver::read(code, repeatDelay)` | 同上，但跳過第 1 ～ repeatDelay - 1 次重複（像鍵盤：按一下、停一下、才開始連續）；255 = 只要按下 |
| `Receiver::isHeld()` | 回傳鍵是不是還按著（碼還在一直來） |
| `Receiver::badCount()` | 回傳時序像 NEC、但檢查碼不對的次數（雜訊或別種協定） |
| `Receiver::clear()` | 丟掉還沒讀的碼 |
| `Remote::toKey(code)` | 回傳碼代表的鍵；別的遙控器或不認識的鍵是 `KEY_NONE` |
| `keyDigit(key)` | 回傳 `KEY_0` ～ `KEY_9` 的數字 0 ～ 9，其他鍵 -1 |
| `keyName(key)` | 回傳簡短的名稱（`"1"`、`"UP"`、`"*"`），印出來用 |

| `Code` 的欄位 | 說明 |
|---|---|
| `address` | 哪個遙控器：0 ～ 255（NEC），或 16 位元（extended NEC） |
| `command` | 哪個鍵（0 ～ 255），意思要看遙控器 |
| `repeats` | 0 = 剛按下；1、2、3… = 按住時第幾次重複（最多 255） |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `ACTIVE_LOW` | `true` | 接收器收到光時 OUT 是 LOW（HX1838、VS1838B 都是；`module/HX1838.h` 已經設好） |
| `QUEUE` | 4 | 最多存幾個還沒讀的碼，每個 4 bytes；滿了就丟掉新的 |
| `REPEAT_TIMEOUT_MS` | 200 | 重複碼離上一個碼超過這麼久就不信（它重複的那一下沒收到） |
| `HOLD_TIMEOUT_MS` | 150 | 最後一個碼之後這麼久內，`isHeld()` 還是 true |

## NEC 協定

接收器只在收到 38 kHz 閃爍的紅外線時輸出 LOW（叫 mark），其他時候是 HIGH（叫 space）。38 kHz 是為了跟太陽、燈泡的紅外線分開；接收器已經把它濾掉，板子只看到長短不同的 LOW 和 HIGH。

| 送什麼 | 波形 | 多久 |
|---|---|---|
| 一次按下 | LOW 9 ms、HIGH 4.5 ms、32 個位元、LOW 0.56 ms | 67.5 ms |
| 一個位元 | LOW 0.56 ms，接著 HIGH 0.56 ms = 0、1.69 ms = 1 | 1.1 或 2.2 ms |
| 32 個位元 | 位址、位址取反、指令、指令取反（每個 8 位元，低位元先） | — |
| 按住 | 每 108 ms 一個重複碼：LOW 9 ms、HIGH 2.25 ms、LOW 0.56 ms（不帶指令） | 11.8 ms |

- **取反檢查**：指令後面跟著它的每個位元反過來的值，兩個對不上就丟掉（`badCount()` 加 1）。
- **extended NEC**：有些遙控器把「位址取反」拿來當位址的高 8 位元，所以位址是 16 位元。
- **解碼**：每次電位改變就進中斷，量剛結束的 LOW 或 HIGH 多長，允許約 ±30% 的誤差（9 ms 的 LOW 收 7 ～ 11 ms）。程式在忙（例如 OLED 的 `display()` 64 ms）也不會漏。
- **電腦上測過**：256 個指令、±15% 的時序誤差、按住、雜訊、斷掉的一半、檢查碼錯的、extended 位址都解對。

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

兩種是照網路上常見的碼寫的，**你的遙控器要先跑 `src/week_5/test_1.cpp` 確認**：按幾個鍵，看哪一欄的名稱跟你按的一樣，作業開頭的 `using Remote = …` 就用那一個。兩個都不對時，照 `test_1` 印出來的指令在 `module/` 加一個檔案。

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

## 資源

| 項目 | 用量 |
|---|---|
| RAM | 約 30 bytes（佇列 16、解碼狀態、時間）；`keyName` 用到時另外約 60 bytes |
| 中斷 | 腳位改變中斷（PCINT），每個碼約 68 次，每次約 10 µs（估計） |
| 腳位 | 一隻；同一組（D0 ～ D7、D8 ～ D13、A0 ～ A5）只能有一個 `UnoInterruptPin` |
| 時間 | `micros()`（Timer0），所以不能在中斷都關掉的程式裡用 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| `Uno_interrupt_pin.h` 被兩個 .cpp include | 中斷函式重複定義，連結錯誤 | 只在一個 .cpp 裡 include |
| 同一組腳位有兩個 `UnoInterruptPin` | 後 `init` 的蓋掉前一個 | 換到別組（D0 ～ D7、D8 ～ D13、A0 ～ A5） |
| 日光、日光燈、手機鏡頭照著接收器 | 偶爾收到雜訊，`badCount()` 增加 | 正常；雜訊不會變成按鍵 |
| 跟 DS18B20（`UnoOneWire`）一起用 | 每個位元關中斷約 70 µs，量到的長度差一點 | 誤差在容許範圍內，可以一起用 |
| 收不到 | — | 用手機相機看遙控器前端，按下時會看到紫光；沒有就換電池或抽掉絕緣片；`test_1` 有沒有印出東西 |
| 遙控器的鍵對不上 | 用錯 `Remote` | 跑 `test_1` 確認 |
