# video/output

`source/convert.py` 產生的動畫，`namespace animation`。除了 `selected.h` 可以手動改，其他檔案都不要手動編輯，要改就重新轉換。

## 檔案

```
output/
├── selected.h     程式裡能用的動畫，video.h 會 include 它
├── *.h            給板子的檔案：uno 有幀資料，mac 只有尺寸和路徑
└── mac/*.h        mac 的幀資料，只有 stream.py 會讀，永遠不會被 include
```

## selected.h

- `convert.py` 每次只把剛產生的那個加進來，其他的保持原樣。
- 不用的動畫刪掉那行 `#include` 就好；就算留著，程式沒用到也不佔 Flash。
- `python -B source/check.py` 檢查每個 include 都是目前格式、原始檔沒變過（規則見 [source/README.md](../source/README.md#checkpy)）。

## 目前的動畫

都從 `assets/orbit.gif`（120×120、24 幀、1.44 秒、16.7 fps）轉出來：

| 檔案 | 指令 | 資料 | 每幀（估計） |
|---|---|---|---|
| `orbit_uno_120x120_palette_16.h` | `--target uno --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8` | 1992 B | 1.4 ～ 40.2 ms |
| `orbit_uno_64x64_palette_16_mono.h` | `--target uno --fit 64x64 --mono --keyframe 8`（OLED） | 882 B | 0.4 ～ 11.4 ms |
| `orbit_uno_mac_120x120_palette_16.h` | `--target mac uno --width 120 --scale 2` | 2352 B | 1.8 ～ 40.4 ms |
| `orbit_mac_120x120_palette_565.h` | `--target mac --fit 120x120 --palette 565` | 4658 B（在 `mac/`） | 2.4 ～ 54.0 ms（含傳送） |

## 產生的 struct

```cpp
namespace animation {
    struct orbit_uno_120x120_palette_16 {
        static constexpr uint8_t target;       // 位元旗標：video::TARGET_UNO = 1、TARGET_MAC = 2，兩者都有就是 3
        static constexpr uint8_t width, height; // 存的大小（放大前）
        static constexpr uint8_t scaleX, scaleY;
        static constexpr uint8_t colorMode;    // video::COLORS_16 = 0 / COLORS_256 = 1 / COLORS_565 = 2
        static constexpr uint16_t colorCount;  // 調色盤顏色數，565 是 0
        static constexpr uint16_t frameCount;
        static constexpr uint32_t durationMs;  // 一輪的總長
        static inline const char *getPath();   // 幀資料檔的路徑（相對於 lib/video），StreamPlayer 傳給 stream.py

        // 有幀資料的檔案（--target uno、uno mac，以及 mac/ 裡的檔案）才有：
        static inline const uint16_t *getPalette();   // colorCount 個 RGB565
        static inline const uint16_t *getOffsets();   // 每一幀在 getFrames() 裡的起點（mac 超過 64 KB 時是 uint32_t）
        static inline const uint8_t *getFrames();     // 所有幀，格式見下
    };
}
```

- **`--target mac`** 產生兩個同名的 struct：
  - `output/x.h`：只有上半部，加進 `selected.h`。
  - `output/mac/x.h`：完整的，`getPath()` 指向它。
- **檔案開頭**：
  - `// format: 3`：格式版本。改格式時要調高 `check.py` 的 `FORMAT_VERSION`，舊檔案才會被抓出來。
  - `// source: … sha256 …`：原始檔 和它的 hash。
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
