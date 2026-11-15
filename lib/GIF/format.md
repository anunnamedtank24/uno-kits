# GIF 資料格式與串流協定

## 幀格式（format 3）

`decoder.h` 解碼；`convert.py` 編碼（也有 Python 版的解碼給 `stream.py` 用）。Flash 和串流共用。多 byte 的數值都是 little endian。

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
- **空幀**：延遲 0、類型 P、窗口數 0，串流用來告訴板子連線還在。
- **放大**：每個點畫成 scaleX×scaleY。有放大時板子先把一行解成顏色存起來（最多 120 個），再重複送出。

### 產生的 struct（`output/*.h`）

```cpp
namespace animation {
    struct orbit_uno_120x120_palette_16 {
        static constexpr uint8_t target;       // 位元旗標：GIF::TARGET_UNO = 1、TARGET_MAC = 2，兩者都有就是 3
        static constexpr uint8_t width, height; // 存的大小（放大前）
        static constexpr uint8_t scaleX, scaleY;
        static constexpr uint8_t colorMode;    // GIF::COLORS_16 = 0 / COLORS_256 = 1 / COLORS_565 = 2
        static constexpr uint16_t colorCount;  // 調色盤顏色數，565 是 0
        static constexpr uint16_t frameCount;
        static constexpr uint32_t durationMs;  // 一輪的總長
        static inline const char *getPath();   // 幀資料檔的路徑（相對於 lib/GIF），StreamPlayer 傳給 stream.py

        // 有幀資料的檔案（--target uno、uno mac，以及 output/mac/ 裡的檔案）才有：
        static inline const uint16_t *getPalette();   // colorCount 個 RGB565
        static inline const uint16_t *getOffsets();   // 每一幀在 getFrames() 裡的起點（mac 超過 64 KB 時是 uint32_t）
        static inline const uint8_t *getFrames();     // 所有幀，格式同上
    };
}
```

- **`--target mac`**：產生兩個同名的 struct。
  - `output/x.h`：只有上半部，加進 `selected.h`。
  - `output/mac/x.h`：完整的，`getPath()` 指向它，只有 `stream.py` 會讀，永遠不會被 include。
- **給 `check.py` 用的行**：檔案開頭的 `// format: 3` 和 `// source: … sha256 …`。描述檔還會檢查它指到的幀資料檔。
- **改格式時**：要調高 `check.py` 的 `FORMAT_VERSION`，舊檔案才會被抓出來。
- **給人看的設定**：`// settings:` 區塊列出所有設定，不必從檔名猜。

## 轉換流程（convert.py）

1. **讀 GIF**：用 Pillow 讀出每一幀，已經處理好疊加和清除規則。
2. **降幀率**（`--max-fps`）：丟掉間隔太短的幀，時間加給前一幀。
3. **尺寸**：
   - 算出螢幕上的大小（`--width`、`--height`、`--resize`、`--fit`、`--fill`）。
   - `--fill` 先從中間裁掉多出的部分。
   - 除以 `--scale` 得到要存的大小，用 `--resample` 縮放，透明的地方填背景色。
4. **顏色**：
   - 565：每個像素直接轉 RGB565。
   - 16 / 256：所有幀合起來用 octree 法做一個共用調色盤；`--dither` 用 Floyd–Steinberg 抖色，否則每個像素取最接近的顏色。
5. **編碼每一幀**：第 0 幀和 `--keyframe` 指定的幀是 I 幀，其他幀比較三種畫法，選成本最低的：
   - **變化窗口**：畫面切成 `--tile` 大小的小方塊，有變化的方塊合併成矩形，再縮小到實際變化的範圍。
   - **單一窗口**：包住所有變化的最小矩形。
   - **整張重畫**：標成 I 幀，FlashPlayer 落後時也可以跳到這裡。
6. **成本**：畫圖時間 + 資料量 × 每 byte 成本，數值都在 `config.json`：
   - 每個窗口約 480 週期，每個螢幕像素約 44 週期，每個 run 約 30 週期。
   - 每 byte：uno 讀 Flash 約 6 週期；mac 經串口約 176 週期（1 Mbaud）。所以串流會偏向資料量少的畫法。
