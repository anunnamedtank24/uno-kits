# image

在螢幕上畫靜態圖片。`namespace image` 是畫圖的程式，`namespace picture` 是產生的圖片。跟 [GIF](../GIF/README.md) 一樣有兩種來源：

- **預先編譯**：圖片燒進 Flash。
- **跟 Mac 拿**：圖片留在電腦，經過 USB 串口送過來。

格式和參數也幾乎一樣：尺寸、`--scale`、調色盤或全彩。

| 文件 | 內容 |
|---|---|
| 這份 | 資料夾、兩種來源、用法、跟 GIF 的比較 |
| [source/README.md](source/README.md) | `convert.py` 的參數和流程、支援的檔案、`check.py`、`stream.py` 和串流協定 |
| [output/README.md](output/README.md) | `selected.h`、產生的 struct、像素格式 |

## 資料夾

```
lib/image/
├── image.h            入口：兩個類別 + output/selected.h
├── decoder.h          image::Decoder<Screen, SEGMENT>：像素格式，畫一張圖
├── flash_image.h      image::FlashImage<Screen, SEGMENT>：畫 Flash 裡的圖（--target uno）
├── stream_image.h     image::StreamImage<Screen, Link, CHUNK, SEGMENT>：畫 Mac 傳來的圖（--target mac）
├── assets/            原始圖片（sunset.jpg / .png / .webp / .heic、logo.png）
├── output/            產生的圖片
└── source/            電腦上的工具：config.json、convert.py、check.py、stream.py
```

## 支援的檔案

| 副檔名 | 怎麼讀 |
|---|---|
| `.jpg`、`.jpeg` | Pillow |
| `.png`（可以有透明） | Pillow；透明的地方放在 `--background` 上（預設黑） |
| `.webp` | Pillow |
| `.heic`、`.heif`（iPhone 照片） | 有裝 `pillow-heif` 就用它；沒有的話用 macOS 內建的 `sips` 先轉成 png，所以不用另外安裝 |

- 照片的旋轉方向（EXIF orientation）會先轉正。
- 動畫請用 [GIF](../GIF/README.md)。

## 顏色

| `--palette` | 每個像素 | 適合 | Flash（uno） | 串流（mac） |
|---|---|---|---|---|
| `2` | 1 bit：黑或白 | OLED、線條圖；照片加 `--dither` 變成點狀的灰階 | ✅ | ✅ |
| `16` | 4 bits，調色盤最多 16 色 | 扁平色塊、插圖 | ✅（預設） | ✅ |
| `256` | 1 byte，調色盤最多 256 色 | 照片，比 565 省一半 | ✅ | ❌ 調色盤要 512 B RAM |
| `565` | 2 bytes，每個像素自己的顏色 | 照片、漸層 | ✅（很大，會警告） | ✅（預設） |

編碼由 `convert.py` 自動挑比較小的：

- **raw**：每個像素照原樣存，適合照片和抖色圖。
- **runs**：連續同色的一段存成一筆，適合色塊和圖示。

## 兩種來源

| | `--target uno` | `--target mac` | `--target uno mac` |
|---|---|---|---|
| 像素在哪 | `output/x.h`，燒進 Flash | `output/mac/x.h`，留在 Mac | `output/x.h` |
| 能用的類別 | `FlashImage`（`StreamImage` 也行，會警告） | `StreamImage` | 兩個都可以 |
| 適合 | 小、色少、不需要電腦 | 大、全彩（240×240 全彩約 115 KB，Flash 只有 32 KB） | 兩種都想用 |

**用錯會被抓到**：

| 情況 | 結果 | 原因 |
|---|---|---|
| `FlashImage` 拿到只有 mac 的檔案 | 編譯錯誤 | 檔案裡沒有像素 |
| `StreamImage` 拿到只有 uno 的檔案 | 編譯警告 | 能畫，但沒用 mac 的範圍檢查過 |
| `StreamImage` 拿到 256 色的檔案 | 編譯錯誤 | 板子沒有 512 B RAM 放調色盤 |

## FlashImage

```bash
cd lib/image
python -B source/convert.py assets/sunset.jpg output --target uno --fit 120x80 --scale 2 --palette 16
```

```cpp
#include "image.h"
image::FlashImage<Screen> pictures(screen);
pictures.draw<picture::sunset_uno_120x80_palette_16>(60, 40);   // 左上角 (60, 40)
```

## StreamImage

**Mac**：執行一次就好，不用參數。它會自動找板子，燒錄時也會自動放開串口。
```bash
pip install pillow pyserial
cd lib/image
python -B source/convert.py assets/sunset.heic output --target mac --fill 240x240 --palette 565
python -B source/stream.py
```

