# font

字型資料，`namespace font`。每個字型是一個 struct，由 `source/convert.py` 從 .ttf/.otf 產生，給 `graphics::Text` 用。

## 資料夾

```
lib/font/
├── font.h             入口：include output/selected.h
├── assets/            原始字型（.ttf / .otf）
├── output/
│   ├── selected.h     要編譯進程式的字型清單（可以手動編輯）
│   └── *.h            產生的字型
└── source/
    ├── convert.py     .ttf → output/*.h，並把新產生的那個加進 selected.h
    └── check.py       檢查 selected.h
```

## 流程

```bash
cd lib/font
python source/convert.py assets/menlo.ttf output --height 23
python source/convert.py assets/menlo.ttf output --height 46 --first 48 --last 57
python source/check.py
```

```cpp
#include "font.h"
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
```

- **`convert.py` 只加剛產生的那一個**：把它加進 `output/selected.h`，其他字型保持原樣，最後跑一次檢查。檢查不通過時結束碼是 1。
- **`selected.h` 可以手動改**：它決定程式裡「能用」哪些字型，不用的刪掉那行 `#include` 即可。`output/` 裡沒被選到的字型只會出警告。
- **Flash 只算真的用到的**：資料放在 inline 函式裡，程式沒有 `setFont<…>` 到的字型會被編譯器丟掉，留在 `selected.h` 也不佔 Flash（實測多選一個沒用到的 5.4 KB 資料，Flash 不變）。

## check.py 檢查什麼

| 類型 | 項目 |
|---|---|
| 錯誤 | `selected.h` 不存在或沒有 include guard |
| 錯誤 | include 的檔案不存在、不在 `output/` 裡、include 自己、重複 include |
| 錯誤 | 檔案是舊版 convert.py 產生的（沒有 `// format:` 行，或版本不是目前的 `FORMAT_VERSION`） |
| 錯誤 | 檔案裡沒有 `struct font::{檔名}`（例如檔案被改過名字） |
| 錯誤 | 原始字型跟產生時不一樣了（比對產生時記下的 sha256） |
| 警告 | 原始字型找不到（沒辦法確認有沒有變） |
| 警告 | `output/` 裡有沒被選到的檔案 |

每個產生的檔案開頭都有這幾行，check.py 就是靠它們判斷：

```
// menlo.ttf --height 23 --first 32 --last 126 --default '?'     ← 產生時的參數
// format: 2                                                    ← 格式版本
// source: assets/menlo.ttf sha256 …                            ← 原始檔和它的 hash
```

## convert.py 參數

| 參數 | 說明 |
|---|---|
| `--height` / `--width` / `--size` | 三選一：格子高度、最寬字元的寬度，或直接指定 FreeType 的字型大小 |
| `--first` / `--last` | 字元碼範圍，預設 32～126 |
| `--default` | 範圍外的字元畫成哪個字，預設 `?`。字型裡沒有這個字就報錯 |

輸出檔名：`{字型}_{參數名}_{值}_first_{f}_last_{l}_default_{碼}.h`，struct 也是同樣的名字。

## 資料格式（format 2：形狀）

- **高度和寬度**：`gridHeight` 是固定的，每個字有自己的寬度（`getWidths()`）。
- **每個字的資料**：先是 4 個數量（點、橫線、直線、矩形），接著依序是：
  - 點：`(x, y)`
  - 橫線：`(x, y, w)`
  - 直線：`(x, y, h)`
  - 矩形：`(x, y, w, h)`
- **`getOffsets()`**：每個字的資料在 `getShapes()` 裡從哪個位置開始。
- **形狀的選法**：形狀剛好蓋住字的像素，可以互相重疊，但絕不畫到字外面。用畫圖時間模型找最便宜的組合：
  1. 找出所有最大矩形（上下左右都不能再長）。
  2. 用分支定界找成本最低的組合。
  3. 把重疊的部分盡量縮掉。
  4. 依大小分成點、橫線、直線、矩形。

  畫圖時間模型：每個形狀約 480 週期、每個像素約 44 週期、每個儲存 byte 約 6 週期（UNO，估算值）。
