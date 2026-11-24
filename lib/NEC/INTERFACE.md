# NEC：介面

| 需要 | 得到 |
|---|---|
| 接收：`InterruptPin`：`init(onChange)`、`read()`、`nowUs()`…（`Uno::UnoInterruptPin<PIN>`） | `NEC::HX1838`：收到的碼（哪個遙控器、哪個鍵、是不是按住），不會漏 |
| 遙控器的指令表 | `NEC::Remote17KeyNumbersTop`、`Remote17KeyArrowsTop`：碼換成鍵（`KEY_1`、`KEY_UP`…） |
| 發射：`Carrier`：38 kHz 開、關（`Uno::UnoIrCarrier<3>`） | `NEC::IRLED`：用紅外線 LED 當遙控器 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [讀我.md](讀我.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 接收 | `InterruptPin`：腳位改變中斷，一組腳（D0 ～ D7、D8 ～ D13、A0 ～ A5）只能一個；碼放進佇列，程式忙也不漏 |
| 發射 | `Carrier`：`Uno::UnoIrCarrier` 用 Timer2（只能 D3 / D11），不能同時用 `UnoPcmOut`；`send()` 會等 67.5 ms |
| 遙控器 | `Remote`：`ADDRESS`、`toKey()`、`toCommand()`；按鍵碼要先用測試程式確認 |

## 檔案

```
lib/NEC/
├── NEC.h                   入口：全部
├── protocol.h              NEC 的時間（收、發共用）
├── receiver.h              Receiver<InterruptPin, ACTIVE_LOW, QUEUE>：在中斷裡解 NEC，Code = 位址 + 指令 + 第幾次重複
├── transmitter.h           Transmitter<Carrier>：用紅外線 LED 送 NEC（當遙控器）
├── key.h                   Key：KEY_0 ～ KEY_9、方向鍵、OK、*、#（每個遙控器都一樣）；keyDigit、keyName
└── module/
    ├── HX1838.h            HX1838<InterruptPin>、VS1838B<InterruptPin>：接收器（KY-022 也是）
    ├── IR_LED.h            IRLED<Carrier>：裸的紅外線 LED（+ 電阻），發射
    ├── KY_005.h            KY005<Carrier>：紅外線 LED 的小板子，發射
    └── remote_17_key.h     Remote17KeyNumbersTop、Remote17KeyArrowsTop：Arduino 套件的 17 鍵遙控器，指令 ↔ Key
```

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `InterruptPin` | `init(onChange)`（輸入腳，每次電位改變就在中斷裡呼叫 `onChange()`）、`read()`（true = HIGH）、`nowUs()`（中斷裡也能用）、`nowMs()`、`stopInterrupts()`、`restoreInterrupts(saved)` | `Uno::UnoInterruptPin` |
| `Carrier`（`Transmitter`） | `init()`、`on()`、`off()`、`delayUs()`、`nowMs()` | `Uno::UnoIrCarrier` |
| `Remote` | `static constexpr uint16_t ADDRESS`、`Key toKey(const Code &code)`（別的位址或不認識的指令回傳 `KEY_NONE`）、`bool toCommand(Key key, uint8_t &command)`（發射用；沒有這個鍵回傳 false） | `NEC::Remote17KeyNumbersTop`、`Remote17KeyArrowsTop` |

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
| `Receiver::changeCount()` | 回傳 `init()` 之後腳位變了幾次（停在 65535）：按著鍵還是 0 = 接線問題，有變但沒有碼 = 協定問題 |
| `Transmitter::send(address, command)`、`send(code)` | 送一次按下（會等 67.5 ms）；位址 > 255 用 extended NEC |
| `Transmitter::sendKey<Remote>(key)` | 送遙控器的某個鍵；沒有這個鍵回傳 false |
| `Transmitter::sendRepeat()` | 送一個重複碼 |
| `Transmitter::hold(address, command, ms)`、`holdKey<Remote>(key, ms)` | 按住 ms：按下 + 每 108 ms 一個重複碼 |
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

## 發射和接收為什麼放一起、又分成兩個類別

| 項目 | 一樣嗎 | 所以 |
|---|---|---|
| 協定（時間、32 位元、重複碼） | 一樣 | 放同一個資料夾 `NEC/`，共用 `protocol.h`、`Code`、`Key` |
| 遙控器的指令表 | 一樣 | `Remote17Key…` 兩個方向都有：`toKey`（收）、`toCommand`（送） |
| 硬體 | 不一樣：接收是一顆會自己解調的晶片，接腳位中斷；發射是一顆 LED，要計時器產生 38 kHz | 分成 `Receiver<InterruptPin>`、`Transmitter<Carrier>` 兩個類別，用的 Uno 介面不同 |
| 時間 | 不一樣：接收在中斷裡、不會等；發射要精準控制 LED 的亮滅，會等 | 合成一個類別也只是兩組不相干的函式 |

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
