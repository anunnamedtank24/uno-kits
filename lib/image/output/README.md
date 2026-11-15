# image/output

`source/convert.py` 產生的圖片，`namespace picture`。除了 `selected.h` 可以手動改，其他檔案都不要手動編輯，要改就重新轉換。

## 檔案

```
output/
├── selected.h     程式裡能用的圖片，image.h 會 include 它
├── *.h            給板子的檔案：uno 有像素，mac 只有尺寸和路徑
└── mac/*.h        mac 的像素，只有 stream.py 會讀，永遠不會被 include
```

## selected.h

- `convert.py` 每次只把剛產生的那個加進來，其他的保持原樣。
- 不用的圖片刪掉那行 `#include` 就好；就算留著，程式沒用到也不佔 Flash。
- `python -B source/check.py` 檢查每個 include 都是目前格式、原始圖片沒變過。

## 目前的圖片

| 檔案 | 來源 | 螢幕上 | 顏色 | 編碼 | 資料 |
|---|---|---|---|---|---|
| `sunset_uno_120x80_palette_16.h` | sunset.jpg | 120×80（存 60×40，scale 2） | 16（用了 15） | runs | 319 B |
| `sunset_uno_80x53_palette_256.h` | sunset.webp | 80×53 | 256 | raw | 4744 B |
| `sunset_uno_60x40_palette_565.h` | sunset.png | 60×40 | 565 | runs | 1629 B |
| `sunset_mac_240x240_palette_565.h` | sunset.heic | 240×240（`--fill`） | 565 | runs | 19137 B（在 `mac/`） |
| `logo_uno_mac_128x64_palette_2.h` | logo.png（透明） | 128×64 | 2 | runs | 357 B |
| `sunset_uno_mac_96x64_palette_2.h` | sunset.jpg（`--dither`） | 96×64 | 2 | raw | 772 B |
| `sunset_uno_128x64_palette_2.h` | sunset.heic（`--fill`、`--dither`） | 128×64 | 2 | raw | 1028 B |
| `moon_uno_128x64_palette_2.h` | moon.jpg（`--fill`、`--dither`） | 128×64 | 2 | raw | 1028 B |
| `city_uno_128x64_palette_2.h` | city.png（`--fill`、`--threshold 100`） | 128×64 | 2 | raw | 1028 B |
| `treble_clef_uno_15x40_palette_2.h` | treble_clef.png | 15×40 | 2 | raw | 75 B |

`assets/` 的圖片是用 Pillow 畫的：漸層天空、太陽、三層山（sunset）、星空和月亮（moon）、亮著窗戶的大樓（city），以及透明背景的圖示（logo）。後面三張 128×64 的是 `src/week_4` 作業一、三的照片。treble_clef 是高音譜記號：用 Pillow 把 macOS 字型 Apple Symbols 的 𝄞（U+1D11E）畫成 40 px 高的黑白圖，給 `src/week_5/assignment_3` 的五線譜用（漩渦的中心在第 25 列，對齊 G4 線）。sunset 另外存成 jpg、webp，再用 `sips` 轉成 heic，四種格式都測得到。

## 產生的 struct

```cpp
namespace picture {
    struct sunset_uno_120x80_palette_16 {
        static constexpr uint8_t target;        // 位元旗標：image::TARGET_UNO = 1、TARGET_MAC = 2，兩者都有就是 3
        static constexpr uint8_t width, height; // 存的大小（放大前），1 ～ 240
        static constexpr uint8_t scaleX, scaleY;
        static constexpr uint8_t colorMode;     // image::COLORS_2 = 0 / COLORS_16 = 1 / COLORS_256 = 2 / COLORS_565 = 3
        static constexpr uint8_t encoding;      // image::ENCODING_RAW = 0 / ENCODING_RUNS = 1
        static constexpr uint16_t colorCount;   // 調色盤顏色數，565 是 0
        static constexpr uint32_t dataSize;     // 像素資料的 bytes
        static inline const char *getPath();    // 像素檔的路徑（相對於 lib/image），StreamImage 傳給 stream.py

        // 有像素的檔案（--target uno、uno mac，以及 mac/ 裡的檔案）才有：
        static inline const uint16_t *getPalette();   // colorCount 個 RGB565
        static inline const uint8_t *getPixels();     // 像素，格式見下
    };
}
```

- **`--target mac`** 產生兩個同名的 struct：
  - `output/x.h`：只有上半部，加進 `selected.h`。
  - `output/mac/x.h`：完整的，`getPath()` 指向它。
- **檔案開頭**：
  - `// format: 1`：格式版本。
  - `// source: … sha256 …`：原始圖片和它的 hash。
  - `// settings:`：所有設定，包括原本的指令、裁切範圍、實際尺寸、顏色、編碼、資料量、估計的時間。

## 格式（format 1）

`decoder.h` 解碼，`convert.py` 編碼；`convert.py` 裡也有 Python 版的解碼器（`get_decoded_values`），轉換後自我檢查、`stream.py` 檢查檔案都用它。Flash 和串流共用這個格式。

像素一列一列排列，共 width × height 個，每個在螢幕上畫成 scaleX × scaleY。

**raw**：每個像素照原樣，位元跨列連續（列尾不補齊）

| 顏色模式 | 每個像素 |
|---|---|
| `COLORS_2` | 1 bit，第一個像素在 bit 7 |
| `COLORS_16` | 4 bits，第一個像素在高 4 bit |
| `COLORS_256` | 1 byte（調色盤編號） |
| `COLORS_565` | 2 bytes，little endian |

**runs**：一段同色的像素，可以跨列

| 顏色模式 | 一段 |
|---|---|
| `COLORS_2` | 1 byte：bit 7 = 顏色編號，bit 0 ～ 6 = n − 1（1 ～ 128 個像素） |
| `COLORS_16` | 1 byte：高 4 bit = n，低 4 bit = 顏色編號；n = 0 時，下一個 byte + 16 個像素（16 ～ 271） |
| `COLORS_256` | 2 bytes：[n − 1][顏色編號]（1 ～ 256） |
| `COLORS_565` | 3 bytes：[n − 1][RGB565 低位][RGB565 高位]（1 ～ 256） |

`COLORS_16`、`COLORS_256`、`COLORS_565` 的 runs 跟 video 的格式（format 3）相同。
