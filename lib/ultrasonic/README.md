# ultrasonic

超音波測距模組，`namespace ultrasonic`。依賴一個 Trigger（輸出腳，例如 `Uno::UnoOutputPin<PIN>`）和一個 Echo（有中斷的輸入腳，例如 `Uno::UnoInterruptPin<PIN>`），換主板只要換這兩個。

## 名稱

| 名稱 | 是什麼 |
|---|---|
| **HC-SR04** | 模組的型號（零件），放在 `module/HC_SR04.h` |
| 晶片 | 沒有固定的：不同廠商的板子用不同的晶片（EM78P153、RCWL-9206、CS100A…），但腳位和用法都一樣 |
| **Trig / Echo** | 用法（通訊方式）：Trig 給一個脈衝開始量，Echo 的 HIGH 有多長就是聲音來回的時間 |

照「`lib/` 用晶片或通訊方式命名」的規則，這裡沒有共同的晶片，Trig / Echo 也不是有名字的協定，所以跟 `switch`、`buzzer`、`LED` 一樣用零件種類命名：`ultrasonic/`。以後其他 Trig / Echo 的模組（JSN-SR04T 防水探頭、US-100）放 `module/`。

## 檔案

```
lib/ultrasonic/
├── ultrasonic.h      入口：全部（程式只 include 它）
├── ranger.h          Ranger<Trigger, Echo, MIN_MM, MAX_MM>：送 Trig、用中斷量 Echo、換成 mm、溫度補償（模組共用的核心）
└── module/           實際買到的零件
    └── HC_SR04.h     HCSR04<Trigger, Echo>：VCC、Trig、Echo、GND；2 ～ 400 cm
```

## 原理

```
Trig  ──┐▔▔┐────────────────────────────────────    ≥ 10 µs 的 HIGH：開始量一次
Sound      ))))))))                    ((((((((      送 8 個 40 kHz 的超音波（約 0.2 ms），碰到東西反彈回來
Echo  ───────────┐▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┐──────────    送完變 HIGH，聽到回音變 LOW
                 └──── 來回的時間 ─────┘
```

- **距離** = Echo 的 HIGH 時間 × 音速 ÷ 2（來回）。20 °C 時音速 343 m/s，**每 58 µs 是 1 cm**。
- **沒有回音**：Echo 約 38 ms 後自己變回 LOW（有些仿製品要更久，見 `RESULT_BUSY`）。
- **兩次之間至少隔 60 ms**：上一次的超音波可能還在房間裡反彈，太快再量會把舊的回音當成新的。

## 接線

| HC-SR04 | UNO | 說明 |
|---|---|---|
| VCC | 5V | HC-SR04 要 5 V（HC-SR04P 是 3 ～ 5.5 V） |
| Trig | 任何數位腳，例如 **D7** | `Uno::UnoOutputPin<7>` |
| Echo | 任何數位腳，例如 **D8** | `Uno::UnoInterruptPin<8>`；輸出 5 V，UNO 可以直接接 |
| GND | GND | |

- **Echo 用中斷量**：腳位改變中斷一組只能有一個（D0 ～ D7、D8 ～ D13、A0 ～ A5），紅外線接收器在 D2，所以 Echo 放 D8，兩個可以同時用。
- 前面要空：探頭前方約 15° 的圓錐裡最近的東西就是量到的距離。

## 用法

```cpp
#include "Uno_pin.h"
#include "Uno_interrupt_pin.h"
#include "ultrasonic.h"

using Ranger = ultrasonic::HCSR04<Uno::UnoOutputPin<7>, Uno::UnoInterruptPin<8>>;   // Trig D7、Echo D8

void setup() {
    Ranger::init();
    Ranger::setTemperature(250);      // 25.0 °C（可省略，預設 20.0 °C）
}

void loop() {
    if (Ranger::update()) {           // 每次 loop 都要呼叫，不會等；量完一次回傳 true
        if (Ranger::result() == ultrasonic::RESULT_OK) {
            uint16_t mm = Ranger::distanceMm();
        }
    }
}
```

