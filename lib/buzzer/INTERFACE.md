# buzzer：介面

| 需要 | 得到 |
|---|---|
| 有源：一個 `Pin`：`init()`、`write(high)`…（`Uno::UnoOutputPin<PIN>`） | `buzzer::ActiveBuzzer2Pins`、`ActiveBuzzer3Pins`：開、關（音高固定） |
| 無源：一個 `Tone`：`init()`、`start(hz)`、`stop()`…（`Uno::UnoTone<PIN>`） | `buzzer::PassiveBuzzer2Pins`、`PassiveBuzzer3Pins`：任意頻率、音量 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 有源 | 一個輸出 `Pin`；只有開、關，沒有音高 |
| 無源 | `Tone`：`Uno::UnoTone<PIN>` 用 Timer1，D9 / D10 不能同時拿來 PWM；音量靠脈衝寬度 |
| 三隻腳的模組 | `IDLE_LEVEL_HIGH` 要跟模組一致（安靜時腳位的電位），否則安靜時也會有電流 |

## 檔案

```
lib/buzzer/
├── buzzer.h                     入口：全部（程式只 include 它）
├── active_buzzer.h              ActiveBuzzer<Pin, ACTIVE_HIGH>：有源，只能開關（模組共用的核心）
├── passive_buzzer.h             PassiveBuzzer<Tone>：無源，任何音高（模組共用的核心）
└── module/                      實際買到的零件
    ├── active_buzzer_2_pins.h   ActiveBuzzer2Pins<Pin>：有源，兩隻腳（+、-）
    ├── active_buzzer_3_pins.h   ActiveBuzzer3Pins<Pin, LOW_TRIGGER>：有源模組，VCC、I/O、GND
    ├── passive_buzzer_2_pins.h  PassiveBuzzer2Pins<Tone>：無源，兩隻腳（+、-）
    └── passive_buzzer_3_pins.h  PassiveBuzzer3Pins<Tone>：無源模組，VCC、I/O、GND
```

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Pin`（有源） | `init()`、`write(high)`、`nowMs()`、`readFlash8(address)`、`readFlash16(address)` | `Uno::UnoOutputPin` |
| `Tone`（無源） | `init()`、`start(hz)`（播放中呼叫就換頻率）、`stop()`（腳位回到安靜的電位）、`realHz()`、`nowMs()`、`readFlash8(address)`、`readFlash16(address)`；常數 `IDLE_LEVEL_HIGH`（安靜時是不是 HIGH，模組用它檢查） | `Uno::UnoTone<PIN, IDLE_HIGH>` |

每個模組本身都是 `audio::Player`、`StreamPlayer` 要的 Sound（`HAS_PITCH`、`init`、`start`、`stop`、`nowMs`、`readFlash8/16`）：有源的 `HAS_PITCH` 是 false，給它音符會編譯警告（只聽得到節奏）。播取樣（PCM）不經過這裡，用 `Uno::UnoPcmOut`，見 [audio](../audio/README.md#接線)。

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 設定腳位，安靜 |
| `start(hz)` | 有源：開（hz 只要 > 0）；無源：送 hz 的方波，或在播放中換頻率 |
| `stop()` | 安靜：有源關掉；無源的腳位回到 Tone 安靜的電位（兩隻腳 LOW、三隻腳模組 HIGH） |
| `nowMs()`、`readFlash16()` | 轉給 Pin / Tone（給 `audio::Player`） |
| `ActiveBuzzer::HZ` | 2300：有源的音高不知道，用常見的值（給 `beep(HZ, ms)`、`currentHz()`） |
| `PassiveBuzzer::realHz()` | 回傳實際播出的頻率（計時器只能整數 tick），安靜時是 0 |

平常不直接呼叫這些，而是透過 `audio::Player` 的 `tone`、`beep`、`beeps`、`play`、`slide`（見 [audio](../audio/INTERFACE.md#函式)）。

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Pin`（有源） | — | `Uno::UnoOutputPin<PIN>` |
| `LOW_TRIGGER`（`ActiveBuzzer3Pins`） | `true` | 三隻腳模組 I/O 是 LOW 時響（PNP，大部分）；HIGH 才響的（NPN）改成 `false` |
| `ACTIVE_HIGH`（核心 `ActiveBuzzer`） | `true` | 自己接線、不照模組時用：+ 接 5V、- 接腳位時是 `false` |
| `Tone`（無源） | — | 兩隻腳：`Uno::UnoTone<PIN>`；三隻腳模組：`Uno::UnoTone<PIN, true>`。D9 / D10 不佔 CPU，其他腳用中斷 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 腳位 | 0 ～ 19；D0 / D1 編譯警告 | 編譯錯誤 |
| 無源的 hz | 16 ～ 20000（`UnoTone`）；好聽的範圍約 200 ～ 5000，最大聲在共振頻率（常見 2 ～ 4 kHz） | 夾到範圍內 |
| 有源的 hz | 任何 > 0 都一樣 | — |
| 音量 | 無源 0 ～ 255、有源只能靜音（見[音量](README.md#音量)） | 255 以上不可能（已經是 50%） |
| 同時發幾個音 | 一個 Sound 一個音 | — |

## 資源

| 項目 | 有源 | 無源 |
|---|---|---|
| RAM | 0（模組只有 static 函式；`audio::Player` 約 28 bytes） | 同左 |
| 腳位 | 一隻 | 一隻 |
| 計時器 | 不用 | **Timer1**（整個） |
| 中斷 | 不用 | D9 / D10：不用；其他腳：Timer1 比較中斷，每次翻轉約 2 µs（20 kHz 約佔 8% CPU） |
| 同時用幾個 | 幾個都可以（各自一隻腳、一個 `Player`） | **一個**（只有一個 Timer1） |
