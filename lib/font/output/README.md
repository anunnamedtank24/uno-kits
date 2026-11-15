# font/output

`source/convert.py` 產生的字型，`namespace font`。除了 `selected.h` 可以手動改，其他檔案都不要手動編輯，要改就重新轉換。

## selected.h

- `font.h` 會 include 它，它再 include 要用的字型。
- `convert.py` 每次只把剛產生的那個加進來；不用的刪掉那行 `#include` 就好。
- `python -B source/check.py` 檢查每個 include（規則見 [source/README.md](../source/README.md#checkpy)）。

## 檔案開頭

```
// menlo.ttf --height 23 --first 32 --last 126 --default '?'     ← 產生時的參數
// format: 2                                                    ← 格式版本
// source: assets/menlo.ttf sha256 …                            ← 原始檔和它的 hash
```

改格式時要調高 `check.py` 的 `FORMAT_VERSION`，舊檔案才會被抓出來。

## 資料格式（format 2：形狀）

- **高度和寬度**：`gridHeight` 是固定的，每個字有自己的寬度（`getWidths()`）。
- **`getOffsets()`**：每個字的資料在 `getShapes()` 裡從哪個位置開始。
- **每個字的資料**：先是 4 個數量（點、橫線、直線、矩形），接著依序是：
  - 點：`(x, y)`
  - 橫線：`(x, y, w)`
  - 直線：`(x, y, h)`
  - 矩形：`(x, y, w, h)`
- **畫法**：`graphics::Text` 依序呼叫 `drawPoint`、`drawHLineSize`、`drawVLineSize`、`fillRectSize`。
