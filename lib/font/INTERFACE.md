# font：介面

| 需要 | 得到 |
|---|---|
| （不需要）；字型檔 .ttf / .otf | `font::*`：每個字型一個 struct（`FONT("檔案")` 或 `source/convert.py` 轉出來），給 `graphics::Text` 的 `setFont<…>()` |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [讀我.md](讀我.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 依賴 | 沒有：只有資料（Flash 裡的陣列，屬性由 `platformio.ini` 決定）；畫字由 `graphics::Text` 負責 |
| 資料 | `font::*` 由 `convert.py` 產生，格式要是目前的版本 |
| 顏色 | 字是單色的形狀，用螢幕的顏色格式（`Screen::ColorFormat`）裡的任何一個顏色畫；背景不畫（透明） |

## 檔案

```
lib/font/
├── font.h      入口：asset.h + output/selected.h
├── asset.h     FONT(路徑, 選項) 巨集：編譯前自動轉換（src/tools/assets.py）
├── assets/     原始字型（menlo.ttf）
├── source/     convert.py、check.py，見 source/讀我.md
└── output/     產生的字型和 selected.h，見 output/讀我.md
```

## 流程

| 步驟 | 說明 |
|---|---|
| `convert.py` | 產生 `output/x.h`，把它加進 `output/selected.h`，最後跑一次檢查（`check.py`） |
| `src/tools/assets.py` | 每次編譯前：轉 `FONT(...)` 要的字型，再把 `selected.h` 重寫成 `output/` 的**全部** .h，`registry.h` 寫巨集對應的型態 |
| 編譯 | 只有程式 `setFont<…>` 到的字型會進 Flash（實測多列一個沒用到的 5.4 KB 字型，Flash 不變）；不要的字型直接刪掉 .h（`python src/tools/assets.py --unused` 列出沒用到的） |

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
