# week_6_ultrasonic

超音波測距模組 HC-SR04（[lib/ultrasonic](../../lib/ultrasonic/README.md)）的測試，以及加上開關（[lib/switch](../../lib/switch/README.md)）和無源蜂鳴器（[lib/buzzer](../../lib/buzzer/README.md)）的作業。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python src/hide.py week_6_ultrasonic/tests/1_distance     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
pio device monitor                      # 1 Mbaud
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `tests/1_distance.cpp` | 每 200 ms 量一次距離，印 cm、Echo 的 µs 和長條圖 | HC-SR04 |
| `tests/2_median.cpp` | 每次量測和最近 `MEDIAN_COUNT` 次的中位數（`ultrasonic::Median`）並排印出 | HC-SR04 |
| `assignments/1_parking_sensor.cpp` | 作業一：按著開關、距離小於 50 cm 時叫，音量跟距離成反比 | 開關模組 + HC-SR04 + 無源蜂鳴器 |
| `assignments/2_theremin.cpp` | 作業二：距離在 5 ～ 50 cm 時叫，音高（頻率的對數）跟距離成線性 | HC-SR04 + 無源蜂鳴器 |

## 接線

| 零件 | 腳 | UNO | 哪些程式 |
|---|---|---|---|
| HC-SR04 | VCC | 5V | 全部 |
| | Trig | **D7** | |
| | Echo | **D8** | |
| | GND | GND | |
| 開關模組 | OUT | **D2** | `assignments/1_parking_sensor` |
| | VCC、GND | 5V、GND | |
| 無源蜂鳴器 | + | 100 Ω → **D9** | `assignments/1_parking_sensor`、`assignments/2_theremin` |
| | - | GND | |
| USB | D0、D1 | 串口 | 全部 |

| 腳 | 為什麼選它 |
|---|---|
| D8 | Echo 用腳位改變中斷，一組只能一個：放在 D8 ～ D13 這組，D2（紅外線接收器）那組就空著，可以一起用 |
| D7 | Trig 只是輸出，任何腳都可以；D7 沒有被其他週的零件用 |
| D2 | 跟 week_4_SSD1306_switch_buzzer_audio 一樣的開關接法；開關只是輪詢，不用中斷，所以 D2 那組的中斷還留給紅外線接收器 |
| D9 | 無源蜂鳴器在 D9 由 Timer1 直接輸出，可以調音量（PWM 的寬度），不佔 CPU |

# tests/1_distance.cpp：超音波測距

```
HC-SR04: Trig D7, Echo D8; sound at 346.5 m/s
Echo idle: LOW (ok)
 23.4 cm  echo  1351 us  |###########
100.2 cm  echo  5786 us  |##################################################>
too far / nothing (> 400 cm), or a soft / slanted surface
```

- 一個 `#` 是 2 cm，到 100 cm；更遠的結尾是 `>`。
- `AIR_TENTHS_C`：房間的溫度（0.1 °C），用來算音速。
- **一直 `no echo`**：沒電，或 Trig、Echo 接反（開機那行會是 `Echo idle: HIGH`）。
- **對著遠處一直 `too far`**：正常；對著布、海綿或斜的牆也會這樣。
- 拿尺比一比：誤差應該在幾 mm 內。

# tests/2_median.cpp：中位數濾波

```
HC-SR04 Trig D7, Echo D8, median of 5
raw   234 mm   median   233 mm  (5/5 good)
raw  1873 mm   median   234 mm  (5/5 good)      <- 一次錯的回音：raw 跳了，median 沒動
raw  too far   median   234 mm  (4/5 good)
```

- **raw**：這一次的量測；**median**：最近 `MEDIAN_COUNT` 次好的量測的中位數；**good**：其中幾次是好的。
- 好的不到一半時 median 印 `-`（前面沒東西，或接線有問題）。
- 手快速移動時，median 會慢約 `MEDIAN_COUNT / 2` 次才跟上：`MEDIAN_COUNT` 越大越穩、越慢。
- `ONLY_CHANGES = true`：只在 median 變了 1 cm 以上時才印。

