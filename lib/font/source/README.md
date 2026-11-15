# font/source

在電腦上跑的工具。需要 Python 3、`pip install pillow`。都在 `lib/font` 裡執行，加 `-B` 不產生 `__pycache__`。

## convert.py

```bash
python -B source/convert.py assets/menlo.ttf output --height 23
```

| 參數 | 說明 |
|---|---|
| `--height` / `--width` / `--size` | 三選一：格子高度、最寬字元的寬度，或直接指定 FreeType 的字型大小 |
| `--first` / `--last` | 字元碼範圍，預設 32～126 |
| `--default` | 範圍外的字元畫成哪個字，預設 `?`。字型裡沒有這個字就報錯 |

- **輸出檔名**：`{字型}_{參數名}_{值}_first_{f}_last_{l}_default_{碼}.h`，struct 也是同樣的名字。
- **做完之後**：把新檔案加進 `output/selected.h`，然後跑 `check.py`。

### 形狀的選法

每個字存成一組形狀（點、橫線、直線、矩形）。形狀剛好蓋住字的像素，可以互相重疊，但絕不畫到字外面。用畫圖時間模型找最便宜的組合：

1. 找出所有最大矩形（上下左右都不能再長）。
2. 用分支定界找成本最低的組合。
3. 把重疊的部分盡量縮掉。
4. 依大小分成點、橫線、直線、矩形。

**畫圖時間模型**（UNO，估算值）：每個形狀約 480 週期、每個像素約 44 週期、每個儲存 byte 約 6 週期。跟一個一個點畫比，小字約只要 30% 的時間、大字約 21%。

## check.py

| 類型 | 項目 |
|---|---|
| 錯誤 | `selected.h` 不存在或沒有 include guard |
| 錯誤 | include 的檔案不存在、不在 `output/` 裡、include 自己、重複 include |
| 錯誤 | 檔案是舊版 convert.py 產生的（沒有 `// format:` 行，或版本不是目前的 `FORMAT_VERSION`） |
| 錯誤 | 檔案裡沒有 `struct font::{檔名}`（例如檔案被改過名字） |
| 錯誤 | 原始字型跟產生時不一樣了（比對產生時記下的 sha256） |
| 警告 | 原始字型找不到（沒辦法確認有沒有變） |
| 警告 | `output/` 裡有沒被選到的 .h |

檢查不通過時結束碼是 1。
