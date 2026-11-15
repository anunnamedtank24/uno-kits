# image

在螢幕上畫靜態圖片，`namespace image`（畫圖的程式）和 `namespace picture`（產生的圖片）。跟 [video](../video/README.md) 一樣有兩種來源，參數也幾乎一樣（尺寸、`--scale`、調色盤或全彩）：

- **預先編譯**：圖片燒進 Flash（`FlashImage`）。
- **跟 Mac 拿**：圖片留在電腦，經過 USB 串口送過來（`StreamImage`）。

## 檔案

```
lib/image/
├── image.h            入口：兩個類別 + output/selected.h
├── decoder.h          Decoder<Screen, SEGMENT>：像素格式，畫一張圖
├── flash_image.h      FlashImage<Screen, SEGMENT>：畫 Flash 裡的圖（--target uno）
├── stream_image.h     StreamImage<Screen, Link, CHUNK, SEGMENT>：畫 Mac 傳來的圖（--target mac）
├── assets/            原始圖片（sunset.jpg / .png / .webp / .heic、logo.png、moon.jpg、city.png）
├── source/            config.json、convert.py、check.py、stream.py，見 source/README.md
└── output/            產生的圖片和 selected.h，見 output/README.md
```

## 用法

### FlashImage

```bash
cd lib/image
python -B source/convert.py assets/sunset.jpg output --target uno --fit 120x80 --scale 2 --palette 16
```

```cpp
#include "image.h"
image::FlashImage<Screen> pictures(screen);
pictures.draw<picture::sunset_uno_120x80_palette_16>(60, 40);   // 左上角 (60, 40)
```

### StreamImage

**Mac**：執行一次就好，不用參數。它會自動找板子，燒錄時也會自動放開串口。

```bash
cd lib/image
python -B source/convert.py assets/sunset.heic output --target mac --fill 240x240 --palette 565
python -B source/stream.py
```

**UNO**：

```cpp
using Link = Uno::UnoSerial<1000000>;
image::StreamImage<Screen, Link> pictures(screen);
uint8_t result = pictures.draw<picture::sunset_mac_240x240_palette_565>(0, 0);
if (result != pictures.OK) {
    // 見下面的錯誤代碼
}
```

### 在 OLED 上畫

`Decoder` 只透過 `writeWindow` 畫，所以 OLED（[I2C](../I2C/README.md)）也能用：

```bash
python -B source/convert.py assets/logo.png output --target uno mac --fit 128x64 --palette 2
```

```cpp
image::FlashImage<Screen> pictures(screen);           // Screen = graphics::Canvas<SSD1306::OLED128x64<Bus>>
pictures.draw<picture::logo_uno_mac_128x64_palette_2>(0, 0);
screen.display();                                      // OLED：畫完要送出
```

- **用 `--palette 2`**：OLED 上顏色 0 是暗，其他都是亮；彩色的圖除了純黑以外全部都會變亮。
- **照片加 `--dither`**：用點的密度表現灰階；`--threshold` 決定多亮算白，`--invert` 把黑白反過來。

完整的例子：`src/week_3/test_3.cpp`（彩色）、`src/week_4/test_1.cpp` 的 `tests[14]`、`tests[15]`（OLED）、`src/week_4/assignment_1.cpp`。

## 介面需求

