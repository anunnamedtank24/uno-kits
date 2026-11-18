# TCRT5000：介面

| 需要 | 得到 |
|---|---|
| 數位輸出（OUT）：一個 `Pin`：`init()`、`read()`、`nowMs()`（`Uno::UnoPin<PIN>`） | `TCRT5000::HW006`（`Reflector`）：反射或暗、剛改變、維持多久 |
| 類比輸出（四腳板的 A0）：`AnalogPin`（`Uno::UnoAnalogPin<A0>`） | `TCRT5000::AnalogReflector`：反射有多強 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 數位輸出 | `Pin`：模組上的可變電阻決定門檻；`ACTIVE_LOW` 要跟模組一致 |
| 類比輸出 | `AnalogPin`（四腳板的 A0） |
| 呼叫 | `update()` 每次 loop 都要呼叫 |

## 檔案

```
lib/TCRT5000/
├── TCRT5000.h        入口：全部（程式只 include 它）
├── reflector.h       Reflector<Pin, ACTIVE_LOW, FILTER_MS>：反射 / 暗、濾掉抖動、變化次數、時間（模組共用的核心）
└── module/           實際買到的零件
    ├── HW_006.h      HW006<Pin>：VCC、GND、OUT 的循跡模組
    └── TCRT5000_4_pins.h  AnalogReflector<AnalogPin>：四腳板（VCC、GND、D0、A0）的 A0：反射有多強
```

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Pin` | `init()`、`read()`（true = HIGH）、`nowMs()` | `Uno::UnoPin` |

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 設定腳位，讀一次目前的狀態 |
| `update()` | 讀腳位、濾掉抖動；狀態剛改變時回傳 true |
| `isReflecting()` | 前面有白色 / 近的東西 |
| `isDark()` | 相反：黑色、遠、沒東西（在黑線上） |
| `readLevel()` | 腳位現在的電位（沒濾），true = HIGH，用來確認 `ACTIVE_LOW` |
| `becameReflecting()`、`becameDark()` | 剛變成反射 / 暗時回傳 true 一次（呼叫後清掉） |
| `changeCount()` | `init()` 之後變了幾次 |
| `stateMs()` | 目前的狀態維持多久了 |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Pin` | — | `Uno::UnoPin<PIN>`（不開上拉） |
| `ACTIVE_LOW`（`Reflector`） | `true` | 反射時 OUT 是 LOW（HW-006 和大部分板子）；相反的板子用 `Reflector<Pin, false>`（`HW006` 固定是 `true`） |
| `FILTER_MS` | 2 | 新的電位要維持這麼久才算數。在可變電阻的臨界點附近，比較器會快速來回跳；0 = 不濾，變了馬上算 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `Pin` 的腳位 | 0 ～ 19；D0 / D1 編譯警告（USB 串口） | 編譯錯誤 |
| `FILTER_MS` | 0 ～ 255 ms | — |
| `update()` 的間隔 | 越常越好；循跡的車子越快越要常呼叫 | 比 `FILTER_MS` 久時，很快掠過的黑線可能會漏掉 |
| `changeCount()` | 0 ～ 65535 | 停在 65535 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | 約 16 bytes |
| 硬體 | 一隻腳；不用中斷 |
