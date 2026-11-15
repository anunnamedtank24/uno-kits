# font

字型資料，`namespace font`。每個字型是一個 struct，由 `source/convert.py` 從 .ttf/.otf 產生，給 `graphics::Text` 用。

| 文件 | 內容 |
|---|---|
| 這份 | 資料夾、流程、用法 |
| [source/README.md](source/README.md) | `convert.py` 的參數和形狀演算法、`check.py` |
| [output/README.md](output/README.md) | `selected.h`、產生的 struct、資料格式 |

## 資料夾

```
lib/font/
├── font.h             入口：include output/selected.h
├── assets/            原始字型（.ttf / .otf）
├── output/            產生的字型 + selected.h
└── source/            convert.py、check.py
```

## 流程

```bash
cd lib/font
python -B source/convert.py assets/menlo.ttf output --height 23
python -B source/convert.py assets/menlo.ttf output --height 46 --first 48 --last 57
python -B source/check.py
```

```cpp
#include "font.h"
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
```

- **`convert.py` 只加剛產生的那一個**：把它加進 `output/selected.h`，其他字型保持原樣，最後跑一次檢查。
- **`selected.h` 可以手動改**：它決定程式裡「能用」哪些字型，不用的刪掉那行 `#include` 即可。
- **Flash 只算真的用到的**：資料放在 inline 函式裡，程式沒有 `setFont<…>` 到的字型會被編譯器丟掉，留在 `selected.h` 也不佔 Flash（實測多選一個沒用到的 5.4 KB 資料，Flash 不變）。

## 限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 字元碼 | 0 ～ 255，一個 byte 一個字 | UTF-8 的中文等多 byte 字元會拆成好幾個 byte，各自畫成預設字元（字型範圍到 255 時則畫成 Latin-1 的字） |
| `--first` ～ `--last` | 0 ～ 255，first ≤ last | `convert.py` 報錯 |
| 格子高度 `gridHeight`、每個字的寬度 | ≤ 255 px（`uint8_t`） | `convert.py` 報錯 |
| 每個字的點、橫線、直線、矩形數量 | 各 ≤ 255 | `convert.py` 報錯 |
| 整個字型的形狀資料 | ≤ 65535 bytes（`uint16_t` 位移） | `convert.py` 報錯，縮小字型或字元範圍 |
| 預設字元 `--default` | 必須在字型裡 | `convert.py` 報錯 |
| 字型檔 | .ttf、.otf | `convert.py` 報錯 |
| 字元範圍外的符號（例如 °） | first ～ last 必須連續；`assets/menlo.ttf` 只有 ASCII，沒有 ° | 要用的話，先用 fontTools 從系統字型取出需要的字再轉換 |

- **只畫字的像素**：沒有背景色，抗鋸齒的灰階也沒有，每個像素不是有就是沒有。
- **等寬或不等寬都可以**：每個字有自己的寬度（`getWidths()`），高度都是 `gridHeight`。