全部是 static（中斷沒有物件可以呼叫），所以用 `Ranger::` 不用宣告變數；一個 Echo 腳位一個 Ranger。完整的例子：`src/week_6/test_1.cpp`。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Trigger` | `init()`、`write(high)` | `Uno::UnoOutputPin` |
| `Echo` | `init(onChange)`、`read()`、`nowUs()`、`nowMs()`、`stopInterrupts()`、`restoreInterrupts(saved)` | `Uno::UnoInterruptPin` |

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 設定兩隻腳和中斷；下一次 `update()` 開始第一次測量 |
| `update()` | 時間到就送 Trig、檢查 Echo；量完一次回傳 true（不會等） |
| `measure()` | 量一次並等它結束（最多約 40 ms，還要加上離上一次不到間隔的時間），回傳結果 |
| `result()` | 上一次的結果（見下表） |
| `distanceMm()` | 上一次的距離，mm（`RESULT_OK` 時才對，其他是 0） |
| `echoUs()` | 上一次 Echo 的 HIGH 有幾 µs（沒結束的是 0） |
| `setIntervalMs(ms)`、`intervalMs()` | 兩次測量的間隔，預設和最小都是 60 ms |
| `setTemperature(tenthsC)` | 氣溫，0.1 °C（250 = 25.0 °C）：音速 = 331.3 + 0.606 × °C m/s |
| `soundSpeed()` | 現在用的音速，0.1 m/s（20 °C 是 3434） |
| `toMm(us)` | Echo 的 µs 換成 mm（用現在的溫度） |
| `count()` | `init()` 之後開始量了幾次 |
| `onChange()` | Echo 的中斷函式；公開是為了讓測試可以包一層 |

## 結果

| 結果 | 意思 | 常見原因 |
|---|---|---|
| `RESULT_OK` | 量到了，`distanceMm()` 就是距離 | |
| `RESULT_TOO_NEAR` | 比 `MIN_MM`（2 cm）近 | 回音在模組還在送的時候就回來了 |
| `RESULT_TOO_FAR` | 超過 `MAX_MM`（400 cm）還沒回音 | 前面沒東西；布、海綿吸音；表面斜的把聲音反射到別處 |
| `RESULT_NO_ECHO` | Echo 一直沒變 HIGH | 沒電、Trig 和 Echo 接反、沒接好 |
| `RESULT_BUSY` | 要量的時候 Echo 還是 HIGH | 有些仿製品沒回音時 Echo 會 HIGH 很久；這次不量，下個間隔再試 |
| `RESULT_NONE` | 還沒量過 | |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Trigger` | — | `Uno::UnoOutputPin<PIN>` |
| `Echo` | — | `Uno::UnoInterruptPin<PIN>`（不開上拉，模組自己輸出） |
| `MIN_MM`、`MAX_MM` | HC-SR04：20、4000 | 量得到的範圍；`MAX_MM` 也決定等 Echo 多久（`MAX_MM` 的來回時間多 10 %） |

## 準確度

| 誤差來源 | 大小 | 怎麼辦 |
|---|---|---|
| `nowUs()` 每 4 µs 一格 | 約 0.7 mm | — |
| 中斷的延遲（別的中斷正在跑，例如 `millis`） | 幾 µs，約 1 mm | — |
| 氣溫 | 每差 10 °C 約 1.7 %（1 m 差 1.7 cm） | `setTemperature`，可以用 DS18B20 量 |
| 模組本身 | 約 3 mm；表面越小、越斜越不準 | 量幾次取中位數 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| Trigger、Echo 的腳位 | 0 ～ 19；D0 / D1 編譯警告（USB 串口） | 編譯錯誤 |
| `MIN_MM < MAX_MM`、`MAX_MM` ≤ 10000 | | 編譯錯誤 |
| `setIntervalMs` | 60 ～ 65535 ms | 小於 60 的當成 60 |
| `setTemperature` | -400 ～ 800（-40 ～ 80 °C） | 夾到範圍內 |
| 同一組（D0 ～ D7、D8 ～ D13、A0 ～ A5）有別的 `UnoInterruptPin` | | 後 `init` 的蓋掉前一個 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | 約 30 bytes |
| 硬體 | 兩隻腳；Echo 用腳位改變中斷（每次變化約 5 µs） |
| 時間 | `update()` 送 Trig 時等 12 ～ 16 µs，其他時候不會等 |
