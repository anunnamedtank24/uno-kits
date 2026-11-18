# LED

| 需要 | 得到 |
|---|---|
| 一個 `Pwm`：`init()`、`write(duty)`（`Uno::UnoPwmPin<PIN>`） | `LED::LED2Pins`、`RGBLED4Pins`、`TrafficLight4Pins`：照眼睛感覺的亮度 0 ～ 255；`LED::Effect`：閃爍、呼吸、漸變、心跳 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

用 PWM 調亮度的 LED，`namespace LED`。亮度照眼睛的感覺換算（0 ～ 255 看起來均勻變亮），不是直接的工作週期。

## 用法

```cpp
#include "Uno_pin.h"
#include "LED.h"

constexpr uint8_t RGB_RED_PIN = 3;                     // 都要是 PWM 腳：3、5、6、9、10、11
constexpr uint8_t RGB_GREEN_PIN = 5;
constexpr uint8_t RGB_BLUE_PIN = 6;
constexpr uint8_t LIGHT_RED_PIN = 9;
constexpr uint8_t LIGHT_GREEN_PIN = 10;
constexpr uint8_t SINGLE_PIN = 11;

LED::RGBLED4Pins<Uno::UnoPwmPin<RGB_RED_PIN>, Uno::UnoPwmPin<RGB_GREEN_PIN>, Uno::UnoPwmPin<RGB_BLUE_PIN>> rgb;
LED::TrafficLight4Pins<Uno::UnoPwmPin<LIGHT_RED_PIN>, LED::NoPwm, Uno::UnoPwmPin<LIGHT_GREEN_PIN>> trafficLight;   // 只用紅、綠
LED::LED2Pins<Uno::UnoPwmPin<SINGLE_PIN>> single;

void setup() {
    rgb.init();
    rgb.set(255, 128, 0);                                  // 橘色
    rgb.setBlue(LED::levelToBrightness(3, 10));            // 藍色第 3 格（共 10 格）
    trafficLight.init();
    trafficLight.setGreen(255);
    single.init();
    single.set(128);                                       // 看起來一半亮
}
```

完整的例子：`src/week_5_NEC/assignments/1_rgb_remote.cpp`（遙控器調紅綠燈的紅、綠和一顆藍色 LED）。

## Effect：閃爍、呼吸、漸變、心跳

```cpp
constexpr uint8_t LED_PIN = 6;
LED::LED2Pins<Uno::UnoPwmPin<LED_PIN>> light;
LED::Effect<decltype(light)> effect(light);

effect.breathe(2000);          // 2 秒一次呼吸
// loop 裡：effect.update();
```

| 函式 | 說明 |
|---|---|
| `steady(brightness)` | 固定亮度 |
| `blink(onMs, offMs, count, brightness)` | 閃爍；`count` 次後熄滅（0 = 一直閃） |
| `breathe(periodMs, low, high)` | 在 low 和 high 之間平順地變亮變暗，一次呼吸 periodMs |
| `fadeTo(brightness, ms)` | 從現在的亮度漸變到 brightness |
| `heartbeat(periodMs, brightness)` | 兩下短跳、休息（心跳） |
| `update()` | 每次 loop 都要呼叫 |
| `kind()`、`isRunning()` | 現在是哪一種；有次數的閃爍、漸變結束後是 false |

- **亮度是眼睛看的**（0 ～ 255，見 `brightness.h`），所以呼吸看起來是均勻的。
- 用在一個顏色上（`SingleLED`、`LED2Pins`）；RGB LED 的顏色要一起變，用 `set(r, g, b)`。
- 完整的例子：`src/week_2_LED/tests/1_effects.cpp`（每種效果輪流）、`src/week_4_SSD1306_switch_buzzer_audio/tests/6_clicks.cpp`（按鈕手勢換效果）。

## 接線

**裸 LED 一定要串電阻**（220 ～ 330 Ω）：LED 自己不限流，直接接腳位會燒掉 LED 或腳位。板子上已經有電阻的模組（紅綠燈模組、部分 RGB 模組）不用再串。

