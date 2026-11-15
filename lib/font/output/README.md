# font/output

`source/convert.py` 產生的字型，`namespace font`。除了 `selected.h` 可以手動改，其他檔案都不要手動編輯，要改就重新轉換。

## 檔案

```
output/
├── selected.h    程式裡能用的字型，font.h 會 include 它
└── *.h           產生的字型
```

## selected.h

- `convert.py` 每次只把剛產生的那個加進來；不用的刪掉那行 `#include` 就好。
- `python -B source/check.py` 檢查每個 include（規則見 [source/README.md](../source/README.md#checkpy)）。

## 目前的字型

| 檔案 | 指令 | 每個字 |
|---|---|---|
| `menlo_height_12_first_32_last_126_default_63.h` | `--height 12` | 約 4×12，太粗糙，不建議 |
| `menlo_height_23_first_32_last_126_default_63.h` | `--height 23` | 8×23：OLED 一行 16 個字、2 行 |
| `menlo_height_46_first_48_last_57_default_63.h` | `--height 46 --first 48 --last 57` | 只有數字 0 ～ 9，大字 |
| `menlo_width_12_first_32_last_126_default_63.h` | `--width 12` | 最寬的字 12 px |

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
