# DS18B20：介面

| 需要 | 得到 |
|---|---|
| 一個 `Wire`（1-Wire 匯流排）：`init()`、`reset()`、`write8()`、`read8()`…（`Uno::UnoOneWire<PIN>`） | `DS18B20::TO92`、`WaterproofProbe`、`KY001`：溫度（1/100 °C 或 float）、解析度 9 ～ 12 bits、不用等的轉換、錯誤碼 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [讀我.md](讀我.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 匯流排 | `Wire`：1-Wire，DQ 要有 4.7 kΩ 上拉（模組板子有，裸的要自己加） |
| 數量 | 一條線一顆（Skip ROM） |
| 時間 | 轉換要 `conversionMs()`（12 bit 約 750 ms）：用 `startConversion()` → `isConversionDone()` 不會等；`readCentiCelsius()` 會等 |
| 浮點 | 只有 `toCelsius()` 用到浮點；其他都是整數（0.01 °C） |

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
