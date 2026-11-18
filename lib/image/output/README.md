# image/output

`source/convert.py` 和程式裡的巨集產生的圖片，`namespace picture`。這裡的檔案都不要手動編輯，要改就重新轉換。

## 檔案

```
output/
├── selected.h    自動寫：列出這裡全部的 .h，image.h 會 include 它
├── registry.h    自動寫：目前程式裡 PICTURE("路徑", "選項") 對應的型態
└── *.h           轉好的圖片：手動跑 convert.py 或 PICTURE(...) 轉的都在這裡，格式一樣
```

## selected.h、registry.h

- 都是 `tools/assets.py` 每次編譯前寫的（`convert.py` 也會把新轉的加進 `selected.h`），不用手動編輯。
- `selected.h` 列出**全部**：沒用到的不佔 Flash（資料在 `inline` 函式裡的 `PROGMEM` 陣列，沒呼叫就不會產生，實測 Flash 一樣）。
- `registry.h` 只有目前程式裡的 `PICTURE(...)`。
- `python source/check.py` 檢查每個檔案都是目前的格式、原始檔沒變過；`convert.py` 每次轉完會自動檢查一次。

## 目前的圖片

| 檔案 | 來源 | 螢幕上 | 顏色 | 編碼 | 資料 |
|---|---|---|---|---|---|
| `sunset_flash_120x80_palette_16.h` | sunset.jpg | 120×80（存 60×40，scale 2） | 16（用了 15） | runs | 319 B |
| `sunset_flash_80x53_palette_256.h` | sunset.webp | 80×53 | 256 | raw | 4744 B |
| `sunset_flash_60x40_palette_565.h` | sunset.png | 60×40 | 565 | runs | 1629 B |
| `sunset_stream_240x240_palette_565.h` | sunset.heic | 240×240（`--fill`） | 565 | runs | 19137 B（在 `#if 0` 裡） |
| `logo_flash_stream_128x64_palette_2.h` | logo.png（透明） | 128×64 | 2 | runs | 357 B |
| `sunset_flash_stream_96x64_palette_2.h` | sunset.jpg（`--dither`） | 96×64 | 2 | raw | 772 B |
| `sunset_flash_128x64_palette_2.h` | sunset.heic（`--fill`、`--dither`） | 128×64 | 2 | raw | 1028 B |
| `moon_flash_128x64_palette_2.h` | moon.jpg（`--fill`、`--dither`） | 128×64 | 2 | raw | 1028 B |
| `city_flash_128x64_palette_2.h` | city.png（`--fill`、`--threshold 100`） | 128×64 | 2 | raw | 1028 B |
| `treble_clef_flash_15x40_palette_2.h` | treble_clef.png | 15×40 | 2 | raw | 75 B |
| `moon_flash_64x32_palette_2.h` | moon.jpg（`PICTURE(...)`，`src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp`） | 64×32 | 2，dither | raw | 260 B |

`assets/` 的圖片是用 Pillow 畫的：漸層天空、太陽、三層山（sunset）、星空和月亮（moon）、亮著窗戶的大樓（city），以及透明背景的圖示（logo）。後面三張 128×64 的是 `src/week_4_SSD1306_switch_buzzer_audio` 作業一、三的照片。treble_clef 是高音譜記號：用 Pillow 把 macOS 字型 Apple Symbols 的 𝄞（U+1D11E）畫成 40 px 高的黑白圖，給 `src/week_5_NEC/assignments/3_staff` 的五線譜用（漩渦的中心在第 25 列，對齊 G4 線）。sunset 另外存成 jpg、webp，再用 `sips` 轉成 heic，四種格式都測得到。

## 產生的 struct

```cpp
namespace picture {
    struct sunset_flash_120x80_palette_16 {
        static constexpr uint8_t target;        // 位元旗標：image::TARGET_FLASH = 1、TARGET_STREAM = 2，兩者都有就是 3
        static constexpr uint8_t width, height; // 存的大小（放大前），1 ～ 240
        static constexpr uint8_t scaleX, scaleY;
        static constexpr uint8_t colorMode;     // image::COLORS_2 = 0 / COLORS_16 = 1 / COLORS_256 = 2 / COLORS_565 = 3
        static constexpr uint8_t encoding;      // image::ENCODING_RAW = 0 / ENCODING_RUNS = 1
        static constexpr uint16_t colorCount;   // 調色盤顏色數，565 是 0
        static constexpr uint32_t dataSize;     // 像素資料的 bytes
        static inline const char *getPath();    // 像素檔的路徑（相對於 lib/image），StreamImage 傳給 stream.py

        // 有像素的檔案（--target flash、flash stream）才編譯；--target stream 的在 #if 0 裡：
        static inline const uint16_t *getPalette();   // colorCount 個 RGB565
        static inline const uint8_t *getPixels();     // 像素，格式見下
    };
}
```

- **`--target stream`** 也只有一個檔案 `output/x.h`：前面是板子編譯的上半部（大小、`getPath()`），後面的像素放在 `#if 0` 裡（太大，燒不進 Flash，所以永遠不編譯），`stream.py` 從同一個檔案讀。檔名的 `stream` 和 struct 的 `target` 都寫著它是串流用的。
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

## 名稱和來源

每個 .h 的開頭都寫著它從哪個檔案、用什麼選項轉出來：

```
// options: --fit 120x80 --palette 16
// source: assets/sunset.jpg sha256 1413…
```

程式裡的 `PICTURE("路徑", "選項")` 就是靠這兩行找到已經轉好的檔案：同一個檔案、同樣的選項（順序、`--fit=…` 的寫法不影響）就直接用，沒有才轉。所以不管是手動轉的還是巨集轉的，同一份只轉一次。

| 規則 | 說明 |
|---|---|
| 名稱 | `convert.py` 的規則：`{檔名}_{設定}`，struct 同名（`picture::名稱`） |
| `assets/` 外面的檔案 | 檔名前面加上資料夾（相對於專案；不在專案裡就相對於家目錄），例如 `~/Music/Game/title.mp3` → `music_game_title_…`，不同資料夾的同名檔案不會混在一起 |
| 不是英數字的字（例如日文） | 拿掉，改加 6 碼雜湊，例如 `16 戦い：ウォロ.aiff` → `…_16_7b5dc7_…` |
| 名稱撞到（名稱沒包含每個選項，或來源不同） | 加 `_2`、`_3`…，不會蓋掉別的檔案 |
| 原始檔改過 | 編譯時會警告；`python tools/assets.py --force` 重轉（同名的蓋掉） |
| 原始檔移走、刪掉 | 輸出還在就照樣能編譯（程式裡的舊路徑一樣找得到它） |
| 不要的 | `python tools/assets.py --unused` 列出沒有程式（包括 `.bak`）用的，自己刪 |
