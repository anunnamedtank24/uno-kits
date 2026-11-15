# LED

用 PWM 調亮度的 LED，`namespace LED`。亮度照眼睛的感覺換算（0 ～ 255 看起來均勻變亮），不是直接的工作週期。只依賴 Pwm（例如 `Uno::UnoPwmPin`），換主板只要換它。

## 檔案

```
lib/LED/
├── LED.h                       入口：全部（程式只 include 它）
├── brightness.h                toDuty（眼睛的亮度 → 工作週期）、levelToBrightness（第幾格 → 亮度）
├── single_LED.h                SingleLED<Pwm, ACTIVE_HIGH>：一個顏色（模組共用的核心）；NoPwm：沒接的顏色
└── module/                     實際買到的零件
    ├── LED_2_pins.h            LED2Pins<Pwm>：一顆 LED（2 隻腳）
    ├── RGB_LED_4_pins.h        RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>：RGB LED（4 隻腳）
    └── traffic_light_4_pins.h  TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>：紅綠燈模組（R、Y、G、GND）
```

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

## 用法

```cpp
#include "Uno_pin.h"
#include "LED.h"

LED::RGBLED4Pins<Uno::UnoPwmPin<3>, Uno::UnoPwmPin<5>, Uno::UnoPwmPin<6>> rgb;            // R = D3、G = D5、B = D6
LED::TrafficLight4Pins<Uno::UnoPwmPin<9>, LED::NoPwm, Uno::UnoPwmPin<10>> trafficLight;    // 只用紅、綠
LED::LED2Pins<Uno::UnoPwmPin<11>> single;

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

完整的例子：`src/week_5/assignment_1.cpp`（遙控器調紅綠燈的紅、綠和一顆藍色 LED）。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Pwm` | `init()`（腳位 LOW）、`write(duty)`（0 = 一直 LOW、255 = 一直 HIGH） | `Uno::UnoPwmPin` |

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 設定腳位，關掉 |
| `SingleLED::set(brightness)` | 設定亮度：0 = 關 ～ 255 = 最亮，照眼睛的感覺 |
| `SingleLED::setDuty(duty)` | 直接設定工作週期，不換算 |
| `SingleLED::brightness()` | 回傳目前的亮度 |
| `RGBLED4Pins::set(red, green, blue)`、`TrafficLight4Pins::set(red, yellow, green)` | 設定每個顏色的亮度 |
| `setRed`、`setGreen`、`setBlue`、`setYellow` | 設定一個顏色 |
| `red`、`green`、`blue`、`yellow` | 回傳一個顏色的亮度 |
| `NoPwm` | 什麼都不做的 Pwm：給沒接的顏色 |
| `toDuty(brightness)` | 回傳亮度對應的工作週期：brightness² / 255 |
| `levelToBrightness(level, levels)` | 回傳第 level 格（共 levels 格）的亮度，0 ～ 255 |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `ACTIVE_HIGH`（核心 `SingleLED`） | `true` | 自己接線時用：5V → LED → 電阻 → 腳位時改成 `false`（`LED2Pins` 固定 `true`） |
| `COMMON_ANODE`（`RGBLED4Pins`） | `false` | 共陰是 `false`、共陽是 `true` |

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

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `LED2Pins` 1 byte，`RGBLED4Pins`、`TrafficLight4Pins` 3 bytes（記住的亮度） |
| 計時器 | D5、D6：Timer0（跟 `millis` 共用，只開關輸出）；D9、D10：Timer1；D3、D11：Timer2 |
| CPU | 不用：計時器自己產生 PWM |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 不串電阻 | 電流太大，LED 或腳位燒掉 | 每個顏色各串 220 ～ 330 Ω |
| 三個顏色共用一顆電阻（串在共用腳） | 顏色互相影響，紅色比較亮（壓降最低） | 每個顏色各一顆 |
| LED 接 D9 / D10，同時用無源蜂鳴器（`UnoTone`） | 兩邊搶 Timer1，亮度和音高都錯 | LED 改接 D3、D5、D6、D11 |
| LED 接 D3 / D11，同時用 Arduino 的 `tone()` | 搶 Timer2 | 不要用 `tone()`（用 `lib/buzzer`） |
| 共陽極設成共陰極 | 亮暗反過來：0 最亮 | 改 `COMMON_ANODE` |
| 一隻腳推很多 LED | 一隻腳建議 ≤ 20 mA | 用電晶體 |
