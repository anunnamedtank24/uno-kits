# analog：介面

| 需要 | 得到 |
|---|---|
| `AnalogPin`：`init()`、`readAnalog()`…（`Uno::UnoAnalogPin<A0>`）；搖桿的按鈕另外要一個 `Pin`（`Uno::UnoInputPin<PIN, true>`） | `analog::Potentiometer`、`Photoresistor`、`KY023`（搖桿）、`KY013`（溫度）、`KY037`（聲音）：平滑後的值、百分比或任意範圍、像開關的門檻、溫度、音量和拍手 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [讀我.md](讀我.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 主板 | `AnalogPin`：讀 0 ～ `MAX`（UNO 1023）、`referenceMillivolts()`；換算成 % 或 °C 都以參考電壓為準（預設 5V） |
| 呼叫 | `update()` 每次 loop 都要呼叫（約 112 µs 讀一次），濾波和事件靠它 |
| 搖桿 | `init()` 時記下中心：開機時不要碰 |
| 硬體 | 類比腳只有 A0 ～ A5；搖桿的按鈕另外要一個數位 `Pin`（UNO 的上拉） |

## 檔案

```
lib/analog/
├── analog.h            入口：全部（程式只 include 它）
├── input.h             Input<AnalogPin, SMOOTHING>：平滑、範圍、百分比 / 任意範圍、門檻、變化（模組共用的核心）
├── joystick.h          Joystick<XPin, YPin, ButtonPin, INVERT_X, INVERT_Y>：兩軸 -100 ～ 100、死區、方向、按鈕
├── thermistor.h        Thermistor<AnalogPin, NTC_ON_GND, R0_OHMS, BETA, FIXED_OHMS>：NTC 熱敏電阻的溫度
├── sound_level.h       SoundLevel<AnalogPin, WINDOW_MS>：麥克風模組的音量、拍手
└── module/             實際買到的零件
    ├── potentiometer.h Potentiometer<AnalogPin>：旋鈕、滑桿、微調電阻
    ├── photoresistor.h Photoresistor<AnalogPin, BRIGHTER_IS_HIGHER>：光敏電阻、KY-018、光感測板
    ├── KY_023.h        KY023<XPin, YPin, ButtonPin>：搖桿
    ├── KY_013.h        KY013<AnalogPin>：NTC 溫度模組
    └── KY_037.h        KY037<AnalogPin>、KY038<AnalogPin>：聲音模組
```

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `AnalogPin` | `init()`、`readAnalog()`、`readAnalogAverage(n)`、`referenceMillivolts()`、`nowMs()`、常數 `MAX` | `Uno::UnoAnalogPin` |
| `ButtonPin`（搖桿） | `init()`、`read()` | `Uno::UnoInputPin<PIN, true>` |

## 函式

### Input：一個類比感測器（旋鈕、光敏電阻都是它）

| 函式 | 說明 |
|---|---|
| `init()` | 讀一次，平滑從這裡開始 |
| `update()` | 讀一次、平滑；剛越過門檻時回傳 true |
| `raw()`、`value()` | 最後一次的原始讀數、平滑後的值（0 ～ 1023） |
| `millivolts()` | 平滑後的值換成 mV |
| `setRange(low, high)` | 什麼算 0 % 和 100 %（預設 0、1023）；low > high 就是反過來 |
| `startLearning()`、`stopLearning()` | 自己學範圍：開始後把旋鈕轉到兩端、或讓光感測器全亮全暗 |
| `rangeLow()`、`rangeHigh()` | 現在的範圍 |
| `percent()`、`permille()` | 0 ～ 100、0 ～ 1000 |
| `map(outLow, outHigh)` | 換成任意範圍，例如 `map(-100, 100)`、`map(0, 180)` |
| `setThreshold(on, off)` | 門檻（像開關）：升到 on 變開、降到 off 變關；on < off 就是反過來（降下去才開） |
| `isOn()`、`wasTurnedOn()`、`wasTurnedOff()` | 門檻的狀態、剛開 / 剛關（呼叫後清掉） |
| `hasChanged(delta)` | 從上次回傳 true 之後，值變了至少 delta（旋鈕被轉了；忽略雜訊） |

- **平滑**（`SMOOTHING`）：0 = 不平滑；n = 大約最近 2ⁿ 次的平均（預設 3，約 8 次）。越大越穩、反應越慢。
- **門檻要留間隔**：on 和 off 一樣時，值在門檻附近會一直開開關關；差個幾十比較穩。

### Photoresistor：光敏電阻

`Input` 再加上 `brightness()`（0 = 暗 ～ 100 = 亮）。

| `BRIGHTER_IS_HIGHER` | 接法 | 怎麼確認 |
|---|---|---|
| `true` | 裸的光敏電阻：5V — LDR — 腳 — 10 kΩ — GND | 用手指蓋住，`value()` 變小 |
| `false` | 大部分的模組（電阻在 GND 那邊） | 用手指蓋住，`value()` 變大 |

### Joystick：搖桿

| 函式 | 說明 |
|---|---|
| `init()` | 記下中心（搖桿要放開） |
| `update()` | 讀兩軸和按鈕（約 230 µs）；方向或按鈕改變時回傳 true |
| `x()`、`y()` | -100（左 / 下）～ 100（右 / 上），死區內是 0 |
| `direction()` | `CENTER`、`UP`、`DOWN`、`LEFT`、`RIGHT`：推超過 50 % 才算，回到 30 % 以下才回中心 |
| `wasMoved(direction)` | 剛推向一個方向時回傳 true 一次（選單一格一格移動） |
| `isPressed()`、`wasPressed()` | 按鈕（去彈跳） |
| `setDeadZone(percent)` | 中心附近當成 0 的範圍（預設 8 %） |
| `recenter()` | 重新記中心 |

- **兩邊分開算**：中心不一定在 512，左右（上下）各自換成 0 ～ 100，所以推到底一定是 ±100。
- **方向反了**：改 `INVERT_X`、`INVERT_Y`（`KY023` 預設 `INVERT_Y = true`）。

### Thermistor：NTC 熱敏電阻

| 函式 | 說明 |
|---|---|
| `readTemperature()` | 讀 8 次平均（約 1 ms），回傳 0.1 °C；讀數在兩端（沒接好）回傳 `INVALID`（-32768） |
| `ohms()` | 上一次算出的 NTC 電阻 |
| `calibrate(tenthsC)` | 用一個已知溫度（例如旁邊的 DS18B20）校正 R0 |

- **公式**（Beta）：1 / T = 1 / 298.15 + ln(R / R0) / B，T 是絕對溫度。
- **參數**：`R0`（25 °C 的電阻，預設 10 kΩ）、`BETA`（預設 3950）、`FIXED`（分壓電阻，預設 10 kΩ）、`NTC_ON_GND`（NTC 在 GND 那邊：越熱讀數越低）。
- **準確度**：典型值約 ± 1 ～ 2 °C；校正後更好。比 DS18B20 差，但便宜、反應快。
- 用到浮點（`logf`），約多 1 KB Flash。

### SoundLevel：聲音模組

| 函式 | 說明 |
|---|---|
| `update()` | 讀一次；一個視窗（`WINDOW_MS`，預設 50 ms）結束時回傳 true |
| `level()` | 上一個視窗的擺幅（最高 - 最低），0 ～ 1023 |
| `peak()` | 上次呼叫之後最大的擺幅（呼叫後清掉） |
| `middle()` | 波形的中間值：把模組的可變電阻調到約 512 |
| `heard(threshold)` | 拍手、敲桌子：擺幅升到 threshold 時回傳 true 一次，降到一半以下才能再觸發 |

- **呼叫越頻繁越準**：聲音是很快的波，`update()` 每次只讀一個點；loop 裡不要有 `delay`。
- 這種模組不靈敏：安靜約 0 ～ 5，近距離說話幾十，對著拍手幾百。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| AnalogPin 的腳位 | A0 ～ A5（14 ～ 19） | 編譯錯誤（`UnoAnalogPin`） |
| `SMOOTHING` | 0 ～ 6 | 編譯錯誤 |
| `setDeadZone` | 0 ～ 90 % | 大於 90 當成 90 |
| `setRange` 兩個一樣 | | `map` 回傳 outLow |
| 讀數 | 0 ～ 1023 | — |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `Input` 約 20 bytes、`Joystick` 約 20 bytes、`Thermistor` 8 bytes、`SoundLevel` 約 25 bytes |
| 時間 | 每次讀約 112 µs（ADC 忙碌等待）；`Thermistor::readTemperature` 約 1 ms |
| 硬體 | ADC（一次一隻腳），不用中斷 |
