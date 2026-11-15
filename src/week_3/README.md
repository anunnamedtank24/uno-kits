# week_3

M130T（1.3 吋 240×240 ST7789）的完整測試、DS18B20 溫度感測器、彩色圖片，以及三個作業。

`src/` 裡同時只能有一個檔案有 `setup()` / `loop()`，其他的要改成 `.bak`。用 `hide.py` 只留下要跑的那一個：

```bash
python -B src/hide.py week_3/assignment_2     # 其他全部改成 .bak，assignment_2 留下（或拿掉 .bak）
python -B src/hide.py 'w*3*as*1'              # "*" 分段比對，要加引號
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `test_1.cpp` | 螢幕、圖形、文字、GIF 的完整測試 | M130T |
| `test_2.cpp` | DS18B20 溫度，每秒用文字送到串口 | DS18B20 |
| `test_3.cpp` | 彩色圖片（[lib/image](../../lib/image/README.md)）：16 / 256 / 565 / 2 色、裁切、從 Mac 串流 240×240 全彩 | M130T |
| `assignment_1.cpp` | 作業一：溫度、raw 值、幀率顯示在螢幕上，用顏色表示變化 | M130T + DS18B20 |
| `assignment_2.cpp` | 作業二：漣漪（藝術作品），雨勢隨機變化 | M130T |
| `assignment_3.cpp` | 作業三：兩條射線，參數空間裡的旋轉與震動 | M130T |

## 接線

UNO + M130T（1.3 吋 240×240，沒有 CS），每個檔案都一樣：

| M130T | UNO |
|---|---|
| SCL | D13 |
| SDA | D11 |
| DC | D8 |
| RES | D10 |
| BLK | D9 |
| VCC / GND | 3.3V / GND |

DS18B20（`test_2`、`assignment_1`）：

| DS18B20 | UNO |
|---|---|
| VCC | 5V |
| DQ | D2（要有 4.7 kΩ 上拉到 VCC，大部分模組板上已經有） |
| GND | GND |

# test_1.cpp

每個測試畫一個畫面，停 1 秒（`HOLD_MS`）再清畫面跑下一個，一直循環。每個測試上方的註解寫著「應該看到什麼」。

## 測試

| `tests[i]` | 函式 | 測什麼 |
|---|---|---|
| 0 | `testColors` | 初始化、RGB/BGR 順序、反相：紅 → 綠 → 藍 → 白 → 黑 |
| 1 | `testBorder` | 尺寸和偏移：白框貼齊四邊，四角紅、綠、藍、黃 |
| 2 | `testRamRect` | `fillWindow`、`writeWindow`，包括剛好貼著左、右、下邊的方塊 |
| 3 | `testPoint` | 點：每 10 px 一個 |
| 4 | `testHVLines` | 橫線、直線的三種寫法，每組三條要一樣 |
| 5 | `testLines` | 斜線的三種寫法：扇形、星形、X |
| 6 | `testRects` | 矩形的三種寫法，空心和實心 |
| 7 | `testTriangles` | 三角形的三種寫法，空心要剛好貼在實心的邊上 |
| 8 | `testCircles` | 圓的兩種寫法 |
| 9 | `testBorders` | 空心的邊界寬度 1、6、40：彩色邊界蓋在同樣大小的灰色實心上，外面不能露出灰色 |
| 10 | `testClip` | 剛好貼著螢幕邊緣的圖形（不裁切，所以圖形必須在螢幕上），不能繞到另一邊 |
| 11 | `testTextChars` | 字型的每個字元，沒有空隙也沒有重疊 |
| 12 | `testTextTypes` | 字串、`F()`、字元、8～32 位元整數 |
| 13 | `testTextFloat` | 浮點數、`setDecimals`、nan、inf |
| 14 | `testTextArrays` | 陣列、`setSeparator` |
| 15 | `testTextDefault` | 字型範圍外的字元畫成 `?` |
| 16 | `testTextLayout` | 透明背景、回傳的 x、`fontHeight`、`F()`、貼著邊緣的文字 |
| 17 | `testTextLineBreak` | `setAutoLineBreak`、`setLineSpacing` |
| 18 | `testGifFlash` | `GIF::FlashPlayer`：播 4 秒，再故意停 1 秒，看它跳到 I 幀追上 |
| 19 | `testGifStream` | `GIF::StreamPlayer`：播放、暫停、跳轉、2 倍速、關循環播到結尾 |
| 20 | `testOffScreen` | 部分畫出螢幕外的圖形和文字：只看到螢幕上的部分，不能繞到另一邊 |

## 只跑部分測試

- **`tests[]`**：全部的測試，不想跑的註解掉。`tests[19]` 需要電腦端，沒開 `stream.py` 時它會等 10 秒，畫面顯示 waiting for stream.py。
- **`selectedTests[]`**：放要單獨跑的，例如 `tests[18]`；然後把 `// #define SELECTED` 打開。