一顆 LED：

| LED | UNO |
|---|---|
| 長腳（+） | 220 Ω → PWM 腳，例如 D3 |
| 短腳（-） | GND |

RGB LED（4 隻腳，最長的是共用腳）：

| RGB LED | 共陰極（common cathode） | 共陽極（common anode） |
|---|---|---|
| R | 220 Ω → D3 | 220 Ω → D3 |
| 最長的腳 | GND | 5V |
| G | 220 Ω → D5 | 220 Ω → D5 |
| B | 220 Ω → D6 | 220 Ω → D6 |
| 參數 | `COMMON_ANODE = false`（預設） | `COMMON_ANODE = true` |

- **分辨共陰、共陽**：最長的腳接 GND、另一隻腳串電阻接 5V 會亮的是共陰；要反過來才亮的是共陽。模組標 `-` 的（KY-016 等）是共陰。
- **PWM 腳**：UNO 只有 D3、D5、D6、D9、D10、D11 能調亮度，其他腳編譯錯誤。
- **模組板**：有些 RGB 模組已經焊了電阻，就不用再串。
紅綠燈模組（4 隻腳：R、Y、G、GND，板子上已經有電阻）：

| 紅綠燈 | UNO |
|---|---|
| R | D3（直接接，不用電阻） |
| Y | D5（不用的顏色就不接，程式給 `LED::NoPwm`） |
| G | D6 |
| GND（或 -） | GND |

- **三顆分開的 LED 也可以**：紅、綠、藍三顆的長腳各串 220 Ω 接 D3、D5、D6，短腳都接 GND，就等於共陰極的 RGB LED，程式不用改。差別是光從三個點出來，近看不會混成一個顏色；靠在一起、蓋一張白紙就會混。

## 亮度和 PWM

**PWM**：腳位很快地開關（UNO 約 976 Hz），開的時間比例（工作週期）越長看起來越亮。眼睛跟不上這麼快的閃爍，所以看到的是平均亮度。

**為什麼要換算**：眼睛對暗的變化比較敏感，工作週期 50% 看起來大約有 70% 亮。直接用工作週期調，前半段變化很大、後半段幾乎看不出來。

| 亮度（`set`） | 工作週期（`toDuty`） | 看起來 |
|---|---|---|
| 0 | 0 | 關 |
| 64 | 16（6%） | 約四分之一 |
| 128 | 64（25%） | 約一半 |
| 255 | 255（100%） | 最亮 |

- **格數**：`levelToBrightness(level, levels)` 把 0 ～ levels 平均分給 0 ～ 255，配上 `toDuty` 每一格看起來差不多一樣大。
- **最暗的幾格**：工作週期只有整數，亮度 1 ～ 15 都是 0 ～ 1，看起來是關或很暗。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 腳位 | 3、5、6、9、10、11 | 編譯錯誤 |
| 亮度、工作週期 | 0 ～ 255 | — |
| `levelToBrightness` 的 level | 0 ～ levels | 大於 levels 時是 255；levels = 0 時是 0 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 不串電阻 | 電流太大，LED 或腳位燒掉 | 每個顏色各串 220 ～ 330 Ω |
| 三個顏色共用一顆電阻（串在共用腳） | 顏色互相影響，紅色比較亮（壓降最低） | 每個顏色各一顆 |
| LED 接 D9 / D10，同時用無源蜂鳴器（`UnoTone`） | 兩邊搶 Timer1，亮度和音高都錯 | LED 改接 D3、D5、D6、D11 |
| LED 接 D3 / D11，同時用 Arduino 的 `tone()` | 搶 Timer2 | 不要用 `tone()`（用 `lib/buzzer`） |
| 共陽極設成共陰極 | 亮暗反過來：0 最亮 | 改 `COMMON_ANODE` |
| 一隻腳推很多 LED | 一隻腳建議 ≤ 20 mA | 用電晶體 |
