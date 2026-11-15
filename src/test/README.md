# src/test

在真的板子上跑的測試。`src/` 裡同時只能有一個檔案有 `setup()` / `loop()`，其他的要改成 `.bak`。

| 檔案 | 測什麼 | 需要的硬體 |
|---|---|---|
| `test_1.cpp` | 螢幕、圖形、文字、GIF | M130T 螢幕 |
| `test_2.cpp` | DS18B20 溫度，每秒用文字送到串口 | DS18B20 |

# test_1.cpp

每個測試畫一個畫面，停 1 秒（`HOLD_MS`）再清畫面跑下一個，一直循環。每個測試上方的註解寫著「應該看到什麼」。

## 接線

UNO + M130T（1.3 吋 240×240，沒有 CS）：

| M130T | UNO |
|---|---|
| SCL | D13 |
| SDA | D11 |
| DC | D8 |
| RES | D10 |
| BLK | D9 |
| VCC / GND | 3.3V / GND |

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

## 為什麼在 `test/` 裡也能編譯

PlatformIO 找依賴時會跳過 `src/test/`，所以 `platformio.ini` 用 `lib_deps` 直接列出 `lib/` 的六個資料夾。

# test_2.cpp

## 接線

| DS18B20 | UNO |
|---|---|
| VCC | 5V |
| DQ | D2（要有 4.7 kΩ 上拉到 VCC，大部分模組板上已經有） |
| GND | GND |

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

