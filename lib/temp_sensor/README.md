# temp_sensor

DS18B20 數位溫度感測器（三支腳：VCC、DQ、GND），`namespace temp_sensor`。只依賴一個 Wire（1-Wire 匯流排），Wire 例如 `Uno::UnoOneWire<PIN>`，所以換主板只要換 Wire。

## 接線

| 感測器 | UNO |
|---|---|
| VCC | 5V（3.0 ～ 5.5 V） |
| DQ | 任何數位腳，例如 D2 |
| GND | GND |

- **DQ 要有上拉電阻**：4.7 kΩ 接到 VCC。大部分三支腳的模組板上已經有了；只有裸的 DS18B20（TO-92 封裝）要自己加。
- `Uno::UnoOneWire` 預設也會開 UNO 內部約 35 kΩ 的上拉，短線（幾十公分內）沒有外接電阻也可能可以用，但不保證。

## 用法

```cpp
#include "Uno_one_wire.h"
#include "temp_sensor.h"

using SensorWire = Uno::UnoOneWire<2>;            // DQ 接 D2
temp_sensor::DS18B20<SensorWire> sensor;

sensor.init(12);                             // 12 bits：0.0625 °C，一次 750 ms
int16_t centi;
if (sensor.readCentiCelsius(centi) == sensor.OK) {
    // centi = 2350 表示 23.50 °C
}
```

不想卡住 750 ms 時，分步驟做：

```cpp
sensor.startConversion();
// ……做別的事，或一直問 sensor.isConversionDone()
int16_t raw;
if (sensor.readRaw(raw) == sensor.OK) {
    int16_t centi = sensor.toCentiCelsius(raw);
}
```

完整的例子：[src/test/test_2.cpp](../../src/test/test_2.cpp)，每秒把溫度用文字送到串口。

## 函式

| 函式 | 作用 | 會等多久 |
|---|---|---|
| `init(bits)` | 初始化 Wire，設定解析度 | 約 10 ms |
| `setResolution(bits)` | 9 ～ 12 bits | 約 10 ms |
| `resolution()`、`conversionMs()` | 目前的解析度、一次轉換要多久 | — |
| `startConversion()` | 開始量 | 約 2 ms |
| `isConversionDone()` | 轉換完了沒（只能在 `startConversion()` 之後問） | 70 µs |
| `readRaw(raw)` | 讀最後一次的結果，檢查 CRC | 約 7 ms |
| `readCentiCelsius(centi)` | 上面三個一起：開始、等、讀 | 最多 `conversionMs()` + 50 ms |
| `toCentiCelsius(raw)` | 換成 1/100 °C，四捨五入 | — |
| `toCelsius(raw)` | 換成 `float` °C（會帶進浮點程式庫） | — |
| `crc8(data, length)` | DS18B20 用的 CRC-8 | — |

## 解析度

| bits | 每格 | 一次轉換 | `conversionMs()` |
|---|---|---|---|
| 9 | 0.5 °C | 93.75 ms | 94 |
| 10 | 0.25 °C | 187.5 ms | 188 |
| 11 | 0.125 °C | 375 ms | 375 |
| 12（預設） | 0.0625 °C | 750 ms | 750 |

- 解析度存在感測器的 RAM，斷電就回到 12 bits；`init()` 會重新設定。
- 低解析度時，raw 沒用到的低位元會被清成 0。

## 回傳值

| 值 | 名稱 | 原因 |
|---|---|---|
| 0 | `OK` | — |
| 1 | `ERROR_NO_SENSOR` | 沒有回應：DQ 接錯腳、沒有上拉、VCC / GND 沒接；或讀到的全是 0xFF |
| 2 | `ERROR_CRC` | 資料傳錯：雜訊、線太長、上拉太弱 |
| 3 | `ERROR_TIMEOUT` | 時間到了還沒轉換完（例如用寄生電源） |
| 4 | `ERROR_RESOLUTION` | bits 不是 9 ～ 12 |

## 輸入限制和細節

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 量測範圍 | −55 ～ +125 °C（raw −880 ～ 2000） | 感測器的規格 |
| 準確度 | −10 ～ +85 °C 時 ±0.5 °C | 感測器的規格；解析度高不代表更準 |
| raw | `int16_t`，每格 1/16 °C | — |
| `toCentiCelsius` | `int16_t`，−5500 ～ 12500 | — |
| 感測器數量 | **一條線一個**（用 Skip ROM，不分辨是誰） | 兩個以上：同時回答，CRC 錯或讀到錯的值 |
| 電源 | 要接 VCC | 寄生電源（VCC 接 GND）不支援：轉換時需要強上拉 |
| 85.00 °C | 也是開機後還沒轉換過的值（`POWER_ON_RAW`） | 剛上電就讀、或轉換被斷電打斷時會出現 |
| 中斷 | 每個位元的時序裡關中斷約 70 µs | `millis()` 不會少算（中斷只是延後） |
| 線長 | 短線最穩；長線要較小的上拉（例如 2.2 kΩ） | `ERROR_CRC` 或 `ERROR_NO_SENSOR` |