7. **`--max-bytes`**：資料太大時，依 `--reduce` 把幀率或尺寸乘以 0.85 再重做，直到放得下。幀率不低於 `--min-fps`，尺寸不小於 4 px。

## FlashPlayer 的時間控制

- 第 0 幀在 `update()` 第一次被呼叫時畫出，之後每一幀在「前面所有幀的延遲加總」時到期。
- 落後超過一幀時，往後找已經到期的幀：
  - 中間有 I 幀：跳到最後一個已經到期的 I 幀，跳過的幀數累加到 `skippedFrames()`。
  - 中間沒有 I 幀：照順序畫下一幀，並從現在重新計時。動畫會變慢，但不會畫錯。

## 串流協定

板子決定要播哪個檔案和怎麼播，Mac 負責計時和送幀。原則：**Mac 主動送的資料永遠不超過 1 byte**。UNO 的 UART 只能暫存 2 個 byte，板子在畫圖或做別的事時，多送的資料會遺失。

### 板子 → Mac：指令

| 指令 | 參數 | Mac 的回答 |
|---|---|---|
| `'O'` 開啟 | CHUNK、旗標（bit 0 播放、bit 1 循環）、速度（uint16 %）、起始位置（uint32 ms）、路徑長度、路徑 | `'G'` 或 `'E'` |
| `'N'` 要下一幀 | — | `'F'`（有新幀時）或 `'K'`（沒有新幀） |
| `'P'` 播放 / `'S'` 暫停 | — | `'A'` |
| `'J'` 跳轉 | 位置（uint32 ms） | `'A'` |
| `'V'` 速度 | 百分比（uint16，1～1000） | `'A'` |
| `'L'` 循環 | 0 / 1 | `'A'` |
| `'R'` 拉一塊 | — | 目前回答的下一塊 |

### Mac → 板子

1. **通知**：回答準備好時，Mac 只送 1 byte 的 `'D'`。
2. **拉取**：板子有空時送 `'R'`，Mac 送一塊 `[長度 1～CHUNK][資料]`；板子處理完再送 `'R'` 拿下一塊。
3. **下一個回答**：前一個回答被拉完之後，Mac 才會送下一個 `'D'`。

回答的第一個 byte 是種類：

| 回答 | 內容 |
|---|---|
| `'G'` 標頭 | 狀態、寬、高、scaleX、scaleY、顏色模式、色數、幀數（uint16）、總長（uint32 ms）、調色盤（只有 16 色模式） |
| `'F'` 幀 | 狀態 + 一幀（格式見上） |
| `'K'` 沒有新幀 | 狀態。暫停、播完、或下一幀還很久時，最多每秒一次；狀態改變時（例如播完）立刻送。板子收到後再送 `'N'` |
| `'A'` 指令完成 | 狀態 |
| `'E'` 錯誤 | 代碼（1 找不到、2 讀不懂、3 256 色不能串流）、訊息長度、訊息 |

**狀態**（7 bytes）：旗標（bit 0 播放中、bit 1 已結束）、位置（uint32 ms）、幀號（uint16）。

### 計時（Mac）

- **位置**：位置 = 起點 + （現在 − 開始時間）× 速度。暫停時位置不動。循環時超過總長就從頭；不循環時停在結尾，狀態變成已結束。
- **送哪一幀**：收到 `'N'` 後，只要「現在位置該顯示的幀」跟螢幕上的不同，就送那一幀跟螢幕上那幀的差異。落後的幀直接跳過，不需要 I 幀。
- **跳轉、改速度**：只改時鐘，下一個 `'N'` 自然會送新位置的那一幀。

### 錯誤處理（板子）

- **重連**：下面這些情況板子會重新送 `'O'`，帶著目前的位置、播放狀態、速度和循環設定，所以會從原本的地方接著播：
  - 3 秒內沒收到任何回答。
  - 一塊資料在 200 ms 內沒收完。
  - 指令 1 秒內沒有回答。
  - 回答的種類不對。
  - 窗口超出動畫範圍。
- **`'E'`**：記下錯誤代碼，等 3 秒再重試，因為檔案可能晚一點才產生。
- **競爭**：板子送控制指令時，Mac 可能剛好在送幀。板子會先把那一幀畫完、再要下一幀，然後繼續等指令的 `'A'`；Mac 也會等那一幀被拉完，才回答指令。
