# font

| 需要 | 得到 |
|---|---|
| （不需要）；字型檔 .ttf / .otf | `font::*`：每個字型一個 struct（`FONT("檔案")` 或 `source/convert.py` 轉出來），給 `graphics::Text` 的 `setFont<…>()` |

不需要任何介面，任何主板都能用。開發者看的細節（前提條件、資料怎麼產生、輸入限制）在 [INTERFACE.md](INTERFACE.md)。

字型資料，`namespace font`。每個字型是一個 struct，由 `source/convert.py` 從 .ttf / .otf 產生，給 `graphics::Text` 畫。

## 用法

```bash
cd lib/font
python source/convert.py assets/menlo.ttf output --height 23
python source/check.py
```

```cpp
#include "font.h"
screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
screen.drawTexts(10, 10, color, "hello");
```

完整的例子：`src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp`（ST7789）、`src/week_4_SSD1306_switch_buzzer_audio/tests/1_oled.cpp`（OLED）。

## 直接用檔案（推薦）

程式裡寫 `FONT("路徑", "選項")`，編譯前自動轉換，不用先跑 `convert.py`，檔案也不用放在 `assets/`：

```cpp
#include "font.h"
using Title = FONT("~/Fonts/Inter.ttf", "--height 16");
screen.setFont<Title>();
```

| 項目 | 說明 |
|---|---|
| 路徑 | 絕對路徑、`~/…`、相對於寫它的 .cpp；找不到時再試專案資料夾、`lib/font/assets/` |
| 選項 | 一個字串，就是 `source/convert.py` 的選項：大小（`--height` / `--width` / `--size`）、`--first`、`--last`、`--default`；省略大小時是 `--height 16` |
| 轉到哪 | `lib/font/output/`，跟手動跑 `convert.py` 的一樣（`tools/assets.py` 每次編譯前找：同一個檔案、同樣的選項已經轉過就直接用，沒有才轉） |
| 型態 | `FONT(...)` 就是那個 `font::` struct，跟手動轉換的一樣用 |
| 限制 | 兩個參數都要是字串常數（不能是變數） |

完整的說明：[lib/README.md](../README.md#字型圖片影片聲音直接用檔案)；例子：`src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp`（四種都有）。上面「用法」是手動轉換的方式，一樣能用。

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
