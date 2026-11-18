# week_4_SSD1306_switch_buzzer_audio

I2C OLED（SSD1306，0.96 吋 128×64）、OLED 上的黑白圖片（[lib/image](../../lib/image/README.md)；彩色圖片的測試在 [week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/3_pictures](../week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md#tests3_picturescpp彩色圖片)）、開關（[lib/switch](../../lib/switch/README.md)）、蜂鳴器（[lib/buzzer](../../lib/buzzer/README.md)、旋律在 [lib/audio](../../lib/audio/README.md)）的測試，以及四個作業。OLED 函式庫見 [lib/SSD1306/README.md](../../lib/SSD1306/README.md)。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python src/hide.py week_4_SSD1306_switch_buzzer_audio/tests/1_oled     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `tests/1_oled.cpp` | OLED：尺寸、圖形、文字、裁切、速度、datasheet 的每一類指令、黑白圖片（Flash 和串流） | I2C OLED |
| `tests/2_switch.cpp` | 開關的狀態，用文字送到串口 | 開關模組 |
| `tests/3_active_buzzer.cpp` | 有源蜂鳴器：開關、嗶聲、節奏 | 有源蜂鳴器 |
| `tests/4_passive_buzzer.cpp` | 無源蜂鳴器：頻率（實測）、音階、滑音、旋律 | 無源蜂鳴器 |
| `tests/5_files.cpp` | 直接用檔案：`FONT`、`PICTURE`、`ANIMATION`、`MELODY`（編譯前自動轉） | OLED + 無源蜂鳴器 |
| `tests/6_clicks.cpp` | 一個按鈕的單擊、雙擊、多擊、長按（`switches::Clicks`），用 LED 效果（week_2）表示 | 按鈕 + LED |
| `tests/7_play_file.cpp` | 電腦上任何聲音檔（`MELODY("路徑")`），編譯時轉成音符，蜂鳴器播 | 無源蜂鳴器 |
| `tests/8_graphics.cpp` | graphics 的每個函式（圖形、文字）在 OLED 上：點、線、方形、三角形、圓、邊框寬度、邊緣、裁切、每種數值型態、小數、陣列、換行 | I2C OLED |
| `assignments/1_photo_switch.cpp` | 作業一：按開關換下一張照片 | OLED + 開關模組 |
| `assignments/2_animation.cpp` | 作業二：OLED 播動畫 | OLED |
| `assignments/3_photo_music.cpp` | 作業三：依序顯示照片，每張同時播一首不同的音樂 | OLED + 無源蜂鳴器 |
| `assignments/4_circle_dots.cpp` | 作業四：大圓吃掉自己生滅的像素點，長到上限就縮小並噴出點 | OLED |

## 接線

| OLED | UNO |
|---|---|
| GND | GND |
| VCC | 5V（或 3.3V） |
| SCL | D13 |
| SDA | D11 |

- **軟體 I2C**：D13 / D11 不是 UNO 的硬體 I2C 腳（那是 A4 / A5），所以用 `Uno::UnoI2C<SCL_PIN, SDA_PIN>` 以軟體產生時序，任何兩隻腳都可以。換腳只要改設定的 `SCL_PIN`、`SDA_PIN`。
- **實測**：位址 0x3C 有回應；狀態位元組上電時是 `0x43`（顯示關），初始化後是 `0x03`（顯示開）。低 4 位元是 3，符合 SSD1306；SH1106 會是 8。
- **D13 的板載 LED 會跟著閃**：它接在 SCL 上，不影響傳輸。
- **不能同時接 M130T**：D13 / D11 也是 M130T 的 SPI 腳。

開關模組（`tests/2_switch`、`assignments/1_photo_switch`）：

| 開關 | UNO |
|---|---|
| GND | GND |
| VCC | 5V |
| OUT | D2 |

蜂鳴器（`tests/3_active_buzzer`、`tests/4_passive_buzzer`、`tests/5_files`、`tests/7_play_file`、`assignments/3_photo_music`），兩隻腳：

| 蜂鳴器 | UNO |
|---|---|
| 有源的 +（長腳） | D8 |
| 無源的 + | D9（中間串 100 Ω 比較安全，見 [lib/buzzer](../../lib/buzzer/README.md#接線)） |
| - | GND |

- **分辨**：接 5V 會一直響的是有源；不響的是無源。
- **D9**：無源用 Timer1 直接翻轉 D9，不佔 CPU。D9 / D10 不能同時拿來 PWM。

按鈕和 LED（`tests/6_clicks`）：

| 零件 | 腳 | UNO |
|---|---|---|
| 按鈕（2 隻腳） | 一隻、另一隻 | **D2**（用 UNO 的上拉）、GND |
| LED | 長腳、短腳 | 220 Ω → **D6**、GND |

# tests/1_oled.cpp：OLED 的每個功能

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
| 15 | `testStreamImage` | `image::StreamImage` | 要先開 `lib/stream/source/stream.py`：同一個 logo，右下角顯示傳輸幾 ms；沒開時 3 秒後顯示 `no answer` |

## 只跑部分測試

- **設定的 `ONLY_TESTS`**：0 = 全部輪流跑；要只跑某幾個，就把它們的位元設起來，例如 `(1UL << 14) | (1UL << 15)` 只跑兩個圖片測試。

## tests[15]：串流

```bash
python lib/stream/source/stream.py     # 先開它，再燒錄；圖片、動畫、聲音都是它
```

實測（OLED、logo 128×64 黑白、369 bytes）：`stream.py` 印出 `369 bytes in 0.07 s`。換成 240×240 全彩的夕陽（19 KB）也完整收到，1.65 s。

## 看到的不對時

| 現象 | 可能的原因 |
|---|---|
| 一片黑 | `init()` 失敗：VCC / GND、SCL / SDA 接反，或位址是 0x3D（改成 `Screen screen(0x3D);`） |
| 只有上半部、每隔一列 | 其實是 128×32 的面板：改用 `SSD1306::OLED128x32`（`SSD1306.h` 已經包含） |
| 左右或上下鏡像 | 模組裝反了：設定的 `FLIP = true`（`init(true)`）轉 180° |
| 右邊有 2 欄雜點、整體偏移 | 控制器其實是 SH1106（132 欄），這個函式庫不支援 |
| 偶爾亂掉 | 線太長或上拉太弱：降低速度，設定的 `I2C_KHZ = 100` |

# tests/2_switch.cpp：開關

開關模組的狀態，用文字送到串口：

```bash
pio device monitor        # platformio.ini 已設定 monitor_speed = 1000000
```

```
switch on D2, SWITCH_ACTIVE_HIGH = true
starts off, level LOW
      level LOW, off for 1000 ms                      每秒一行：腳位的電位、目前狀態維持多久
on  (level HIGH), presses 1, was off for 2300 ms      一改變就印：新狀態、電位、第幾次按、上一個狀態多久
off (level LOW), presses 1, was on for 180 ms
```

- **實測**：D2 沒按時是 `level LOW`、`off`。按下的部分我沒辦法測，請你按按看。
- **按著卻顯示 `off`**：你的模組是按下時輸出 LOW，把設定的 `SWITCH_ACTIVE_HIGH` 改成 `false`。`assignments/1_photo_switch` 也要改。
- **串口**：用 `Uno::UnoSerial<BAUD>`，所以不能同時用 Arduino 的 `Serial`。

# tests/3_active_buzzer.cpp：有源蜂鳴器

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
- **一直不響**：+、- 接反，或其實是無源的（換跑 `tests/4_passive_buzzer`）。
- **該安靜時在響**：可能是三隻腳的模組（VCC、I/O、GND，LOW 才響）。I/O 接 `BUZZER_PIN`（D8）、VCC 接 5V，把 `Beeper` 改成 `buzzer::ActiveBuzzer3Pins<BuzzerPin>`（見 [lib/buzzer](../../lib/buzzer/README.md#兩隻腳和三隻腳)）。

# tests/4_passive_buzzer.cpp：無源蜂鳴器

無源蜂鳴器（+ 接 D9）。跟 `tests/3_active_buzzer` 一樣一個接一個跑；頻率會從腳位讀回來量，所以**沒接蜂鳴器也能測**。

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

照片和動畫都是黑白的，因為 OLED 每個點只有亮或暗。

| | 照片 / 動畫 | 轉換指令（在 `lib/image` 或 `lib/video` 裡） |
|---|---|---|
| 夕陽 | `sunset_flash_128x64_palette_2` | `python source/convert.py assets/sunset.heic output --target flash --fill 128x64 --palette 2 --dither` |
| 月亮 | `moon_flash_128x64_palette_2` | `... assets/moon.jpg ... --palette 2 --dither` |
| 城市 | `city_flash_128x64_palette_2` | `... assets/city.png ... --palette 2 --threshold 100` |
| logo | `logo_flash_stream_128x64_palette_2` | `... assets/logo.png --target flash stream --fit 128x64 --palette 2` |
| 動畫 | `orbit_flash_64x64_palette_16_mono` | `python source/convert.py assets/orbit.gif output --target flash --fit 64x64 --mono --keyframe 8` |

換成自己的照片：放進 `lib/image/assets/`（jpg、png、webp、heic 都可以），用上面的指令轉換，再把作業裡 `photos[]` 的名字換掉。

# tests/5_files.cpp：直接用檔案

字型、圖片、動畫、聲音直接寫檔案路徑，編譯前 `tools/assets.py` 自動轉換（第一次編譯每個要幾秒，之後用快取）：

```cpp
using Title = FONT("/System/Library/Fonts/Supplemental/Arial Bold.ttf", "--height 16");   // 絕對路徑
using Moon  = PICTURE("../../../lib/image/assets/moon.jpg", "--fit 64x64 --palette 2 --dither"); // 相對於這個檔案
using Orbit = ANIMATION("orbit.gif", "--width 64 --mono");                                 // 只有檔名：lib/video/assets/
using Chime = MELODY("success.m4a");                                                       // 沒有選項：預設
```

接線：OLED（SCL D13、SDA D11）、無源蜂鳴器（+ 串 100 Ω 接 D9）。

1. 系統字型 Arial Bold 16 的文字，同時播 `success.m4a`。
2. 月亮的照片（64 × 64，抖動）。
3. 軌道動畫 3 秒。

換成自己的檔案：改路徑和選項字串，再 `pio run -t upload`。

# tests/6_clicks.cpp：按鈕手勢

一個按鈕當好幾個按鍵用（`switches::Clicks`），每種手勢換一種 LED 效果（`LED::Effect`，見 week_2_LED），串口同時印出手勢：

| 按鈕 | LED |
|---|---|
| 按一下 | 閃 `BLINK_TIMES` 下 |
| 按兩下 | 呼吸 |
| 按三下以上 | 心跳 |
| 按住 | 漸亮，繼續按住每 `REPEAT_MS` 再亮一點 |
| 放開（長按後） | 漸暗 |

- **單擊會晚一點**：放開後要等 `DOUBLE_MS`，確定沒有第二下才算單擊。
- **調手感**：`DOUBLE_MS`（雙擊的間隔）、`LONG_MS`（多久算長按）、`REPEAT_MS`（按住時多久重複一次）。

# tests/7_play_file.cpp：播任意音檔

`MELODY("/任何/路徑/歌.aiff")`：編譯前 `tools/assets.py` 找出每個時間最大聲的音高，變成音符給蜂鳴器播（一次只能一個音，和弦會變成主旋律）。

| 選項 | 效果 |
|---|---|
| （沒有） | 中等品質：例如 4 分半的戰鬥曲 → 283 個音、2 KB |
| `"--quality high"` | 抓更多音：同一首 619 個音、4.3 KB |
| `"--start 30 --duration 20"` | 只要其中 20 秒 |
| `"--transpose -12"` | 低一個八度 |
| `"--sound pcm --quality low"` | 真正的聲音（取樣），但 Flash 只放得下幾秒；整首要從電腦串流（`--target stream`） |

- 管弦樂、很多樂器一起的曲子，抓出來的旋律會比較零碎；單一樂器、人聲哼唱的效果最好。
- 先聽聽看：`python lib/audio/source/convert.py 檔案 /tmp/x --preview /tmp/x.wav` 會寫一個「板子會播出來的樣子」的 wav。

# tests/8_graphics.cpp：graphics 的每個函式

跟 `week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen` 一樣的圖形和文字測試，改成 128 × 64 黑白：`ON`（= `ScreenColor::ON`）是亮的點、`OFF` 是暗的。字用 4 × 12 的小字（一行 32 個字、5 行），標題用 23 px 的字。

| 測試 | 測什麼 | 應該看到 |
|---|---|---|
| 0 | `drawPoint` | 每 8 px 一個點的格子 |
| 1 | `drawHLine…`、`drawVLine…`（Size / Points / Center） | 3 條一樣長的橫線、3 條一樣長的直線 |
| 2 | `drawLinePoints`、`drawLineAngle`、`drawLineDelta` | 左邊扇形、中間每 30° 一條的星（最長的是 0°，朝右）、右邊 X |
| 3 | 方形三種寫法 | 上排空心、下排實心，同一排一樣大 |
| 4 | 三角形三種寫法 | 上排空心、下排實心 |
| 5 | 圓兩種寫法 | 一樣大的圓；右邊實心圓上的暗點是給的那一點 |
| 6 | 空心圖形的邊框寬度 1、4、20 | 邊框往內長；比一半寬就變實心 |
| 7 | 剛好碰到邊緣的圖形 | 都完整，沒有被切掉 |
| 8 | 字型的每個字（用回傳的 x 接著畫） | `' '` ～ `'~'` 三行，沒有空隙、沒有重疊 |
| 9 | 字串、字元、每種整數 | 註解寫的每一行 |
| 10 | `float`、`double`、`setDecimals` | `3.14`、`nan inf -inf`、`ovf`、小數 0 位和 4 位 |
| 11 | 陣列、`setSeparator` | 預設沒有分隔，`, `、`\|` |
| 12 | 字型裡沒有的字 | 換成 `?`；大數字字型沒有 `:` |
| 13 | 透明背景、回傳的 x、空的呼叫、`fontHeight` | 亮條上的暗字；`empty ok fontHeight=12` |
| 14 | `setAutoLineBreak`、`setLineSpacing` | `\n` 換行；太長的句子在右邊折行 |
| 15 | 超出螢幕的圖形 | 只畫螢幕上的部分，不會繞到另一邊 |

- **畫完要 `display()`**：graphics 只改 RAM 裡的畫面，每個測試最後送一次。
- **只跑某幾個**：`ONLY_TESTS`，例如 `(1UL << 2) | (1UL << 9)`。
- **RAM 用了約 75 %**：畫面本身就佔 1 KB（見 [lib/SSD1306](../../lib/SSD1306/README.md)）。

# assignments/1_photo_switch.cpp：按開關換照片

- 開機顯示第一張；每按一次（`wasPressed()`）換下一張，最後一張之後回到第一張。
- 照片畫進 OLED 的 RAM 畫面，再 `display()` 送出改過的部分。
- Flash 7.2 KB、RAM 1087 B（OLED 的畫面就佔 1 KB）。

# assignments/2_animation.cpp：OLED 播動畫

- `video::FlashPlayer` 直接畫在 OLED 上：OLED 的 `Frame` 也提供 `setWindow` / `writePixel`，畫進 RAM 的畫面；每畫完一幀呼叫 `display()`。
- 64×64 的 orbit 放在中間（x = 32），一直循環。
- 實測 16.5 ～ 17 fps（動畫本身 16.7 fps），沒有跳幀，最慢一幀（I 幀）66 ms。
- Flash 4.6 KB、RAM 1369 B。

# assignments/3_photo_music.cpp：照片配音樂

接線：OLED（D13、D11）+ 無源蜂鳴器（+ 串 100 Ω 接 **D9**、- 接 GND）。

| 順序 | 照片 | 音樂 | 長度 |
|---|---|---|---|
| 1 | 夕陽 | 小星星 | 6.4 s |
| 2 | 月亮 | 歡樂頌 | 6.9 s |
| 3 | 城市 | C 大調音階 | 4.3 s |
| 4 | logo | 開機音效 | 0.5 s（照片至少留 `MIN_SLIDE_MS` 2 秒） |

- **換的時機**：音樂開始時換照片；音樂播完（而且照片已經顯示至少 2 秒）就停 `PAUSE_MS`（0.8 秒），再換下一組；最後一組之後回到第一組。
- **同時播放**：音樂由 `audio::Player` 用 Timer1 播，不會等；OLED 的 `display()` 只在換照片時花約 64 ms，音樂照常（方波由計時器產生），只是那一刻換音會晚一點。
- **換音樂或照片**：改 `photos[]`、`songs[]` 兩個陣列，兩個要一樣長（不一樣時編譯錯誤）。音樂用 `lib/audio/source/convert.py` 轉，照片用 `lib/image/source/convert.py`。
- **音量**：設定的 `VOLUME`，0 ～ 255（255 最大聲、128 小 6 dB、64 小 12 dB），見 [lib/buzzer 的音量](../../lib/buzzer/README.md#音量)。
- **三隻腳的無源模組**：`Speaker` 改成 `buzzer::PassiveBuzzer3Pins<Uno::UnoTone<BUZZER_PIN, true>>`。
- Flash 8.6 KB、RAM 1164 B。已經燒進板子，畫面和聲音要你看、聽確認。


# assignments/4_circle_dots.cpp：大圓和生滅的像素點

- **大圓**：方向一直有小小的隨機變動，同時轉向最新吃到的那個點（每秒最多轉 `STEER` 弧度），到了就只剩隨機變動；碰到邊會反彈。
  - 點在邊上時，只走到圓還放得下的地方。
  - 碰到的點會消失，圓的面積加 `AREA_PER_DOT`，所以面積跟吃掉的點數成正比。
  - 半徑到 `R_MAX`（24 px）時縮回 `R_START`，並往四周噴出吃掉的一半點數（最多 `BURST_MAX` 24 個）。
  - 噴出去的點減速，0.9 秒後落地，變成一般的點。
- **像素點**：照你給的規則生滅。
  - 每幀隨機挑 `UPDATES_PER_FRAME` 個點，各算一次 w、ρ、P_die、P_rep、P_sex，再擲骰子。
  - 每個點有自己的壽命：活到年齡 t 的機率 S(t) = e^(−(t/τ)^β)（Weibull），超過 `LIFE_MAX` 必死。
  - 少於 `MIN_DOTS` 時隨機補一個，所以不會滅絕。

## 壽命

- **不用查表**：出生時就抽好壽命 L = τ · (−ln U)^(1/β)，U 是 (0, 1] 的均勻亂數。這樣抽出來的 L 剛好滿足 P(L > t) = e^(−(t/τ)^β)，跟逐時查表擲骰子的結果一樣，但每個點只算一次（`powf` + `logf`，出生時）。
- **之後只倒數**：每 `LIFE_TICK_MS`（100 ms）所有點減 1，到 0 就死。一個點 1 byte，最多 255 格（25.5 秒）。
- **`LIFE_MAX`** = 200 格（20 秒）：S(20 s) = e^−4 ≈ 2 %，這些點一律在 20 秒死。
- **β 的意思**：β = 1 時任何年齡死的機率都一樣（無記憶）；β > 1 時越老越容易死；β < 1 時年輕的比較容易死。
- **跟 P_die 的關係**：原本的 `P_DIE_0`（孤單也會死）就是「自然死亡」，現在交給壽命，所以設成 0。擁擠造成的死亡 C_die · ρ² 照舊。
- **電腦上先模擬**（沒有大圓）：τ = 10 s、β = 2 時點數穩定在 130 ～ 150 個，跟原本 P_DIE_0 = 0.02 差不多。

## 規則的問題和做法

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

## 參數

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

## 實測（RHO_0 = 0.55 時，跑 2 分鐘）

- 約 12.5 fps：整張畫面每幀重畫，送出就要 64 ms；最慢一幀 98 ms（爆發的那幀）。
- 點數在 4 ～ 57 之間起伏；大圓大約每 45 秒長到上限一次。
- 點偏少，所以 `RHO_0` 調成 0.65（模擬裡點數大約多一倍）。
- **RAM**：1659 B（多了追的目標 9 B、每個點的壽命 150 B）。噴出去的點不存位置和速度，只存方向和初速（3 bytes），位置用公式算，省了 312 B。
