# switch

開關，`namespace switches`（`switch` 是 C++ 的關鍵字，不能當 namespace）。只依賴一個 Pin（例如 `Uno::UnoPin<PIN>`），換主板只要換 Pin。

## 檔案

```
lib/switch/
├── switch.h              入口：全部（程式只 include 它）
├── debounced_switch.h    DebouncedSwitch<Pin, ACTIVE_HIGH, DEBOUNCE_MS>：去彈跳、按下 / 放開、次數、時間（模組共用的核心）
└── module/               實際買到的零件
    ├── switch_2_pins.h   Switch2Pins<Pin>：兩隻腳的裸開關，接腳位和 GND（開內部上拉，按下是 LOW）
    └── switch_3_pins.h   Switch3Pins<Pin, ACTIVE_HIGH>：三隻腳的模組（GND、VCC、OUT）
```

## 兩隻腳和三隻腳

| 零件 | 例子 | 接法 | 用 |
|---|---|---|---|
| 兩隻腳的裸開關 | 按鈕、磁簧開關、傾斜開關（滾珠） | 一隻接腳位、一隻接 GND | `Switch2Pins<Uno::UnoPin<PIN, true>>`：內部上拉，沒按 HIGH、按下 LOW |
| 四隻腳的輕觸按鈕 | 麵包板常用的方形按鈕 | 四隻腳兩兩相通，**取對角的兩隻**當兩隻腳 | 同上 |
| 三隻腳的模組（GND、VCC、OUT） | 按鈕模組、TTP223 觸摸、傾斜、磁簧、霍爾模組 | OUT 接腳位，模組自己輸出高低 | `Switch3Pins<Uno::UnoPin<PIN>>`，不開上拉；按下是 LOW 的模組加 `false` |

- **兩隻腳一定要上拉**：開關打開時沒有東西推腳位，不開上拉會讀到亂跳的雜訊。
- **三隻腳的 ACTIVE_HIGH**：觸摸模組和大部分按鈕模組按下是 HIGH（預設）；有些按鈕、傾斜、霍爾模組按下是 LOW。不確定就跑 `src/week_4/test_2.cpp`。

## 接線

| 模組 | UNO |
|---|---|
| GND | GND |
| VCC | 5V |
| OUT | 任何數位腳，例如 D2 |

兩隻腳的裸開關：一邊接腳位、一邊接 GND，用 `Switch2Pins<Uno::UnoPin<2, true>>`。

## 用法

```cpp
#include "Uno_pin.h"
#include "switch.h"

switches::Switch3Pins<Uno::UnoPin<2>> button;     // 三隻腳的模組，OUT 接 D2
// switches::Switch2Pins<Uno::UnoPin<2, true>> button;   // 兩隻腳的裸按鈕，接 D2 和 GND

void setup() {
    button.init();
}

void loop() {
    button.update();                              // 每次 loop 都要呼叫，不會等
    if (button.wasPressed()) {
        // 剛按下（每按一次只會是 true 一次）
    }
}
```

完整的例子：`src/week_4/test_2.cpp`（把狀態用文字送到串口）、`src/week_4/assignment_1.cpp`（按一下換照片）。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Pin` | `init()`、`read()`（true = HIGH）、`nowMs()` | `Uno::UnoPin` |

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

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Pin` | — | 兩隻腳：`Uno::UnoPin<PIN, true>`（上拉）；三隻腳：`Uno::UnoPin<PIN>` |
| `ACTIVE_HIGH`（`Switch3Pins`） | `true` | 開的時候 OUT 是 HIGH。觸摸模組和大部分按鈕模組是 HIGH；有些模組是 LOW，要設成 `false`（`Switch2Pins` 固定是 LOW） |
| `DEBOUNCE_MS` | 20 | 去彈跳：新的電位要維持這麼久才算數 |

不知道 `ACTIVE_HIGH` 要設多少：跑 `src/week_4/test_2.cpp`，沒按的時候顯示 `on`，就改成 `false`。

## 去彈跳

機械接點在碰到、分開的那幾 ms 會來回跳好幾次。不處理的話，按一次會被算成好幾次。

| 情況 | 處理 |
|---|---|
| 電位一直在跳 | 每跳一次就重新計時，不改狀態 |
| 新的電位維持 `DEBOUNCE_MS` | 改狀態，`update()` 回傳 true，記下按下或放開 |
| 代價 | 每次變化都晚 `DEBOUNCE_MS`（20 ms）才看得到 |

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
