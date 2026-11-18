# ultrasonic

| 需要 | 得到 |
|---|---|
| `Trigger`：輸出腳（`Uno::UnoOutputPin<PIN>`）；`Echo`：有中斷的輸入腳（`Uno::UnoInterruptPin<PIN>`） | `ultrasonic::HCSR04`：距離（mm），量的時候不會等，結果碼，溫度修正；`ultrasonic::Median`：比較穩的距離 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

超音波測距模組，`namespace ultrasonic`。

## 用法

```cpp
#include "Uno_pin.h"
#include "Uno_interrupt_pin.h"
#include "ultrasonic.h"

constexpr uint8_t TRIG_PIN = 7;
constexpr uint8_t ECHO_PIN = 8;
using Ranger = ultrasonic::HCSR04<Uno::UnoOutputPin<TRIG_PIN>, Uno::UnoInterruptPin<ECHO_PIN>>;

void setup() {
    Ranger::init();
    Ranger::setTemperature(250);      // 25.0 °C（可省略，預設 20.0 °C）
}

void loop() {
    if (Ranger::update()) {           // 每次 loop 都要呼叫，不會等；量完一次回傳 true
        if (Ranger::result() == ultrasonic::RESULT_OK) {
            uint16_t mm = Ranger::distanceMm();
        }
    }
}
```

全部是 static（中斷沒有物件可以呼叫），所以用 `Ranger::` 不用宣告變數；一個 Echo 腳位一個 Ranger。完整的例子：`src/week_6_ultrasonic/tests/1_distance.cpp`。

## Median：比較穩的距離

偶爾一次錯的回音（斜面反射、漏掉）會讓距離跳一下。`Median` 取最近 N 次好的量測的中位數，一兩次錯的不會影響結果。

```cpp
using Sensor = ultrasonic::HCSR04<Uno::UnoOutputPin<TRIG_PIN>, Uno::UnoInterruptPin<ECHO_PIN>>;
ultrasonic::Median<Sensor, 5> distance;

distance.init();                        // 也會 init 感測器
// loop 裡：
if (distance.update() && distance.isValid()) {
    uint16_t mm = distance.distanceMm();
}
```

| 函式 | 說明 |
|---|---|
| `init()` | 也 init 感測器 |
| `update()` | 呼叫感測器的 `update()`；有新的量測時回傳 true |
| `distanceMm()` | 最近 N 次好的量測的中位數 |
| `goodCount()` | 最近 N 次裡有幾次是 `RESULT_OK` |
| `isValid()` | 至少一半是好的（否則前面可能沒東西） |

- `N` 是 3 ～ 15 的奇數（預設 5）；越大越穩，但慢 N / 2 次量測（N = 5 約 150 ms）。

## 接線

| HC-SR04 | UNO | 說明 |
|---|---|---|
| VCC | 5V | HC-SR04 要 5 V（HC-SR04P 是 3 ～ 5.5 V） |
| Trig | 任何數位腳，例如 **D7** | `Uno::UnoOutputPin<TRIG_PIN>` |
| Echo | 任何數位腳，例如 **D8** | `Uno::UnoInterruptPin<ECHO_PIN>`；輸出 5 V，UNO 可以直接接 |
| GND | GND | |

- **Echo 用中斷量**：腳位改變中斷一組只能有一個（D0 ～ D7、D8 ～ D13、A0 ～ A5），紅外線接收器在 D2，所以 Echo 放 D8，兩個可以同時用。
- 前面要空：探頭前方約 15° 的圓錐裡最近的東西就是量到的距離。

## 名稱

| 名稱 | 是什麼 |
|---|---|
| **HC-SR04** | 模組的型號（零件），放在 `module/HC_SR04.h` |
| 晶片 | 沒有固定的：不同廠商的板子用不同的晶片（EM78P153、RCWL-9206、CS100A…），但腳位和用法都一樣 |
| **Trig / Echo** | 用法（通訊方式）：Trig 給一個脈衝開始量，Echo 的 HIGH 有多長就是聲音來回的時間 |

照「`lib/` 用晶片或通訊方式命名」的規則，這裡沒有共同的晶片，Trig / Echo 也不是有名字的協定，所以跟 `switch`、`buzzer`、`LED` 一樣用零件種類命名：`ultrasonic/`。以後其他 Trig / Echo 的模組（JSN-SR04T 防水探頭、US-100）放 `module/`。

## 原理

```
Trig  ──┐▔▔┐────────────────────────────────────    ≥ 10 µs 的 HIGH：開始量一次
Sound      ))))))))                    ((((((((      送 8 個 40 kHz 的超音波（約 0.2 ms），碰到東西反彈回來
Echo  ───────────┐▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┐──────────    送完變 HIGH，聽到回音變 LOW
                 └──── 來回的時間 ─────┘
```

- **距離** = Echo 的 HIGH 時間 × 音速 ÷ 2（來回）。20 °C 時音速 343 m/s，**每 58 µs 是 1 cm**。
- **沒有回音**：Echo 約 38 ms 後自己變回 LOW（有些仿製品要更久，見 `RESULT_BUSY`）。
- **兩次之間至少隔 60 ms**：上一次的超音波可能還在房間裡反彈，太快再量會把舊的回音當成新的。

## 結果

| 結果 | 意思 | 常見原因 |
|---|---|---|
| `RESULT_OK` | 量到了，`distanceMm()` 就是距離 | |
| `RESULT_TOO_NEAR` | 比 `MIN_MM`（2 cm）近 | 回音在模組還在送的時候就回來了 |
| `RESULT_TOO_FAR` | 超過 `MAX_MM`（400 cm）還沒回音 | 前面沒東西；布、海綿吸音；表面斜的把聲音反射到別處 |
| `RESULT_NO_ECHO` | Echo 一直沒變 HIGH | 沒電、Trig 和 Echo 接反、沒接好 |
| `RESULT_BUSY` | 要量的時候 Echo 還是 HIGH | 有些仿製品沒回音時 Echo 會 HIGH 很久；這次不量，下個間隔再試 |
| `RESULT_NONE` | 還沒量過 | |

## 準確度

| 誤差來源 | 大小 | 怎麼辦 |
|---|---|---|
| `nowUs()` 每 4 µs 一格 | 約 0.7 mm | — |
| 中斷的延遲（別的中斷正在跑，例如 `millis`） | 幾 µs，約 1 mm | — |
| 氣溫 | 每差 10 °C 約 1.7 %（1 m 差 1.7 cm） | `setTemperature`，可以用 DS18B20 量 |
| 模組本身 | 約 3 mm；表面越小、越斜越不準 | 量幾次取中位數 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| Trigger、Echo 的腳位 | 0 ～ 19；D0 / D1 編譯警告（USB 串口） | 編譯錯誤 |
| `MIN_MM < MAX_MM`、`MAX_MM` ≤ 10000 | | 編譯錯誤 |
| `setIntervalMs` | 60 ～ 65535 ms | 小於 60 的當成 60 |
| `setTemperature` | -400 ～ 800（-40 ～ 80 °C） | 夾到範圍內 |
| 同一組（D0 ～ D7、D8 ～ D13、A0 ～ A5）有別的 `UnoInterruptPin` | | 後 `init` 的蓋掉前一個 |
