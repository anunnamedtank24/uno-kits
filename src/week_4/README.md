# week_4

I2C OLED（SSD1306，0.96 吋 128×64）、OLED 上的黑白圖片（[lib/image](../../lib/image/README.md)；彩色圖片的測試在 [week_3/test_3](../week_3/README.md#test_3cpp)）、開關（[lib/switch](../../lib/switch/README.md)）的測試，以及三個作業。OLED 函式庫見 [lib/I2C/README.md](../../lib/I2C/README.md)。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python -B src/hide.py week_4/test_1     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `test_1.cpp` | OLED：尺寸、圖形、文字、裁切、速度、datasheet 的每一類指令、黑白圖片（Flash 和串流） | I2C OLED |
| `test_2.cpp` | 開關的狀態，用文字送到串口 | 開關模組 |
| `assignment_1.cpp` | 作業一：按開關換下一張照片 | OLED + 開關模組 |
| `assignment_2.cpp` | 作業二：OLED 播 GIF | OLED |
| `assignment_3.cpp` | 作業三：照片每 3 秒自動換下一張 | OLED |

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
python -B source/stream.py     # 先開它，再燒錄；GIF 的 stream.py 要先關掉
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

# 作業

照片和 GIF 都是黑白的，因為 OLED 每個點只有亮或暗。

| | 照片 / 動畫 | 轉換指令（在 `lib/image` 或 `lib/GIF` 裡） |
|---|---|---|
| 夕陽 | `sunset_uno_128x64_palette_2` | `python -B source/convert.py assets/sunset.heic output --target uno --fill 128x64 --palette 2 --dither` |
| 月亮 | `moon_uno_128x64_palette_2` | `... assets/moon.jpg ... --palette 2 --dither` |
| 城市 | `city_uno_128x64_palette_2` | `... assets/city.png ... --palette 2 --threshold 100` |
| logo | `logo_uno_mac_128x64_palette_2` | `... assets/logo.png --target uno mac --fit 128x64 --palette 2` |
| GIF | `orbit_uno_64x64_palette_16_mono` | `python -B source/convert.py assets/orbit.gif output --target uno --fit 64x64 --mono --keyframe 8` |

換成自己的照片：放進 `lib/image/assets/`（jpg、png、webp、heic 都可以），用上面的指令轉換，再把作業裡 `photos[]` 的名字換掉。

## assignment_1.cpp：按開關換照片

- 開機顯示第一張；每按一次（`wasPressed()`）換下一張，最後一張之後回到第一張。
- 照片畫進 OLED 的 RAM 畫面，再 `display()` 送出改過的部分。
- Flash 7.2 KB、RAM 1087 B（OLED 的畫面就佔 1 KB）。

## assignment_2.cpp：OLED 播 GIF

- `GIF::FlashPlayer` 直接畫在 OLED 上：OLED 的 `Frame` 也提供 `setWindow` / `writeData16`，畫進 RAM 的畫面；每畫完一幀呼叫 `display()`。
- 64×64 的 orbit 放在中間（x = 32），一直循環。
- 實測 16.5 ～ 17 fps（GIF 本身 16.7 fps），沒有跳幀，最慢一幀（I 幀）66 ms。
- Flash 4.6 KB、RAM 1369 B。

## assignment_3.cpp：自動換照片

- 跟作業一一樣的四張照片，每 3 秒（`SLIDE_MS`）換下一張。
- 時間從上一次換照片的時間點往後算，所以畫圖花的時間不會讓間隔越來越長。
- Flash 7.0 KB、RAM 1073 B。

