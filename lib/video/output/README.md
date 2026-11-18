# video/output

`source/convert.py` 和程式裡的巨集產生的動畫，`namespace animation`。這裡的檔案都不要手動編輯，要改就重新轉換。

## 檔案

```
output/
├── selected.h    自動寫：列出這裡全部的 .h，video.h 會 include 它
├── registry.h    自動寫：目前程式裡 ANIMATION("路徑", "選項") 對應的型態
└── *.h           轉好的動畫：手動跑 convert.py 或 ANIMATION(...) 轉的都在這裡，格式一樣
```

## selected.h、registry.h

- 都是 `tools/assets.py` 每次編譯前寫的（`convert.py` 也會把新轉的加進 `selected.h`），不用手動編輯。
- `selected.h` 列出**全部**：沒用到的不佔 Flash（資料在 `inline` 函式裡的 `PROGMEM` 陣列，沒呼叫就不會產生，實測 Flash 一樣）。
- `registry.h` 只有目前程式裡的 `ANIMATION(...)`。
- `python source/check.py` 檢查每個檔案都是目前的格式、原始檔沒變過；`convert.py` 每次轉完會自動檢查一次。

## 目前的動畫

都從 `assets/orbit.gif`（120×120、24 幀、1.44 秒、16.7 fps）轉出來：

| 檔案 | 指令 | 資料 | 每幀（估計） |
|---|---|---|---|
| `orbit_flash_120x120_palette_16.h` | `--target flash --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8` | 1992 B | 1.4 ～ 40.2 ms |
| `orbit_flash_64x64_palette_16_mono.h` | `--target flash --fit 64x64 --mono --keyframe 8`（OLED） | 882 B | 0.4 ～ 11.4 ms |
| `orbit_flash_stream_120x120_palette_16.h` | `--target stream flash --width 120 --scale 2` | 2352 B | 1.8 ～ 40.4 ms |
| `orbit_stream_120x120_palette_565.h` | `--target stream --fit 120x120 --palette 565` | 4658 B（在 `#if 0` 裡） | 2.4 ～ 54.0 ms（含傳送） |
| `orbit_flash_64x64_palette_16_mono_2.h` | `ANIMATION("orbit.gif", "--width 64 --mono")`（`src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp`）；跟上面的 mono 只差 `--keyframe`，所以加 `_2` | 753 B | 0.4 ～ 11.4 ms |

## 產生的 struct

```cpp
namespace animation {
    struct orbit_flash_120x120_palette_16 {
        static constexpr uint8_t target;       // 位元旗標：video::TARGET_FLASH = 1、TARGET_STREAM = 2，兩者都有就是 3
        static constexpr uint8_t width, height; // 存的大小（放大前）
        static constexpr uint8_t scaleX, scaleY;
        static constexpr uint8_t colorMode;    // video::COLORS_16 = 0 / COLORS_256 = 1 / COLORS_565 = 2
        static constexpr uint16_t colorCount;  // 調色盤顏色數，565 是 0
        static constexpr uint16_t frameCount;
        static constexpr uint32_t durationMs;  // 一輪的總長
        static inline const char *getPath();   // 幀資料檔的路徑（相對於 lib/video），StreamPlayer 傳給 stream.py

        // 有幀資料的檔案（--target flash、flash stream）才編譯；--target stream 的在 #if 0 裡：
        static inline const uint16_t *getPalette();   // colorCount 個 RGB565
        static inline const uint16_t *getOffsets();   // 每一幀在 getFrames() 裡的起點（stream 超過 64 KB 時是 uint32_t）
        static inline const uint8_t *getFrames();     // 所有幀，格式見下
    };
}
```

- **`--target stream`** 也只有一個檔案 `output/x.h`：前面是板子編譯的上半部（大小、`getPath()`），後面的幀資料放在 `#if 0` 裡（太大，燒不進 Flash，所以永遠不編譯），`stream.py` 從同一個檔案讀。檔名的 `stream` 和 struct 的 `target` 都寫著它是串流用的。
- **檔案開頭**：
  - `// options: …`：轉換時的參數（`tools/assets.py` 靠它和 `source:` 找已經轉過的）。
  - `// format: 3`：格式版本。改格式時要調高 `check.py` 的 `FORMAT_VERSION`，舊檔案才會被抓出來。
  - `// source: … sha256 …`：原始檔和它的 hash。
  - `// settings:`：所有設定，包括原本的指令、裁切範圍、實際尺寸、顏色、幀率、資料量、估計的 fps，不必從檔名猜。

## 格式（format 3）

`decoder.h` 解碼，`convert.py` 編碼；`convert.py` 裡也有 Python 版的解碼器給 `stream.py` 用。Flash 和串流共用這個格式。多 byte 的數值都是 little endian。

```
幀：     [延遲 ms 低位][延遲 ms 高位][類型][窗口數]
         類型：0 = P 幀，1 = I 幀
窗口：   [x][y][w][h]   動畫座標（放大前），然後是一串 run，直到填滿 w×h 個像素
run：    依顏色模式不同
  COLORS_16   1 byte：高 4 bit = n，低 4 bit = 調色盤編號
              n = 1～15：n 個像素；n = 0：下一個 byte + 16 個像素（16～271）
  COLORS_256  2 bytes：[n][調色盤編號]，n + 1 個像素（1～256）
  COLORS_565  3 bytes：[n][RGB565 低位][RGB565 高位]，n + 1 個像素（1～256）
```

- 窗口內的像素一行一行排列。run 可以跨行，但不會跨窗口。
- **I 幀**：一個窗口蓋住整張動畫，不依賴上一幀。
- **P 幀**：只有變化的窗口，螢幕上其他地方保留上一幀。
- **空幀**：延遲 0、類型 P、窗口數 0。
- **放大**：每個點畫成 scaleX×scaleY。有放大時，板子先把一行解成顏色存起來（最多 120 個），再重複送出。

## 名稱和來源

每個 .h 的開頭都寫著它從哪個檔案、用什麼選項轉出來：

```
// options: --fit 120x80 --palette 16
// source: assets/sunset.jpg sha256 1413…
```

程式裡的 `ANIMATION("路徑", "選項")` 就是靠這兩行找到已經轉好的檔案：同一個檔案、同樣的選項（順序、`--fit=…` 的寫法不影響）就直接用，沒有才轉。所以不管是手動轉的還是巨集轉的，同一份只轉一次。

| 規則 | 說明 |
|---|---|
| 名稱 | `convert.py` 的規則：`{檔名}_{設定}`，struct 同名（`animation::名稱`） |
| `assets/` 外面的檔案 | 檔名前面加上資料夾（相對於專案；不在專案裡就相對於家目錄），例如 `~/Music/Game/title.mp3` → `music_game_title_…`，不同資料夾的同名檔案不會混在一起 |
| 不是英數字的字（例如日文） | 拿掉，改加 6 碼雜湊，例如 `16 戦い：ウォロ.aiff` → `…_16_7b5dc7_…` |
| 名稱撞到（名稱沒包含每個選項，或來源不同） | 加 `_2`、`_3`…，不會蓋掉別的檔案 |
| 原始檔改過 | 編譯時會警告；`python tools/assets.py --force` 重轉（同名的蓋掉） |
| 原始檔移走、刪掉 | 輸出還在就照樣能編譯（程式裡的舊路徑一樣找得到它） |
| 不要的 | `python tools/assets.py --unused` 列出沒有程式（包括 `.bak`）用的，自己刪 |
