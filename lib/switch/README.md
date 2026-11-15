# switch

開關，`namespace switches`（`switch` 是 C++ 的關鍵字，不能當 namespace）。只依賴一個 Pin，例如 `Uno::UnoPin<PIN>`，所以換主板只要換 Pin。

能用的開關：
- 三支腳的模組（GND、VCC、OUT）：按鈕、觸摸、傾斜、磁簧等。
- 直接接到 GND 的裸按鈕：開 Pin 的內部上拉。

## 接線

| 模組 | UNO |
|---|---|
| GND | GND |
| VCC | 5V |
| OUT | 任何數位腳，例如 D2 |

## 用法

```cpp
#include "Uno_pin.h"
#include "switch.h"

using SwitchPin = Uno::UnoPin<2>;                 // OUT 接 D2
switches::Switch<SwitchPin> button;               // OUT 在開的時候是 HIGH（預設）

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

完整的例子：`src/week_4/test_2.cpp`，把狀態用文字送到串口。

## 參數

| template 參數 | 預設 | 說明 |
|---|---|---|
| `Pin` | — | `Uno::UnoPin<PIN, INTERNAL_PULLUP>` |
| `ACTIVE_HIGH` | `true` | 開的時候 OUT 是 HIGH。觸摸模組和大部分按鈕模組是 HIGH；裸按鈕接 GND、有些模組是 LOW，要設成 `false` |
| `DEBOUNCE_MS` | 20 | 去彈跳：新的電位要維持這麼久才算數 |

不知道 `ACTIVE_HIGH` 要設多少的話，就跑 `src/week_4/test_2.cpp`：沒按的時候顯示 `on`，就改成 `false`。

## 函式

| 函式 | 作用 |
|---|---|
| `init()` | 設定腳位，讀一次目前的狀態（開機時已經按著不算一次按下） |
| `update()` | 每次 loop 呼叫；去彈跳後狀態剛改變時回傳 true |
| `isOn()` | 去彈跳後的狀態 |
| `readLevel()` | 腳位現在的電位（沒去彈跳），true = HIGH，用來找 `ACTIVE_HIGH` |
| `wasPressed()`、`wasReleased()` | 剛變成開 / 關時，回傳 true 一次（呼叫後清掉） |
| `pressCount()` | `init()` 之後按了幾次 |
| `stateMs()` | 目前的狀態維持多久了 |
| `lastStateMs()` | 上一個狀態維持多久，例如放開後就是剛才按了多久 |

## 去彈跳

機械接點在碰到、分開的那幾 ms 會來回跳好幾次。如果不處理，按一次會被算成好幾次。

| 情況 | 處理 |
|---|---|
| 電位一直在跳 | 每跳一次就重新計時，不改狀態 |
| 新的電位維持 `DEBOUNCE_MS` | 改狀態，`update()` 回傳 true，記下按下或放開 |
| 代價 | 每次變化都晚 `DEBOUNCE_MS`（20 ms）才看得到 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `UnoPin` 的 `PIN` | 0 ～ 19；D0 / D1 警告（USB 串口） | 編譯錯誤 |
| `DEBOUNCE_MS` | 0 ～ 255 ms；0 = 不去彈跳 | — |
| `update()` 的間隔 | 越常越好；比 `DEBOUNCE_MS` 久的話，很短的按壓可能會漏掉 | — |
| `pressCount()` | 0 ～ 65535，之後從 0 重來 | — |
| `stateMs()` | 約 49 天後 `millis()` 會繞回 0 | 差值還是對的（無號數相減） |