## tests[19]：串流

需要電腦端的 `stream.py`，詳見 [lib/GIF/README.md](../../lib/GIF/README.md#streamplayer)：

```bash
cd lib/GIF
python -B source/stream.py      # pip install pillow pyserial；先關序列監看視窗
```

- `UnoSerial` 用 D0/D1，所以這個檔案不能用 Arduino 的 `Serial`。
- 打開串口會讓板子重置，`stream.py` 先開或後開都可以。
- 板子最多等 10 秒，畫面下方會顯示目前步驟、位置、幀號和狀態。

## 為什麼找得到 lib/ 的標頭檔

這些檔案原本放在 `src/test/`，PlatformIO 找依賴時會跳過那個資料夾，所以 `platformio.ini` 用 `lib_deps` 直接列出 `lib/` 的資料夾，放在 `src/` 的任何子資料夾都能編譯。

# test_2.cpp

## 看結果

```bash
pio device monitor        # platformio.ini 已設定 monitor_speed = 1000000
```

開機時印一次解析度，之後每秒一行：

```
DS18B20 on D2
resolution 12 bits, conversion 750 ms
23.50 C (raw 376, 751 ms)
```

- 括號裡是 raw 值（每格 1/16 °C）和這次量測花的時間。
- 出現 `85.00` 時會加註：這也是感測器剛上電、還沒轉換過的值。
- 錯誤會印出原因，例如 `error: no sensor (check DQ on D2, the pull-up, VCC / GND)`。
- 串口用 `Uno::UnoSerial<1000000>`，所以不能同時用 Arduino 的 `Serial`；打開監看視窗會讓板子重置，開頭那兩行會重印。

# test_3.cpp

M130T 上的彩色圖片。圖片都在 `lib/image/output/`，原始檔在 `lib/image/assets/`。

接線跟最上面一樣（M130T）。OLED 也用 D13 / D11，兩個不能同時接。

## 測試

| `tests[i]` | 函式 | 圖片 | 應該看到 |
|---|---|---|---|
| 0 | `testPalette16` | `sunset_uno_120x80_palette_16`（jpg，scale 2） | 夕陽：紫到橙的天空、黃色太陽、三層暗色的山，色帶明顯；下方顯示畫了幾 ms |
| 1 | `testPalette256` | `sunset_uno_80x53_palette_256`（webp） | 同樣的夕陽，小一點，天空比 0 平滑 |
| 2 | `testPalette565` | `sunset_uno_60x40_palette_565`（png） | 同樣的夕陽，更小，全彩 |
| 3 | `testPalette2` | logo（透明 png）、抖色的夕陽 | 上面白色的 logo，下面點狀的黑白夕陽 |
| 4 | `testClip` | 同一張放在四個角、各超出一半 | 四個角各一塊夕陽，不會繞到另一邊 |
| 5 | `testStream` | `sunset_mac_240x240_palette_565`（heic，`--fill`） | 開著 `stream.py`：整個螢幕是夕陽（約 2 秒畫完），左上角顯示 ms；沒開：紅字 `error 4` |

- 這個檔案只確認過能編譯（M130T 那時沒接）。
- 解碼器已經在電腦上用假螢幕驗證：8 種「顏色 × 編碼」的組合、縮放、裁切，34,838 個像素全部正確。

# assignment_1.cpp：溫度顯示

DS18B20 的溫度、raw 值和螢幕的幀率，每一幀都重畫。

```
Temp: 23.5 \circ C       （拔掉 DQ：Temp: no sensor）
Raw: 376                （拔掉 DQ：Raw: no sensor）
FPS: 85.3
```

- **`\circ` 是 5 個字元**：字型只有 ASCII，沒有 °，所以照字面寫出來。
- **拔掉馬上消失**：感測器不用等 750 ms，每一幀問一次轉換好了沒；拔掉後下一次讀取就是 `ERROR_NO_SENSOR`，同一幀改成 `no sensor`。
- **只有數字變色**：標籤和單位是固定的灰色（`LABEL_LUMA`）。

## 顏色

| 狀態 | 顏色 |
|---|---|
| 跟上一個值一樣 | 灰白 |
| 上升 | 往紅色混 |
| 下降 | 往藍色混 |

- **倍增**：變化量 ÷ 單位 = 步數，紅（藍）的比例每一步加倍，依序是 1/16、1/8、1/4、1/2、全部，5 步就到底。
- **亮度不變**：灰白、紅、藍三個端點都算成同樣的亮度 `VALUE_LUMA`（0.299 R + 0.587 G + 0.114 B），所以中間的混色也一樣亮，只有色調在變。
- **Temp 和 Raw 同時變色**：兩個都用同一次 raw 的變化。Temp 只顯示一位小數，raw 每格 0.0625 °C，所以顏色變了，Temp 的數字不一定變。
- **數值的位置自動對齊**：在 `setup()` 畫標籤，用 `drawTexts` 回傳的結束位置取最大的，換字型也會自動對齊。

| 設定 | 目前 | 說明 |
|---|---|---|
| `TEMP_UNIT` | 0.1 °C | 溫度一步的大小，可以寫小數（編譯時乘 1000 變整數，執行時沒有浮點） |
| `FPS_UNIT` | 1.0 fps | 幀率一步的大小 |
| `VALUE_LUMA` | 0.80 | 數字的亮度；必須大於 0.299（純紅最亮只有這麼亮），否則編譯錯誤 |
| `LABEL_LUMA` | 0.75 | 標籤和單位的亮度 |

# assignment_2.cpp：漣漪

黑色水面上隨機落下水滴，每滴放出 1 ～ 3 圈漣漪，往外擴散、變暗、互相穿過。不用任何輸入，自己一直播。左下角 `No. {n}` 是第幾滴。

## 雨勢：隨機漫步 + Sigmoid

```
walk 每 200 ms 走 −WALK_STEP ～ +WALK_STEP，碰到 ±WALK_LIMIT 彈回
rain = 1 / (1 + e^(−walk / SIGMOID_SCALE))      0 = 和緩，1 = 急雨
```

Sigmoid 兩端平、中間陡，所以大部分時間是穩定的「和緩」或「急雨」，切換時幾秒內就變過去。

| | 和緩（rain = 0） | 急雨（rain = 1） |
|---|---|---|
| 水滴間隔 | 0.9 ～ 1.8 s | 0.07 ～ 0.18 s |
| 漣漪消失的半徑 | 70 px | 28 px |
| 水滴點的半徑 | 4 | 2 |
| 每滴的圈數 | `RING_COUNTS` 隨機：1 圈 3/6、2 圈 2/6、3 圈 1/6 | 越急越常只有 1 圈 |

## 怎麼做到順暢（沒有畫面緩衝）

UNO 只有 2 KB RAM，整個螢幕要 112 KB，所以不能在記憶體裡畫好再送。

1. **只送有變的地方**：圈長大 1 px 時，送一條帶子，從舊的內緣（塗黑）到新的外緣（上色）。中間的部分順便重寫，所以換亮度不用另外重畫；被別的圈擦到的像素，下一次也會補回來。
2. **減少窗口**：每開一個窗口要 32 µs（CASET、RASET、RAMWR）。圓的上下段用橫線、左右段用直線，8 個對稱位置一起送，大約 2.3r 個窗口；螢幕外的部分在送指令前就跳過。
3. **半徑看時間**：半徑由落下後經過的時間算，所以不管同時有幾圈，速度都一樣。
4. **負載預算**：一次更新約 1 + 0.24r ms（實測），一個圈每秒約 `SPEED` ×（1 + 0.12 × 消失半徑）ms。加起來超過 65% 的水滴就不落下，所以每次都只前進 1 px。

| 實測 | 負載 | 最慢的一幀 |
|---|---|---|
| 和緩 | 50 ～ 80% | 約 20 ms |
| 急雨 | 72 ～ 90% | 18 ms |

| 設定 | 目前 | 說明 |
|---|---|---|
| `SPEED` | 24 px/s | 擴散速度 |
| `BORDER` | 3 px | 圈的寬度 |
| `WALK_STEP` / `WALK_LIMIT` | 20 / 40 | 雨勢變化多快；`WALK_STEP` 越小，急雨或和緩持續越久 |
| `SIGMOID_SCALE` | 6 | 越小，切換越突然 |
| `BUDGET` | 65% | 調高會更密，但超過 100% 時圈會一次跳 2 ～ 3 px |

# assignment_3.cpp：兩條射線

兩條射線各從中心附近隨機一點出發，到螢幕邊緣就停下，等一下再換新的點。畫過的線不清除，慢慢疊成兩組線交織的圖案。沒有文字，也不用輸入。

| 項目 | 規則 |
|---|---|
| 斜率 | 第一條永遠是 `M`，第二條永遠是 `N`（數學上的斜率，y 往上；正負號固定） |
| 方向 | 每次換點時隨機挑射向哪一邊，例如 m > 0 時往右上或左下 |
| 起點 | 中心 ±`START_RANGE` 的方塊內 |
| 長出來 | 出發時就算好到邊緣的整條線，每一幀只畫新長出的像素 |
| 速度、等待 | 參數空間裡的兩個點，見下面 |

## 參數空間：重心和距離固定

兩條射線的（速度、等待時間）是參數空間裡的兩個點，永遠以重心（`SPEED_MID`、`WAIT_MID`）對稱，距離固定是 2 × `PARAM_RADIUS`，只有連線的角度會動。所以一條快時另一條就慢，一條等得久時另一條就等得短。

```
角度  = turned + SHAKE_ANGLE · sin(2π · SHAKE_HZ · t)          旋轉 + 震動
轉速  = sign(L) · SPIN_MAX · (1 − e^(−|L| / SPIN_SCALE))       角動量 L 隨機漫步，經過上凸函數
```

- **上凸函數**：`1 − e^(−x)` 在 0 附近很陡、遠處平坦。L 稍微離開 0 就轉得很明確，再大也不會超過 `SPIN_MAX`；L 要穿過 0 才會反轉，所以大部分時間都有清楚的旋轉方向。
- **一開機就在轉**：L 從 ±`WALK_LIMIT` / 2 開始。

| 設定 | 目前 | 說明 |
|---|---|---|
| `M`、`N` | 2.0、1.0 | 兩條的斜率 |
| `START_RANGE` | 125 | ⚠️ 要 ≤ 120：中心到邊緣只有 120 px，超過時起點可能在螢幕外 |
| `SPEED_MID` ± `SPEED_SPAN` | 60 ± 45 px/s | 速度的重心和範圍（`PARAM_RADIUS` = 0.8 時實際 24 ～ 96） |
| `WAIT_MID` ± `WAIT_SPAN` | 1500 ± 1300 ms | 等待的重心和範圍（實際 460 ～ 2540） |
| `PARAM_RADIUS` | 0.8 | 兩點分開多遠：0 = 一樣，1 = 差最多 |
| `SPIN_MAX` / `SPIN_SCALE` | 1.5 rad/s / 2 | 最快轉速、上凸曲線多早變平 |
| `SHAKE_ANGLE` / `SHAKE_HZ` | 0.35 rad / 1.2 Hz | 震動的大小和快慢 |
