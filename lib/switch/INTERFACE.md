# switch：介面

| 需要 | 得到 |
|---|---|
| 一個 `Pin`：`init()`、`read()`、`nowMs()`（`Uno::UnoInputPin<PIN>`） | `switches::Switch2Pins`、`Switch3Pins`：去彈跳的開關（開著沒、剛按下、按了幾次、多久）；`switches::Clicks`：單擊、雙擊、長按、按住連發 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [讀我.md](讀我.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 腳位 | `Pin`：`init()`、`read()`、`nowMs()`；兩隻腳的開關用 UNO 的上拉（`UnoInputPin<PIN, true>`） |
| 呼叫 | `update()` 每次 loop 都要呼叫，去彈跳和 `Clicks` 的計時靠它 |
| 電位 | `ACTIVE_HIGH` 要跟模組一致（按下時 OUT 是 HIGH 還是 LOW） |

## 檔案

```
lib/switch/
├── switch.h              入口：全部（程式只 include 它）
├── debounced_switch.h    DebouncedSwitch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>：去彈跳、按下 / 放開、次數、時間（模組共用的核心）
├── module/               實際買到的零件
│   ├── switch_2_pins.h   Switch2Pins<Pin>：兩隻腳的裸開關，接腳位和 GND（開內部上拉，按下是 LOW）
│   └── switch_3_pins.h   Switch3Pins<Pin, ACTIVE_HIGH>：三隻腳的模組（GND、VCC、OUT）
└── tool/                 建在它上面的常用功能
    └── clicks.h          Clicks<Switch>：單擊、雙擊、多擊、長按、按住重複
```

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Pin` | `init()`、`read()`（true = HIGH）、`nowMs()` | `Uno::UnoInputPin` |

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 設定腳位，讀一次目前的狀態（開機時已經按著不算一次按下） |
| `update()` | 讀腳位、去彈跳；狀態剛改變時回傳 true |
| `isOn()` | 回傳去彈跳後的狀態 |
| `readLevel()` | 回傳腳位現在的電位（沒去彈跳），true = HIGH，用來找 `ACTIVE_HIGH` |
| `wasPressed()`、`wasReleased()` | 剛變成開 / 關時回傳 true 一次（呼叫後清掉） |
| `pressCount()` | 回傳 `init()` 之後按了幾次 |
| `stateMs()` | 回傳目前的狀態維持多久了 |
| `lastStateMs()` | 回傳上一個狀態維持多久，例如放開後就是剛才按了多久 |

`Clicks<Switch>`（`tool/clicks.h`）：Switch 是這個函式庫的開關（`DebouncedSwitch`、`Switch2Pins`、`Switch3Pins`），程式只呼叫 `Clicks::update()`：

| 函式 | 說明 |
|---|---|
| `Clicks(button)` | 建構：要看的開關 |
| `init()` | 也 init 開關 |
| `update()` | 呼叫開關的 `update()`；回傳剛發生的事件（`CLICK`、`DOUBLE_CLICK`、`MULTI_CLICK`、`LONG_PRESS`、`REPEAT`、`LONG_RELEASE`），沒有就是 `NOTHING` |
| `count()` | 回傳上一次單擊 / 雙擊 / 多擊按了幾下 |
| `isPressed()` | 現在有沒有按著 |
| `setDoubleMs(ms)` | 雙擊兩下之間最多隔多久（預設 300 ms；0 = 不分辨雙擊，放開就報 `CLICK`） |
| `setLongMs(ms)`、`setRepeatMs(ms)` | 長按的時間（預設 800 ms）、之後 `REPEAT` 的間隔（預設 150 ms；0 = 不重複） |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Pin` | — | 兩隻腳：`Uno::UnoInputPin<PIN, true>`（上拉）；三隻腳：`Uno::UnoInputPin<PIN>` |
| `ACTIVE_HIGH`（`Switch3Pins`） | `true` | 開的時候 OUT 是 HIGH。觸摸模組和大部分按鈕模組是 HIGH；有些模組是 LOW，要設成 `false`（`Switch2Pins` 固定是 LOW） |
| `DEBOUNCE_MS` | 20 | 去彈跳：新的電位要維持這麼久才算數 |

不知道 `ACTIVE_HIGH` 要設多少：跑 `src/week_4_SSD1306_switch_buzzer_audio/tests/2_switch.cpp`，沒按的時候顯示 `on`，就改成 `false`。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `Pin` 的腳位 | 0 ～ 19；D0 / D1 編譯警告（USB 串口） | 編譯錯誤 |
| `DEBOUNCE_MS` | 0 ～ 255 ms；0 = 不去彈跳 | — |
| `update()` 的間隔 | 越常越好 | 比 `DEBOUNCE_MS` 久時，很短的按壓可能會漏掉 |
| `pressCount()` | 0 ～ 65535 | 之後從 0 重來 |
| `stateMs()` | 約 49 天後 `millis()` 繞回 0 | 差值還是對的（無號數相減） |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | 約 16 bytes（狀態、時間、次數） |
| 硬體 | 一隻腳；不用中斷 |
