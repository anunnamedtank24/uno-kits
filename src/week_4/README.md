# week_4

I2C OLED（SSD1306，0.96 吋 128×64）、OLED 上的黑白圖片（[lib/image](../../lib/image/README.md)；彩色圖片的測試在 [week_3/test_3](../week_3/README.md#test_3cpp)）、開關（[lib/switch](../../lib/switch/README.md)）、蜂鳴器（[lib/buzzer](../../lib/buzzer/README.md)、旋律在 [lib/audio](../../lib/audio/README.md)）的測試，以及四個作業。OLED 函式庫見 [lib/I2C/README.md](../../lib/I2C/README.md)。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python -B src/hide.py week_4/test_1     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `test_1.cpp` | OLED：尺寸、圖形、文字、裁切、速度、datasheet 的每一類指令、黑白圖片（Flash 和串流） | I2C OLED |
| `test_2.cpp` | 開關的狀態，用文字送到串口 | 開關模組 |
| `test_3.cpp` | 有源蜂鳴器：開關、嗶聲、節奏 | 有源蜂鳴器 |
| `test_4.cpp` | 無源蜂鳴器：頻率（實測）、音階、滑音、旋律 | 無源蜂鳴器 |
| `assignment_1.cpp` | 作業一：按開關換下一張照片 | OLED + 開關模組 |
| `assignment_2.cpp` | 作業二：OLED 播動畫 | OLED |
| `assignment_3.cpp` | 作業三：照片每隔 `SLIDE_MS`（現在 1 秒）自動換下一張 | OLED |
| `assignment_4.cpp` | 作業四：大圓吃掉自己生滅的像素點，長到上限就縮小並噴出點 | OLED |

## 接線

| OLED | UNO |
|---|---|
| GND | GND |
| VCC | 5V（或 3.3V） |
| SCL | D13 |
| SDA | D11 |

- **軟體 I2C**：D13 / D11 不是 UNO 的硬體 I2C 腳（那是 A4 / A5），所以用 `Uno::UnoI2C<13, 11>` 以軟體產生時序。換腳只要改這兩個數字。
- **實測**：位址 0x3C 有回應；狀態位元組上電時是 `0x43`（顯示關），初始化後是 `0x03`（顯示開）。低 4 位元是 3，符合 SSD1306；SH1106 會是 8。
- **D13 的板載 LED 會跟著閃**：它接在 SCL 上，不影響傳輸。
- **不能同時接 M130T**：D13 / D11 也是 M130T 的 SPI 腳。

開關模組（`test_2`、`assignment_1`）：

| 開關 | UNO |
|---|---|
| GND | GND |
| VCC | 5V |
| OUT | D2 |

蜂鳴器（`test_3`、`test_4`），兩隻腳：

| 蜂鳴器 | UNO |
|---|---|
| 有源的 +（長腳） | D8 |
| 無源的 + | D9（中間串 100 Ω 比較安全，見 [lib/buzzer](../../lib/buzzer/README.md#接線)） |
| - | GND |

- **分辨**：接 5V 會一直響的是有源；不響的是無源。
- **D9**：無源用 Timer1 直接翻轉 D9，不佔 CPU。D9 / D10 不能同時拿來 PWM。

# test_1.cpp

每個測試先顯示 `test {n}` 0.8 秒，再畫自己的畫面，停 2 秒（`HOLD_MS`）後換下一個，一直循環。每個測試上方的註解寫著「應該看到什麼」。

文字用 23 px 的字型（每字 8×23，一行 16 個字、兩行）。12 px 的字只有 4 px 寬，太粗糙，所以不用。

- **初始化失敗**：`init()` 回傳 false（0x3C 沒有回應）時程式停住，螢幕一片黑。這時檢查接線和電源。
- **先畫在 RAM**：前 6 個測試畫在板子 RAM 裡的畫面，再用 `display()` 送出。
- **直接下指令**：後面的測試直接用控制器的指令（對比、捲動、淡出等），不改 RAM 裡的畫面。

| `tests[i]` | 函式 | 測什麼 | 應該看到 |
|---|---|---|---|
| 0 | `testBorder` | 尺寸、方向、初始化 | 外框貼齊四邊、四角各一個 6×6 方塊、中間十字、`128 x 64` 正向（不是鏡像），排針在上方 |
| 1 | `testShapes` | `graphics` 的圖形 | 左邊放射狀的線；右邊空心 / 實心的矩形、圓、三角形 |
| 2 | `testText` | 23 px 字型 | `Hello, OLED!` / `AaBbCc 123`，再換成 `int -1234` / `f 3.14` |
| 3 | `testOffColor` | 顏色 0 畫在亮的上面 | 上半部白底黑字 `black/white`，下半部黑底白字 `white/black` |
| 4 | `testClip` | 裁切 | 四角各四分之一個圓、左邊半個矩形、右邊 `cut off` 被切掉，不會繞到另一邊 |
| 5 | `testSpeed` | 部分更新的速度 | 球彈 4 秒，接著顯示每秒幀數和整張畫面要幾 ms（實測約 64 ms） |
| 6 | `testContrast` | 81h 對比 | 同一個畫面從暗到亮再到暗，顯示目前的值 |
| 7 | `testInverse` | A6h / A7h、A4h / A5h | 反相 1 秒後恢復；全亮 1 秒後恢復 |
| 8 | `testStatus` | AEh / AFh、讀狀態 | 關 1 秒，再顯示兩個狀態位元組：關 `0x43`（bit 6 = 1）、開 `0x03` |
| 9 | `testScroll` | 26h / 27h / 29h、2Eh / 2Fh、A3h | 往右捲 2 秒、往左 2 秒、斜捲 2 秒，最後回到原位 |
| 10 | `testStartLine` | 40h ～ 7Fh | 畫面往上滾一整圈（64 列），回到原位 |
| 11 | `testMirror` | A0h / A1h、C0h / C8h | 左右鏡像 → 上下鏡像 → 上下顛倒 → 正常 |
| 12 | `testFadeBlink` | 23h | 閃爍 4 秒，再淡出（保持暗），最後恢復 |
| 13 | `testZoom` | D6h | `zoom` 放大兩倍 2 秒（下半部的 `hidden` 看不到），再恢復 |
| 14 | `testFlashImage` | `image::FlashImage`、黑白圖片 | 圓環裡一道閃電加三條橫槓（logo）；再來是點狀的夕陽和山；最後夕陽一半超出右邊（被裁切） |
| 15 | `testStreamImage` | `image::StreamImage` | 要先開 `lib/image/source/stream.py`：同一個 logo，右下角顯示傳輸幾 ms；沒開時 3 秒後顯示 `no answer` |

## 只跑部分測試

- **`tests[]`**：全部的測試，不想跑的註解掉。
- **`selectedTests[]`**：放要單獨跑的（現在是 `tests[14]`、`tests[15]`）；然後把 `// #define SELECTED` 打開。

## tests[15]：串流

```bash
cd lib/image
python -B source/stream.py     # 先開它，再燒錄；video 的 stream.py 要先關掉
```

實測（OLED、logo 128×64 黑白、369 bytes）：`stream.py` 印出 `369 bytes in 0.07 s`。換成 240×240 全彩的夕陽（19 KB）也完整收到，1.65 s。

## 看到的不對時

| 現象 | 可能的原因 |
|---|---|
| 一片黑 | `init()` 失敗：VCC / GND、SCL / SDA 接反，或位址是 0x3D（改成 `Screen screen(0x3D);`） |
| 只有上半部、每隔一列 | 其實是 128×32 的面板：改用 `module/OLED_128x32.h` 的 `OLED128x32` |
| 左右或上下鏡像 | 模組裝反了：`init(true)` 轉 180° |
| 右邊有 2 欄雜點、整體偏移 | 控制器其實是 SH1106（132 欄），這個函式庫不支援 |
| 偶爾亂掉 | 線太長或上拉太弱：降低速度 `Uno::UnoI2C<13, 11, true, 100>` |

# test_2.cpp

開關模組的狀態，用文字送到串口：

```bash
pio device monitor        # platformio.ini 已設定 monitor_speed = 1000000
```

```
switch on D2, ACTIVE_HIGH = true
starts off, level LOW
      level LOW, off for 1000 ms                      每秒一行：腳位的電位、目前狀態維持多久
on  (level HIGH), presses 1, was off for 2300 ms      一改變就印：新狀態、電位、第幾次按、上一個狀態多久
off (level LOW), presses 1, was on for 180 ms
```

- **實測**：D2 沒按時是 `level LOW`、`off`。按下的部分我沒辦法測，請你按按看。
- **按著卻顯示 `off`**：你的模組是按下時輸出 LOW，把檔案開頭的 `ACTIVE_HIGH` 改成 `false`。`assignment_1` 也要改。
- **串口**：用 `Uno::UnoSerial<1000000>`，所以不能同時用 Arduino 的 `Serial`。

# test_3.cpp

有源蜂鳴器（+ 接 D8）。每個測試跑完停 1 秒，全部跑完再從頭來；串口印出現在是哪個測試、應該聽到什麼。

| 測試 | 應該聽到 |
|---|---|
| 0 `testOnOff` | 一直響 1 秒（`tone()` / `stop()`） |
| 1 `testBeep` | 嗶一聲 100 ms |
| 2 `testBeeps` | 三聲，再八聲很快的（要數得出八聲，不是一聲長的） |
| 3 `testSos` | 摩斯碼 SOS：三短、三長、三短 |
| 4 `testRhythm` | 小星星的節奏，全部同一個音高（有源只有一個音高） |
| 5 `testNoWait` | 嗶 500 ms，同時數 `loop` 跑了幾圈 |

- **實測**：`testNoWait` 印出 58758 圈，播放器不會讓程式停下來等。其他測試我聽不到，請你聽聽看。
- **一直不響**：+、- 接反，或其實是無源的（換跑 `test_4`）。
- **該安靜時在響**：+ 接 5V、- 接腳位的接法，把 `ACTIVE_HIGH` 改成 `false`。

# test_4.cpp

無源蜂鳴器（+ 接 D9）。跟 `test_3` 一樣一個接一個跑；頻率會從腳位讀回來量，所以**沒接蜂鳴器也能測**。

| 測試 | 應該聽到 |
|---|---|
| 0 `testA4` | A4 = 440 Hz，1 秒 |
| 1 `testRange` | 50 Hz ～ 15 kHz 十個頻率由低到高；大約 2 ～ 4 kHz 最大聲（蜂鳴器的共振） |
| 2 `testScale` | C 大調音階上去再下來 |
| 3 `testSlide` | 警笛：200 → 3000 Hz 再下來，各 1.5 秒 |
| 4 `testBeeps` | 2 kHz 嗶三聲 |
| 5 `testCues` | 開機（往上）、成功（短、往上）、錯誤（低、往下） |
| 6 `testMelodies` | 小星星、歡樂頌（連續相同的音要分得出來） |

```
passive buzzer on D9 (Timer1 output)
test 0: A4 1 s, 440 Hz: plays 440, measured 440
test 1: range
  50 Hz: plays 50, measured 50
  ...
  8000 Hz: plays 8000, measured 8000
  15000 Hz: plays 15009, measured 15010
```

- **實測**：D9（Timer1 直接翻轉）和 D7（中斷翻轉，把 `BUZZER_PIN` 改成 7）兩種，十個頻率的實測都跟 `realHz()` 一致；整輪七個測試都跑完、從頭再來。
- **15009**：計時器的 tick 是整數，15 kHz 最接近的是 15009 Hz；`realHz()` 就是告訴你實際值。
- **換腳**：改 `BUZZER_PIN`。D9 / D10 不佔 CPU，其他腳用中斷。

# 作業

照片和動畫都是黑白的，因為 OLED 每個點只有亮或暗。

| | 照片 / 動畫 | 轉換指令（在 `lib/image` 或 `lib/video` 裡） |
|---|---|---|
| 夕陽 | `sunset_uno_128x64_palette_2` | `python -B source/convert.py assets/sunset.heic output --target uno --fill 128x64 --palette 2 --dither` |
| 月亮 | `moon_uno_128x64_palette_2` | `... assets/moon.jpg ... --palette 2 --dither` |
| 城市 | `city_uno_128x64_palette_2` | `... assets/city.png ... --palette 2 --threshold 100` |
| logo | `logo_uno_mac_128x64_palette_2` | `... assets/logo.png --target uno mac --fit 128x64 --palette 2` |
| 動畫 | `orbit_uno_64x64_palette_16_mono` | `python -B source/convert.py assets/orbit.gif output --target uno --fit 64x64 --mono --keyframe 8` |

換成自己的照片：放進 `lib/image/assets/`（jpg、png、webp、heic 都可以），用上面的指令轉換，再把作業裡 `photos[]` 的名字換掉。

## assignment_1.cpp：按開關換照片

- 開機顯示第一張；每按一次（`wasPressed()`）換下一張，最後一張之後回到第一張。
- 照片畫進 OLED 的 RAM 畫面，再 `display()` 送出改過的部分。
- Flash 7.2 KB、RAM 1087 B（OLED 的畫面就佔 1 KB）。

## assignment_2.cpp：OLED 播動畫

- `video::FlashPlayer` 直接畫在 OLED 上：OLED 的 `Frame` 也提供 `setWindow` / `writeData16`，畫進 RAM 的畫面；每畫完一幀呼叫 `display()`。
- 64×64 的 orbit 放在中間（x = 32），一直循環。
- 實測 16.5 ～ 17 fps（動畫本身 16.7 fps），沒有跳幀，最慢一幀（I 幀）66 ms。
- Flash 4.6 KB、RAM 1369 B。

## assignment_3.cpp：自動換照片

- 跟作業一一樣的四張照片，每隔 `SLIDE_MS`（現在 1000 ms）換下一張。
- 時間從上一次換照片的時間點往後算，所以畫圖花的時間不會讓間隔越來越長。
- Flash 7.0 KB、RAM 1073 B。


## assignment_4.cpp：大圓和生滅的像素點

- **大圓**：方向一直有小小的隨機變動，同時轉向最新吃到的那個點（每秒最多轉 `STEER` 弧度），到了就只剩隨機變動；碰到邊會反彈。
  - 點在邊上時，只走到圓還放得下的地方。
  - 碰到的點會消失，圓的面積加 `AREA_PER_DOT`，所以面積跟吃掉的點數成正比。
  - 半徑到 `R_MAX`（24 px）時縮回 `R_START`，並往四周噴出吃掉的一半點數（最多 `BURST_MAX` 24 個）。
  - 噴出去的點減速，0.9 秒後落地，變成一般的點。
- **像素點**：照你給的規則生滅。
  - 每幀隨機挑 `UPDATES_PER_FRAME` 個點，各算一次 w、ρ、P_die、P_rep、P_sex，再擲骰子。
  - 每個點有自己的壽命：活到年齡 t 的機率 S(t) = e^(−(t/τ)^β)（Weibull），超過 `LIFE_MAX` 必死。
  - 少於 `MIN_DOTS` 時隨機補一個，所以不會滅絕。

### 壽命

- **不用查表**：出生時就抽好壽命 L = τ · (−ln U)^(1/β)，U 是 (0, 1] 的均勻亂數。這樣抽出來的 L 剛好滿足 P(L > t) = e^(−(t/τ)^β)，跟逐時查表擲骰子的結果一樣，但每個點只算一次（`powf` + `logf`，出生時）。
- **之後只倒數**：每 `LIFE_TICK_MS`（100 ms）所有點減 1，到 0 就死。一個點 1 byte，最多 255 格（25.5 秒）。
- **`LIFE_MAX`** = 200 格（20 秒）：S(20 s) = e^−4 ≈ 2 %，這些點一律在 20 秒死。
- **β 的意思**：β = 1 時任何年齡死的機率都一樣（無記憶）；β > 1 時越老越容易死；β < 1 時年輕的比較容易死。
- **跟 P_die 的關係**：原本的 `P_DIE_0`（孤單也會死）就是「自然死亡」，現在交給壽命，所以設成 0。擁擠造成的死亡 C_die · ρ² 照舊。
- **電腦上先模擬**（沒有大圓）：τ = 10 s、β = 2 時點數穩定在 130 ～ 150 個，跟原本 P_DIE_0 = 0.02 差不多。

### 規則的問題和做法

| 問題 | 做法 |
|---|---|
| `if P_rep_i:` 是機率，不是條件 | 用亂數：`random < P` 才生 |
| w = 0（沒有鄰居）時，ρ 和 P_sex 的公式會除以 0 | 改寫成 ρ = w / (w + ρ₀)、P_sex = ρ / (ρ + P_sex₀)：數學上一樣，w = 0 時是 0 |
| \hat v 沒有定義 | 有性：指向 K 個最近鄰居裡隨機一個（伴侶）；無性：隨機方向 |
| 全部同時更新或依序更新 | 依序會偏向前面的點；全部同時要多存一份決定。改成每次隨機挑一個點，馬上套用（非同步更新），期望值相同 |
| 小孩生得比平衡間距近，會一直擠在同一團裡，那團太擠就全部死光 | 電腦上先模擬：δ_asex = 3 時族群剩約 5 個；改成 8 px 才穩定在 60 ～ 150 個，最近的點平均相隔 3 ～ 4 px |
| C_rep ≤ P_die₀ 時，就算很稀疏也是死得比生得快 | 註解寫明 `C_REP` 要大於 `P_DIE_0` |
| 一個像素可能有好幾個點；點也可能生在螢幕外 | 一個像素最多一個點，生在已經有點的位置、螢幕外，或超過 `MAX_DOTS` 時就不生 |

「像素點密度大於像素單位」我理解成點的間距要大於一個像素，不然看起來會黏成一塊，所以用了上面的 δ 和「一像素一點」。

### 參數

| 參數 | 值 | 意思 |
|---|---|---|
| `K` | 4 | 看幾個最近的點 |
| `RHO_0` | 0.65 | w 等於它時 ρ = 0.5；大 = 點比較多 |
| `P_DIE_0`、`C_DIE` | 0、0.1 | 死亡：基本機率（0：交給壽命）、擁擠時加多少 |
| `TAU`、`BETA` | 10 s、2 | 壽命：活到 τ 的機率是 e^−1 = 37 %（平均約 8.9 秒）；β 見[壽命](#壽命) |
| `LIFE_TICK_MS`、`LIFE_MAX` | 100 ms、200 | 壽命的單位、上限（20 秒，最多 255） |
| `C_REP` | 0.4 | 生育：孤單時的機率 |
| `P_SEX_0` | 0.3 | ρ 等於它時，一半的小孩是有性生殖 |
| `DELTA_SEX`、`DELTA_ASEX` | 5、10 px | 小孩離父母多遠 |
| `MAX_DOTS` | 150 | 每個點 3 bytes（x、y、剩下的壽命） |
| `UPDATES_PER_FRAME` | 12 | 每幀檢查幾個點：越大變化越快、幀率越低 |
| `R_START`、`R_MAX` | 5、24 px | 大圓的大小範圍 |
| `AREA_PER_DOT` | 8 px² | 吃一個點長多少面積：從 5 長到 24 要約 216 個點（`eaten` 是 uint8，要 < 255） |
| `GROW_EASE`、`SHRINK_EASE` | 0.08、0.5 | 畫出來的半徑每幀往實際大小走多少：變大時慢慢跟上；爆發縮小時快，約 5 幀（0.4 秒）就從 24 縮回 5 |
| `STEER` | 6 rad/s | 轉向最新吃到的點有多快：3 的時候，點在旁邊會繞著它轉、永遠到不了（模擬：6 最久約 2 秒到） |

### 實測（RHO_0 = 0.55 時，跑 2 分鐘）

- 約 12.5 fps：整張畫面每幀重畫，送出就要 64 ms；最慢一幀 98 ms（爆發的那幀）。
- 點數在 4 ～ 57 之間起伏；大圓大約每 45 秒長到上限一次。
- 點偏少，所以 `RHO_0` 調成 0.65（模擬裡點數大約多一倍）。
- **RAM**：1659 B（多了追的目標 9 B、每個點的壽命 150 B）。噴出去的點不存位置和速度，只存方向和初速（3 bytes），位置用公式算，省了 312 B。
