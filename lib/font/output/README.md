# font/output

`source/convert.py` 和程式裡的巨集產生的字型，`namespace font`。這裡的檔案都不要手動編輯，要改就重新轉換。

## 檔案

```
output/
├── selected.h    自動寫：列出這裡全部的 .h，font.h 會 include 它
├── registry.h    自動寫：目前程式裡 FONT("路徑", "選項") 對應的型態
└── *.h           轉好的字型：手動跑 convert.py 或 FONT(...) 轉的都在這裡，格式一樣
```

## selected.h、registry.h

- 都是 `tools/assets.py` 每次編譯前寫的（`convert.py` 也會把新轉的加進 `selected.h`），不用手動編輯。
- `selected.h` 列出**全部**：沒用到的不佔 Flash（資料在 `inline` 函式裡的 `PROGMEM` 陣列，沒呼叫就不會產生，實測 Flash 一樣）。
- `registry.h` 只有目前程式裡的 `FONT(...)`。
- `python source/check.py` 檢查每個檔案都是目前的格式、原始檔沒變過；`convert.py` 每次轉完會自動檢查一次。

## 目前的字型

| 檔案 | 指令 | 每個字 |
|---|---|---|
| `menlo_height_12_first_32_last_126_default_63.h` | `--height 12` | 約 4×12，太粗糙，不建議 |
| `menlo_height_23_first_32_last_126_default_63.h` | `--height 23` | 8×23：OLED 一行 16 個字、2 行 |
| `menlo_height_46_first_48_last_57_default_63.h` | `--height 46 --first 48 --last 57` | 只有數字 0 ～ 9，大字 |
| `menlo_width_12_first_32_last_126_default_63.h` | `--width 12` | 最寬的字 12 px |
| `system_library_fonts_supplemental_arial_bold_height_16_first_32_last_126_default_63.h` | `FONT("/System/Library/Fonts/Supplemental/Arial Bold.ttf", "--height 16")`（`src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp`） | macOS 內建的 Arial 粗體，16 px 高；`assets/` 外面的檔案，所以名稱前面有資料夾 |

## 產生的 struct

```cpp
namespace font {
    struct menlo_height_23_first_32_last_126_default_63 {
        static constexpr uint8_t gridHeight;     // 每個字的高度
        static constexpr uint8_t first, last;    // 字元碼範圍
        static constexpr uint8_t defaultChar;    // 範圍外畫成這個字
        static constexpr uint8_t defaultIndex;   // 它在 widths / offsets 裡的位置
        static inline const uint8_t *getWidths();    // 每個字的寬度
        static inline const uint16_t *getOffsets();  // 每個字在 getShapes() 裡的起點
        static inline const uint8_t *getShapes();    // 所有字的形狀，格式見下
    };
}
```

**檔案開頭**：

```
// menlo.ttf --height 23 --first 32 --last 126 --default '?'     ← 產生時的參數
// format: 2                                                    ← 格式版本
// source: assets/menlo.ttf sha256 …                            ← 原始檔和它的 hash
```

改格式時要調高 `check.py` 的 `FORMAT_VERSION`，舊檔案才會被抓出來。

## 格式（format 2：形狀）

- **每個字**：先是 4 個數量（點、橫線、直線、矩形），接著依序是：
  - 點：`(x, y)`
  - 橫線：`(x, y, w)`
  - 直線：`(x, y, h)`
  - 矩形：`(x, y, w, h)`
- **座標**：相對於字的左上角，單位 px。
- **畫法**：`graphics::Text` 依序呼叫 `drawPoint`、`drawHLineSize`、`drawVLineSize`、`drawFilledRectSize`。

## 名稱和來源

每個 .h 的開頭都寫著它從哪個檔案、用什麼選項轉出來：

```
// options: --fit 120x80 --palette 16
// source: assets/sunset.jpg sha256 1413…
```

程式裡的 `FONT("路徑", "選項")` 就是靠這兩行找到已經轉好的檔案：同一個檔案、同樣的選項（順序、`--fit=…` 的寫法不影響）就直接用，沒有才轉。所以不管是手動轉的還是巨集轉的，同一份只轉一次。

| 規則 | 說明 |
|---|---|
| 名稱 | `convert.py` 的規則：`{檔名}_{設定}`，struct 同名（`font::名稱`） |
| `assets/` 外面的檔案 | 檔名前面加上資料夾（相對於專案；不在專案裡就相對於家目錄），例如 `~/Music/Game/title.mp3` → `music_game_title_…`，不同資料夾的同名檔案不會混在一起 |
| 不是英數字的字（例如日文） | 拿掉，改加 6 碼雜湊，例如 `16 戦い：ウォロ.aiff` → `…_16_7b5dc7_…` |
| 名稱撞到（名稱沒包含每個選項，或來源不同） | 加 `_2`、`_3`…，不會蓋掉別的檔案 |
| 原始檔改過 | 編譯時會警告；`python tools/assets.py --force` 重轉（同名的蓋掉） |
| 原始檔移走、刪掉 | 輸出還在就照樣能編譯（程式裡的舊路徑一樣找得到它） |
| 不要的 | `python tools/assets.py --unused` 列出沒有程式（包括 `.bak`）用的，自己刪 |
