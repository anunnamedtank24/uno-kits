# switch

| 需要 | 得到 |
|---|---|
| 一個 `Pin`：`init()`、`read()`、`nowMs()`（`Uno::UnoPin<PIN>`） | `switches::Switch2Pins`、`Switch3Pins`：去彈跳的開關（開著沒、剛按下、按了幾次、多久）；`switches::Clicks`：單擊、雙擊、長按、按住連發 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

開關，`namespace switches`（`switch` 是 C++ 的關鍵字，不能當 namespace）。

## 用法

```cpp
#include "Uno_pin.h"
#include "switch.h"

constexpr uint8_t SWITCH_PIN = 2;                 // 模組的 OUT
switches::Switch3Pins<Uno::UnoPin<SWITCH_PIN>> button;
// switches::Switch2Pins<Uno::UnoPin<SWITCH_PIN, true>> button;   // 兩隻腳的裸按鈕：接 SWITCH_PIN 和 GND

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

完整的例子：`src/week_4_SSD1306_switch_buzzer_audio/tests/2_switch.cpp`（把狀態用文字送到串口）、`src/week_4_SSD1306_switch_buzzer_audio/assignments/1_photo_switch.cpp`（按一下換照片）。

## Clicks：一個按鈕當好幾個按鈕用

```cpp
constexpr uint8_t BUTTON_PIN = 2;
switches::Switch2Pins<Uno::UnoInputPin<BUTTON_PIN, true>> button;
switches::Clicks<decltype(button)> clicks(button);

void setup() { clicks.init(); }            // 也會 init 按鈕
void loop() {
    switch (clicks.update()) {             // 只要呼叫它（它會呼叫按鈕的 update）
        case switches::CLICK:        /* 按一下 */ break;
        case switches::DOUBLE_CLICK: /* 按兩下 */ break;
        case switches::LONG_PRESS:   /* 按住 0.8 秒 */ break;
        default: break;
    }
}
```

| 事件 | 什麼時候 |
|---|---|
| `CLICK` | 按一下放開，而且 `doubleMs`（300 ms）內沒有再按 |
| `DOUBLE_CLICK` | 快速按兩下 |
| `MULTI_CLICK` | 快速按 3 下以上，`count()` 是幾下 |
| `LONG_PRESS` | 按住 `longMs`（800 ms），還沒放開就會報 |
| `REPEAT` | 長按之後繼續按住，每 `repeatMs`（150 ms）一次（像按住鍵盤） |
| `LONG_RELEASE` | 長按之後放開 |

| 函式 | 說明 |
|---|---|
| `init()` | 也 init 按鈕 |
| `update()` | 回傳剛發生的事件，沒有就是 `NOTHING` |
| `count()` | 上一次單擊 / 雙擊 / 多擊按了幾下 |
| `isPressed()` | 現在有沒有按著 |
| `setDoubleMs(ms)` | 兩下之間最多隔多久（預設 300；0 = 不分辨雙擊，每次放開馬上報 `CLICK`） |
| `setLongMs(ms)`、`setRepeatMs(ms)` | 長按的時間（預設 800）、重複的間隔（預設 150；0 = 不重複） |

- **單擊會晚 300 ms**：要等著看會不會有第二下。不需要雙擊時 `setDoubleMs(0)`，就馬上報。
- 完整的例子：`src/week_4_SSD1306_switch_buzzer_audio/tests/6_clicks.cpp`（單擊、雙擊、長按換 LED 效果）。

## 接線

| 模組 | UNO |
|---|---|
| GND | GND |
| VCC | 5V |
| OUT | 任何數位腳，例如 D2 |

兩隻腳的裸開關：一邊接腳位、一邊接 GND，用 `Switch2Pins<Uno::UnoPin<PIN, true>>`。

## 兩隻腳和三隻腳

| 零件 | 例子 | 接法 | 用 |
|---|---|---|---|
| 兩隻腳的裸開關 | 按鈕、磁簧開關、傾斜開關（滾珠） | 一隻接腳位、一隻接 GND | `Switch2Pins<Uno::UnoPin<PIN, true>>`：內部上拉，沒按 HIGH、按下 LOW |
| 四隻腳的輕觸按鈕 | 麵包板常用的方形按鈕 | 四隻腳兩兩相通，**取對角的兩隻**當兩隻腳 | 同上 |
| 三隻腳的模組（GND、VCC、OUT） | 按鈕模組、TTP223 觸摸、傾斜、磁簧、霍爾模組 | OUT 接腳位，模組自己輸出高低 | `Switch3Pins<Uno::UnoPin<PIN>>`，不開上拉；按下是 LOW 的模組加 `false` |

- **兩隻腳一定要上拉**：開關打開時沒有東西推腳位，不開上拉會讀到亂跳的雜訊。
- **三隻腳的 ACTIVE_HIGH**：觸摸模組和大部分按鈕模組按下是 HIGH（預設）；有些按鈕、傾斜、霍爾模組按下是 LOW。不確定就跑 `src/week_4_SSD1306_switch_buzzer_audio/tests/2_switch.cpp`。

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