| 參數 | 需要的函式 | 現成的實作 |
|---|---|---|
| `Screen` | `writeWindow(x, y, w, h, colors)`（會裁切）；型別 `BusType`，它有 `readFlash8/16`（讀 Flash 裡的圖）、`nowMs`（`StreamImage` 計時） | `ST7789::M130T`、`SSD1306::OLED128x64` |
| `Link`（`StreamImage`） | `init()`、`available()`、`read8(byte, timeoutMs)`、`write8(byte)`、`clearInput()` | `Uno::UnoSerial` |
| `Picture` | `output/selected.h` 裡的 struct（見 [output/README.md](output/README.md#產生的-struct)） | `picture::*` |

## 函式

| 函式 | 說明 |
|---|---|
| `FlashImage::draw<Picture>(x, y)` | 畫 Flash 裡的圖，左上角 (x, y) |
| `StreamImage::draw<Picture>(x, y)` | 跟 `stream.py` 要圖、邊收邊畫，畫完才返回；回傳結果 |

| `StreamImage::draw` 回傳 | 原因 |
|---|---|
| `OK`（0） | — |
| `ERROR_NOT_FOUND`（1） | `stream.py` 找不到檔案（或在 `lib/image` 外面） |
| `ERROR_UNREADABLE`（2） | 檔案讀不懂 |
| `ERROR_NOT_STREAMABLE`（3） | 256 色不能串流 |
| `ERROR_NO_ANSWER`（4） | 3 秒沒回答（`stream.py` 沒開），什麼都不畫 |
| `ERROR_BROKEN`（5） | 傳到一半斷了（200 ms 收不到下一塊）或標頭不合理，剩下的像素畫成黑色 |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `SEGMENT` | 32 | 一次 `writeWindow` 畫幾個螢幕像素，佔 2 × SEGMENT bytes RAM |
| `CHUNK`（`StreamImage`） | 64 | 一次收幾個 byte，佔同樣多的 RAM |

## 顏色和來源

| `--palette` | 每個像素 | 適合 | Flash（uno） | 串流（mac） |
|---|---|---|---|---|
| `2` | 1 bit：黑或白 | OLED、線條圖；照片加 `--dither` | ✅ | ✅ |
| `16` | 4 bits，調色盤最多 16 色 | 扁平色塊、插圖 | ✅（預設） | ✅ |
| `256` | 1 byte，調色盤最多 256 色 | 照片，比 565 省一半 | ✅ | ❌ 調色盤要 512 B RAM |
| `565` | 2 bytes，每個像素自己的顏色 | 照片、漸層 | ✅（很大，會警告） | ✅（預設） |

編碼由 `convert.py` 自動挑比較小的：**raw**（每個像素照原樣，適合照片和抖色圖）或 **runs**（連續同色的一段存成一筆，適合色塊和圖示）。

| | `--target uno` | `--target mac` | `--target uno mac` |
|---|---|---|---|
| 像素在哪 | `output/x.h`，燒進 Flash | `output/mac/x.h`，留在 Mac | `output/x.h` |
| 能用的類別 | `FlashImage`（`StreamImage` 也行，會警告） | `StreamImage` | 兩個都可以 |
| 適合 | 小、色少、不需要電腦 | 大、全彩（240×240 全彩約 115 KB，Flash 只有 32 KB） | 兩種都想用 |

| 用錯時 | 結果 | 原因 |
|---|---|---|
| `FlashImage` 拿到只有 mac 的檔案 | 編譯錯誤 | 檔案裡沒有像素 |
| `StreamImage` 拿到只有 uno 的檔案 | 編譯警告 | 能畫，但沒用 mac 的範圍檢查過 |
| `StreamImage` 拿到 256 色的檔案 | 編譯錯誤 | 板子沒有 512 B RAM 放調色盤 |

## 怎麼畫

一列像素切成每段 `SEGMENT` 個螢幕像素，每段呼叫一次 `writeWindow`，每個存的像素重複畫 scaleX × scaleY 次。

| 好處 | 原因 |
|---|---|
| 會裁切 | `writeWindow` 會裁切：圖片可以一部分在螢幕外 |
| 任何螢幕都能用 | 只用到 `writeWindow` |
| RAM 少 | 只留一段的顏色（2 × SEGMENT bytes），不需要一整列 |

代價是比 video 的直接串流（`setWindow`）多開很多窗口：240×240 要 1920 段，每段約 56 µs，光開窗口就約 0.1 秒（估計）。

### 跟 video 比較

| | video | image |
|---|---|---|
| 內容 | 動畫（多幀、時間） | 一張圖 |
| 顏色 | 16、256、565 | 2、16、256、565 |
| 編碼 | runs | raw 或 runs，自動選比較小的 |
| 畫法 | `setWindow` + `writeData16` 直接送 | 分段 `writeWindow` |
| 裁切 | ❌ 開始時檢查，放不下就不畫 | ✅ |
| 存的大小上限 | 255（有放大時寬 ≤ 120） | 240 |
| 串流協定 | 開啟、要下一幀、播放控制 | 只有「要這張圖」 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `draw` 的 x、y | 任意 | 裁切 |
| `Picture` | `output/selected.h` 裡的 struct | 編譯錯誤 |
| 存的寬、高 | 1 ～ 240 | `convert.py` 報錯 |
| scale | 1 ～ 8 | `convert.py` 報錯 |
| 調色盤顏色數 | 2：2 色；16：≤ 16；256：≤ 256 | `convert.py` 保證；Flash 版超過時編譯錯誤 |
| `SEGMENT`、`CHUNK` | 1 ～ 255 | — |

## 資源

| 項目 | 用量 |
|---|---|
| RAM（估計） | `FlashImage<Screen>` 約 115 bytes（一段顏色 64 + 調色盤 32 + 狀態）；`StreamImage<Screen, Link>` 約 180 bytes（再加接收緩衝 64） |
| 時間（實測） | 串流 240×240 全彩（19 KB）約 1.7 秒，128×64 黑白約 0.07 秒 |
| 硬體 | Screen 的；`StreamImage` 另外用 D0 / D1（`UnoSerial`） |

建議放在函式裡（堆疊），畫完就還回去；跟 OLED 1 KB 的畫面一起用也夠。

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| `StreamImage::draw` 很久 | 等到整張畫完才返回 | 大圖用 `--scale` 或 palette 降低資料量 |
| 同時開 video 的 `stream.py` | 兩個都要用同一個串口 | 一次只開一個 |
| 同時用 Arduino 的 `Serial` | `UnoSerial` 也用 D0 / D1 | 不能同時用 |
| 想放動畫 | 這裡只有一張圖 | 用 [video](../video/README.md) |
