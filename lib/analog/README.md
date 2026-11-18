# analog

| 需要 | 得到 |
|---|---|
| `AnalogPin`：`init()`、`readAnalog()`…（`Uno::UnoAnalogPin<A0>`）；搖桿的按鈕另外要一個 `Pin`（`Uno::UnoInputPin<PIN, true>`） | `analog::Potentiometer`、`Photoresistor`、`KY023`（搖桿）、`KY013`（溫度）、`KY037`（聲音）：平滑後的值、百分比或任意範圍、像開關的門檻、溫度、音量和拍手 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

類比感測器，`namespace analog`。輸出是「一直在變的電壓」的零件，用 UNO 的 ADC 讀成 0 ～ 1023。

## 用法

```cpp
#include "Uno_analog_pin.h"
#include "Uno_pin.h"
#include "analog.h"

constexpr uint8_t KNOB_PIN = A0;                  // 旋鈕的中間腳
constexpr uint8_t STICK_X_PIN = A2;               // 搖桿 VRx
constexpr uint8_t STICK_Y_PIN = A3;               // 搖桿 VRy
constexpr uint8_t STICK_BUTTON_PIN = 4;           // 搖桿 SW

analog::Potentiometer<Uno::UnoAnalogPin<KNOB_PIN>> knob;
analog::KY023<Uno::UnoAnalogPin<STICK_X_PIN>, Uno::UnoAnalogPin<STICK_Y_PIN>, Uno::UnoInputPin<STICK_BUTTON_PIN, true>> stick;

void setup() {
    knob.init();
    stick.init();                 // 這時候不要碰搖桿：它記下中心
}

void loop() {
    knob.update();                // 每次 loop 都要呼叫（讀一次，約 112 µs）
    stick.update();
    uint8_t percent = knob.percent();     // 0 ～ 100
    int32_t angle = knob.map(0, 180);     // 0 ～ 180
    int8_t x = stick.x();                 // -100 ～ 100
}
```

完整的例子：`src/week_1_analog/tests/1_sensors.cpp`（五種感測器加上晶片自己的 5V 和溫度）。

## 接線

| 零件 | 腳 | UNO |
|---|---|---|
| 旋鈕（3 隻腳） | 兩邊 | 5V、GND（反過來接只是方向相反） |
| | 中間（滑動端） | A0 ～ A5 |
| 光敏電阻模組（KY-018 等，3 隻腳） | S / AO | A0 ～ A5 |
| | + / VCC、- / GND | 5V、GND |
| 裸的光敏電阻 | 一邊 5V，另一邊接 A0 ～ A5，再串 10 kΩ 到 GND | `BRIGHTER_IS_HIGHER = true` |
| 搖桿 KY-023（5 隻腳） | VRx、VRy | 兩隻 A0 ～ A5 |
| | SW | 任何數位腳（按下接到 GND，用內部上拉：`Uno::UnoInputPin<PIN, true>`） |
| | +5V、GND | 5V、GND |
| NTC 模組 KY-013（3 隻腳） | S | A0 ～ A5 |
| | 中間、- | 5V、GND |
| 聲音模組 KY-037 / KY-038（4 隻腳） | AO | A0 ～ A5 |
| | DO | 任何數位腳（比可變電阻設的大聲：用 `lib/switch` 的 `Switch3Pins` 讀） |
| | +、G | 5V、GND |

## 名稱

這些零件沒有共同的晶片，也沒有通訊協定（就是一個電壓），所以跟 `switch`、`buzzer`、`LED` 一樣用零件種類命名：`analog/`。有晶片的類比零件（例如 TCRT5000 的四腳板）放在晶片的函式庫裡（`TCRT5000/module/TCRT5000_4_pins.h`），也是用 AnalogPin。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| AnalogPin 的腳位 | A0 ～ A5（14 ～ 19） | 編譯錯誤（`UnoAnalogPin`） |
| `SMOOTHING` | 0 ～ 6 | 編譯錯誤 |
| `setDeadZone` | 0 ～ 90 % | 大於 90 當成 90 |
| `setRange` 兩個一樣 | | `map` 回傳 outLow |
| 讀數 | 0 ～ 1023 | — |
