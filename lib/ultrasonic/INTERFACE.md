# ultrasonic：介面

| 需要 | 得到 |
|---|---|
| `Trigger`：輸出腳（`Uno::UnoOutputPin<PIN>`）；`Echo`：有中斷的輸入腳（`Uno::UnoInterruptPin<PIN>`） | `ultrasonic::HCSR04`：距離（mm），量的時候不會等，結果碼，溫度修正；`ultrasonic::Median`：比較穩的距離 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 腳位 | Trig：輸出 `Pin`；Echo：`InterruptPin`（腳位改變中斷，一組腳只能一個） |
| static | 全部是 static（中斷沒有物件可以呼叫）：一個 Echo 腳一個 `Ranger` 型別 |
| 呼叫 | `update()` 每次 loop 都要呼叫；量測間隔至少 60 ms |
| 溫度 | 音速跟溫度有關：`setTemperature()` 給房間的溫度 |

## 檔案

```
lib/ultrasonic/
├── ultrasonic.h      入口：全部（程式只 include 它）
├── ranger.h          Ranger<Trigger, Echo, MIN_MM, MAX_MM>：送 Trig、用中斷量 Echo、換成 mm、溫度補償（模組共用的核心）
├── module/           實際買到的零件
│   └── HC_SR04.h     HCSR04<Trigger, Echo>：VCC、Trig、Echo、GND；2 ～ 400 cm
└── tool/             建在它上面的常用功能
    └── median.h      Median<Ranger, N>：最近 N 次的中位數（比較穩）
```

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
| `result()` | 上一次的結果（`RESULT_OK`、`RESULT_TOO_NEAR`…，見 [README 的結果](README.md#結果)） |
| `distanceMm()` | 上一次的距離，mm（`RESULT_OK` 時才對，其他是 0） |
| `echoUs()` | 上一次 Echo 的 HIGH 有幾 µs（沒結束的是 0） |
| `setIntervalMs(ms)`、`intervalMs()` | 兩次測量的間隔，預設和最小都是 60 ms |
| `setTemperature(tenthsC)` | 氣溫，0.1 °C（250 = 25.0 °C）：音速 = 331.3 + 0.606 × °C m/s |
| `soundSpeed()` | 現在用的音速，0.1 m/s（20 °C 是 3434） |
| `toMm(us)` | Echo 的 µs 換成 mm（用現在的溫度） |
| `count()` | `init()` 之後開始量了幾次 |
| `onChange()` | Echo 的中斷函式；公開是為了讓測試可以包一層 |

`Median<Ranger, N>`（`tool/median.h`）：

| 函式 | 說明 |
|---|---|
| `init()` | 也 init 感測器 |
| `update()` | 呼叫感測器的 `update()`；有新的量測（好的或不好的）時回傳 true |
| `distanceMm()` | 回傳最近 N 次好的量測的中位數（第一次之前是 0） |
| `goodCount()` | 回傳最近 N 次裡有幾次是 `RESULT_OK` |
| `isValid()` | 至少一半是好的時回傳 true |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Trigger` | — | `Uno::UnoOutputPin<PIN>` |
| `Echo` | — | `Uno::UnoInterruptPin<PIN>`（不開上拉，模組自己輸出） |
| `MIN_MM`、`MAX_MM` | HC-SR04：20、4000 | 量得到的範圍；`MAX_MM` 也決定等 Echo 多久（`MAX_MM` 的來回時間多 10 %） |
| `N`（`Median`） | 5 | 取幾次的中位數：3 ～ 15 的奇數；越大越穩，但慢 N / 2 次量測 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| Trigger、Echo 的腳位 | 0 ～ 19；D0 / D1 編譯警告（USB 串口） | 編譯錯誤 |
| `MIN_MM < MAX_MM`、`MAX_MM` ≤ 10000 | | 編譯錯誤 |
| `setIntervalMs` | 60 ～ 65535 ms | 小於 60 的當成 60 |
| `setTemperature` | -400 ～ 800（-40 ～ 80 °C） | 夾到範圍內 |
| `Median` 的 `N` | 3 ～ 15 的奇數 | 編譯錯誤 |
| 同一組（D0 ～ D7、D8 ～ D13、A0 ～ A5）有別的 `UnoInterruptPin` | | 後 `init` 的蓋掉前一個 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | 約 30 bytes（static）；`Median<…, N>` 另外 3 × N + 5 bytes |
| 硬體 | 兩隻腳；Echo 用腳位改變中斷（每次變化約 5 µs） |
| 時間 | `update()` 送 Trig 時等 12 ～ 16 µs，其他時候不會等 |
