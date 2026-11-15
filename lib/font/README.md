# font

字型資料，`namespace font`。每個字型是一個 struct，由 `source/convert.py` 從 .ttf / .otf 產生，給 `graphics::Text` 畫。不依賴任何東西。

## 檔案

```
lib/font/
├── font.h      入口：include output/selected.h
├── assets/     原始字型（menlo.ttf）
├── source/     convert.py、check.py，見 source/README.md
└── output/     產生的字型和 selected.h，見 output/README.md
```

## 用法

```bash
cd lib/font
python -B source/convert.py assets/menlo.ttf output --height 23
python -B source/check.py
```

```cpp
#include "font.h"
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
screen.drawTexts(10, 10, color, "hello");
```

完整的例子：`src/week_3/test_1.cpp`（ST7789）、`src/week_4/test_1.cpp`（OLED）。

## 流程

| 步驟 | 說明 |
|---|---|
| `convert.py` | 產生 `output/x.h`，只把它加進 `output/selected.h`（其他的保持原樣），最後跑一次檢查 |
| `selected.h` | 決定程式裡能用哪些字型；不用的刪掉那行 `#include` 就好 |
| 編譯 | 只有程式 `setFont<…>` 到的字型會進 Flash（實測多選一個沒用到的 5.4 KB 字型，Flash 不變） |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 字元碼 | 0 ～ 255，一個 byte 一個字 | UTF-8 的中文等多 byte 字元拆成好幾個 byte，各自畫成預設字元 |
| `--first` ～ `--last` | 0 ～ 255，first ≤ last，連續 | `convert.py` 報錯 |
| 格子高度、每個字的寬度 | ≤ 255 px | `convert.py` 報錯 |
| 每個字的點、橫線、直線、矩形數量 | 各 ≤ 255 | `convert.py` 報錯 |
| 整個字型的形狀資料 | ≤ 65535 bytes | `convert.py` 報錯：縮小字型或字元範圍 |
| 預設字元 `--default` | 必須在字型裡 | `convert.py` 報錯 |
| 字型檔 | .ttf、.otf | `convert.py` 報錯 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 字型裡沒有的符號（例如 `°`） | `assets/menlo.ttf` 只有 ASCII | 先用 fontTools 從系統字型取出需要的字再轉換 |
| 抗鋸齒 | 沒有：每個像素不是有就是沒有，也沒有背景色 | 要蓋掉舊字，先自己畫背景 |
| 小字（12 px） | 只有 4 px 寬，看起來很粗糙 | 用 23 px 以上 |
| 等寬或不等寬 | 都可以：每個字有自己的寬度，高度都是 `gridHeight` | — |
