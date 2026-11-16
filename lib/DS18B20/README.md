# DS18B20

DS18B20 數位溫度感測器（三隻腳：VCC、DQ、GND），`namespace DS18B20`，類別 `DS18B20::Thermometer`。資料夾用晶片命名；裸的 TO-92、防水探頭、KY-001 模組裡面都是同一顆 DS18B20，程式一樣，`module/` 只記錄各自的接法（差在有沒有上拉電阻）。只依賴一個 Wire（1-Wire 匯流排，例如 `Uno::UnoOneWire<PIN>`），換主板只要換 Wire。

## 檔案

```
lib/DS18B20/
├── DS18B20.h               入口：全部（程式只 include 它）
├── thermometer.h           Thermometer<Wire>：解析度、轉換、讀取、CRC（模組共用的核心）
└── module/                 實際買到的零件
    ├── TO_92.h             TO92<Wire>：裸的 DS18B20（黑色、一面平的、3 隻腳）
    ├── waterproof_probe.h  WaterproofProbe<Wire>：防水探頭（不鏽鋼管、3 條線）
    └── KY_001.h            KY001<Wire>：焊在小板子上、已經有上拉電阻
```

## 接線

| 感測器 | UNO |
|---|---|
| VCC | 5V（3.0 ～ 5.5 V） |
| DQ | 任何數位腳，例如 D2 |
| GND | GND |

| 零件 | 腳位 | 上拉電阻 | 用 |
|---|---|---|---|
| 裸的 DS18B20（TO-92） | 平面朝自己、腳朝下：GND、DQ、VDD | **要自己加** 4.7 kΩ（DQ 到 VCC） | `DS18B20::TO92` |
| 防水探頭 | 紅 VDD、黑 GND、黃（或白、藍）DQ；顏色依賣家，看說明 | **要自己加**，線長更需要 | `DS18B20::WaterproofProbe` |
| KY-001 等模組板 | 照板子印的字：S = DQ、中間 = VCC、- = GND | 板子上已經有 | `DS18B20::KY001` |

- **DQ 要有上拉電阻**：1-Wire 只靠上拉電阻把線拉回 HIGH。
- `Uno::UnoOneWire` 預設也會開 UNO 內部約 35 kΩ 的上拉，短線（幾十公分內）沒有外接電阻也可能可以用，但不保證。

## 用法

```cpp
#include "Uno_one_wire.h"
#include "DS18B20.h"

using SensorWire = Uno::UnoOneWire<2>;            // DQ 接 D2
DS18B20::KY001<SensorWire> sensor;                // 板子上有上拉；裸的用 DS18B20::TO92

sensor.init(12);                                  // 12 bits：0.0625 °C，一次 750 ms
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

完整的例子：`src/week_3/test_2.cpp`（把溫度用文字送到串口）、`src/week_3/assignment_1.cpp`（顯示在螢幕上）。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Wire` | `init()`、`reset()`（回傳有沒有裝置回應）、`write8(byte)`、`read8()`（低位元先）、`readBit()`、`nowMs()` | `Uno::UnoOneWire` |

## 函式

| 函式 | 說明 | 會等多久 |
|---|---|---|
| `init(bits)` | 初始化 Wire，設定解析度 | 約 10 ms |
| `setResolution(bits)` | 設定解析度 9 ～ 12 bits | 約 10 ms |
| `resolution()`、`conversionMs()` | 回傳目前的解析度、一次轉換要多久 | — |
| `startConversion()` | 開始量 | 約 2 ms |
| `isConversionDone()` | 回傳轉換完了沒（只能在 `startConversion()` 之後問） | 70 µs |
| `readRaw(raw)` | 讀最後一次的結果，檢查 CRC | 約 7 ms |
| `readCentiCelsius(centi)` | 開始、等、讀，三個一起 | 最多 `conversionMs()` + 50 ms |
| `toCentiCelsius(raw)` | 換成 1/100 °C，四捨五入 | — |
| `toCelsius(raw)` | 換成 `float` °C（會帶進浮點程式庫） | — |
| `crc8(data, length)` | 算 DS18B20 用的 CRC-8 | — |

除了換算的函式，都回傳一個結果：

| 值 | 名稱 | 原因 |
|---|---|---|
| 0 | `OK` | — |
| 1 | `ERROR_NO_SENSOR` | 沒有回應：DQ 接錯腳、沒有上拉、VCC / GND 沒接；或讀到的全是 0xFF |
| 2 | `ERROR_CRC` | 資料傳錯：雜訊、線太長、上拉太弱 |
| 3 | `ERROR_TIMEOUT` | 時間到了還沒轉換完（例如用寄生電源） |
| 4 | `ERROR_RESOLUTION` | bits 不是 9 ～ 12 |

## 解析度

| bits | 每格 | 一次轉換 | `conversionMs()` |
|---|---|---|---|
| 9 | 0.5 °C | 93.75 ms | 94 |
| 10 | 0.25 °C | 187.5 ms | 188 |
| 11 | 0.125 °C | 375 ms | 375 |
| 12（預設） | 0.0625 °C | 750 ms | 750 |

- 解析度存在感測器的 RAM，斷電就回到 12 bits；`init()` 會重新設定。
- 低解析度時，raw 沒用到的低位元會被清成 0。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `init(bits)`、`setResolution(bits)` | 9 ～ 12 | 回傳錯誤 `ERROR_RESOLUTION` |
| 量測範圍 | −55 ～ +125 °C（raw −880 ～ 2000） | 感測器的規格 |
| 準確度 | −10 ～ +85 °C 時 ±0.5 °C | 感測器的規格；解析度高不代表更準 |
| `toCentiCelsius` | −5500 ～ 12500 | — |
| 感測器數量 | 一條線一個（Skip ROM，不分辨是誰） | 不檢查：兩個以上會同時回答，CRC 錯或讀到錯的值 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | 1 byte（解析度）；讀取時另外用 9 bytes 堆疊 |
| 時間 | 每個位元約 70 µs，其中關中斷（`millis()` 不會少算，中斷只是延後） |
| 硬體 | 一隻腳 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 讀到 85.00 °C | 開機後還沒轉換過的值（`POWER_ON_RAW`） | 剛上電就讀、或轉換被斷電打斷時會出現：再量一次 |
| 寄生電源（VCC 接 GND） | 不支援：轉換時需要強上拉 | 接 VCC |
| 線很長 | `ERROR_CRC` 或 `ERROR_NO_SENSOR` | 用較小的上拉（例如 2.2 kΩ） |
| 同時用 `UnoTone` 在 D9 / D10 以外的腳 | 關中斷時方波暫停，聲音抖一下 | 蜂鳴器接 D9 或 D10 |