**UNO**：
```cpp
using Link = Uno::UnoSerial<1000000>;
image::StreamImage<Screen, Link> pictures(screen);      // CHUNK = 64：一次收幾個 byte（RAM）
uint8_t result = pictures.draw<picture::sunset_mac_240x240_palette_565>(0, 0);
if (result != pictures.OK) {
    // ERROR_NOT_FOUND 1、ERROR_UNREADABLE 2、ERROR_NOT_STREAMABLE 3、ERROR_NO_ANSWER 4、ERROR_BROKEN 5
}
```

- **會等到畫完**：`draw()` 要等整張畫完才返回。實測 240×240 全彩（19 KB）約 1.7 秒，128×64 黑白約 0.07 秒。
- **不能同時開兩個 `stream.py`**：`lib/GIF/source/stream.py` 也要用同一個串口，一次只能開一個。
- **不能用 Arduino 的 `Serial`**：`UnoSerial` 用 D0 / D1。

## 在 OLED 上畫

`Decoder` 只透過 `writeWindow` 畫，所以 OLED（[I2C](../I2C/README.md)）也能用：

```bash
python -B source/convert.py assets/logo.png output --target uno mac --fit 128x64 --palette 2
```

```cpp
image::FlashImage<Screen> pictures(screen);           // Screen = graphics::Canvas<SSD1306::OLED128x64<Bus>>
pictures.draw<picture::logo_uno_mac_128x64_palette_2>(0, 0);
screen.display();                                      // OLED：畫完要送出
```

- **用 `--palette 2`**：OLED 上顏色 0 是暗，其他都是亮。如果用彩色的圖，除了純黑以外全部都會變亮。
- **照片加 `--dither`**：用點的密度表現灰階。
- **調整亮暗**：`--threshold` 決定多亮算白，`--invert` 把黑白反過來。

## 怎麼畫

一列像素切成每段 `SEGMENT`（預設 32）個螢幕像素，每段呼叫一次 `writeWindow`，每個存的像素重複畫 scaleX × scaleY 次。

| 好處 | 原因 |
|---|---|
| **會裁切** | `writeWindow` 會裁切：圖片可以一部分在螢幕外，例如 x < 0 |
| **任何螢幕都能用** | 只用到 `writeWindow`：ST7789、OLED 都有 |
| **RAM 少** | 只留一段的顏色（2 × SEGMENT bytes），不需要一整列 |

代價是比 GIF 的直接串流（`setWindow`，不裁切）多開很多窗口：240×240 要 1920 段，每段約 56 µs，光開窗口就約 0.1 秒（估計）。

## 跟 GIF 比較

| | GIF | image |
|---|---|---|
| 內容 | 動畫（多幀、時間） | 一張圖 |
| 顏色 | 16、256、565 | 2、16、256、565 |
| 編碼 | runs | raw 或 runs，自動選比較小的 |
| 畫法 | `setWindow` + `writeData16` 直接送 | 分段 `writeWindow` |
| 裁切 | ❌ 開始時檢查，放不下就不畫 | ✅ |
| 螢幕 | 只有 ST7789（要 `setWindow`） | 任何有 `writeWindow` 的（ST7789、OLED） |
| 存的大小上限 | 255（有放大時寬 ≤ 120） | 240，沒有其他限制 |
| 串流協定 | 開啟、要下一幀、播放控制 | 只有「要這張圖」 |
| 串流的 `stream.py` | `lib/GIF/source/stream.py` | `lib/image/source/stream.py`（同時只能開一個） |

## RAM（估計）

| 物件 | bytes | 內容 |
|---|---|---|
| `image::FlashImage<Screen>` | 約 115 | 一段顏色 64（SEGMENT = 32）+ 調色盤 32 + 位置、格式、run 的狀態 |
| `image::StreamImage<Screen, Link>` | 約 180 | 上面 + 接收緩衝 64（CHUNK） |

建議放在函式裡（堆疊），畫完就還回去。`src/week_4/test_1.cpp` 就是這樣用的，它跟 OLED 1 KB 的畫面一起用也夠。

## 輸入限制

| 參數 | 型別 | 範圍 | 超出時 |
|---|---|---|---|
| `draw` 的 x、y | `int16_t` | 任何值 | 裁切：只畫螢幕上的部分 |
| `Picture` | `picture::*` | `output/selected.h` 裡的 struct | 編譯錯誤 |
| 存的寬、高 | `uint8_t` | 1 ～ 240 | `convert.py` 報錯 |
| scale | `uint8_t` | 1 ～ 8 | `convert.py` 報錯 |
| 調色盤顏色數 | — | 2：2 色；16：≤ 16；256：≤ 256 | `convert.py` 保證；Flash 版超過 16 色的 2 / 16 色圖會編譯錯誤 |
| `SEGMENT`、`CHUNK` | `uint8_t` | 1 ～ 255，佔同樣多的 RAM（SEGMENT × 2） | — |
| 串流途中斷線 | — | 200 ms 收不到下一塊 | 剩下的像素畫成黑色，回傳 `ERROR_BROKEN` |
| `stream.py` 沒開 | — | 3 秒沒回答 | `ERROR_NO_ANSWER`，什麼都不畫 |