# assignments/1_parking_sensor.cpp：按著開關的倒車雷達

按著開關、而且前面的東西比 `NEAR_MM`（50 cm）近時，蜂鳴器發出 2700 Hz 的聲音。音量跟距離成反比：

```
volume = 255 × FULL_MM ÷ 距離      （FULL_MM = 5 cm 以內都是 255，最大聲）
```

| 距離 | 音量 | 大約 |
|---|---|---|
| 50 cm 以上 | 0（不叫） | |
| 50 cm | 26 | -20 dB |
| 25 cm | 51 | -14 dB |
| 10 cm | 128 | -6 dB |
| 5 cm 以內（含 < 2 cm） | 255 | 最大 |

- **距離減半，音量加倍**：這就是「成反比」。音量是 `UnoTone` 的脈衝寬度（0 ～ 255），見 [lib/buzzer](../../lib/buzzer/README.md#音量)。
- **放開開關、超過 50 cm、量不到（沒回音）**：都不叫。
- **一直在量**：不管有沒有按，每 60 ms 量一次，所以一按下去馬上就有聲音；音量每 60 ms 更新一次，改音量時不會有爆音。
- 串口每 300 ms 印一行：`pressed   23.4 cm  volume 22  (sounding)`。
- 可以改的常數在檔案開頭的設定：`NEAR_MM`、`FULL_MM`、`MAX_VOLUME`、`TONE_HZ`、`AIR_TENTHS_C`，以及每隻腳。
- **開關**：這個開關模組按下時 OUT 是 LOW，所以設定是 `SWITCH_ACTIVE_HIGH = false`（實測）。換成按下是 HIGH 的模組就改成 `true`。

# assignments/2_theremin.cpp：用距離控制音高

距離在 `MIN_MM` ～ `MAX_MM` 之間時叫，**音高跟距離成線性**。音高是頻率的對數，所以每移動同樣的距離，音高變化同樣的音程（像在琴鍵上等距移動），不是同樣的 Hz：

```
t  = (距離 - MIN_MM) ÷ (MAX_MM - MIN_MM)          MIN_MM 是 0，MAX_MM 是 1
hz = HZ_AT_MIN × (HZ_AT_MAX ÷ HZ_AT_MIN)^t        log(hz) 從 log(HZ_AT_MIN) 線性走到 log(HZ_AT_MAX)
```

| 參數 | 預設 | 說明 |
|---|---|---|
| `MIN_MM` | 50（5 cm） | 最近會叫的距離；HC-SR04 量得到 20 ～ 4000 mm |
| `MAX_MM` | 500（50 cm） | 最遠會叫的距離 |
| `HZ_AT_MIN` | 1760（A6） | 在 `MIN_MM` 的頻率 |
| `HZ_AT_MAX` | 220（A3） | 在 `MAX_MM` 的頻率；比 `HZ_AT_MIN` 高就變成越遠越高 |
| `VOLUME` | 255 | 0 ～ 255 |

預設是 3 個八度、越近越高：

| 距離 | 5 cm | 12.5 cm | 20 cm | 27.5 cm | 35 cm | 42.5 cm | 50 cm |
|---|---|---|---|---|---|---|---|
| Hz | 1760 | 1245 | 880 | 622 | 440 | 311 | 220 |

- 每 15 cm 一個八度、每 1.25 cm 一個半音；Hz 不是等差，但聽起來是等距。
- **範圍外或量不到**：不叫。
- 每 60 ms 量一次，換音高時不會有爆音；手要穩，HC-SR04 本身有幾 mm 的誤差（約四分之一個半音）。
- 串口每 300 ms 印一行：`23.4 cm  775 Hz`。
- 編譯期檢查：`MIN_MM < MAX_MM`、在 20 ～ 4000 mm 內、頻率在 31 ～ 20000 Hz 內，不對就編譯錯誤。
