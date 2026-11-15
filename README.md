# UNO + ST7789

Arduino UNO 驅動 1.3 吋 240×240 ST7789 螢幕（M130T 模組）的函式庫：圖形、文字、GIF 動畫，動畫可以存在 Flash，也可以從電腦串流。

- 用 template 做硬體抽象，編譯後跟直接寫暫存器一樣快。
- 字型和動畫由 Python 工具事先轉好，並依 UNO 的畫圖時間挑最快的畫法。
- **圖形和文字會裁切**：超出螢幕的部分不畫（只在最底層的 `fillWindow` / `writeWindow` 裁一次）。GIF 不裁切，開始播放時檢查整個動畫在不在螢幕上。各 README 的「輸入限制」寫了每個參數的範圍。

## 資料夾

```
.
├── platformio.ini     PlatformIO 設定（Arduino UNO，C++17）
├── lib/               函式庫，見 lib/README.md
│   ├── Uno/           ATmega328P：SPI 螢幕 Bus、USB 串口
│   ├── ST7789/        ST7789 驅動 + 模組（M130T）
│   ├── graphics/      圖形和文字
│   ├── color/         RGB565 顏色
│   ├── font/          字型（.ttf → .h）
│   └── GIF/           GIF 動畫（.gif → .h，Flash 播放或從電腦串流）
└── src/
    └── test/          在板子上跑的完整測試，見 src/test/README.md
```

## 開始

需要 [PlatformIO](https://platformio.org/)（VS Code 外掛或 CLI）。

```bash
pio run                  # 編譯
pio run -t upload        # 燒錄
```

- `src/` 裡只能有一個程式有 `setup()` / `loop()`，其他的要先移走，或把副檔名改掉。
- 接線見 [src/test/README.md](src/test/README.md#接線)。

```cpp
#include "Uno_ST7789.h"
#include "module/M130T.h"
#include "graphics.h"
#include "color.h"
#include "font.h"

using Bus = Uno::UnoST7789<8, 10, Uno::NONE, 9>;          // DC, RES, CS, BLK
using Screen = graphics::Canvas<ST7789::M130T<Bus>>;
Screen screen;

void setup() {
    screen.init();
    screen.drawFilledScreen(color::RGB565::BLACK);
    screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
    screen.drawTexts(10, 10, color::RGB565::WHITE, "hello ", 42);
}

void loop() {
}
```

## Python 工具

字型和 GIF 的轉換、串流在電腦上跑，需要 Python 3：

```bash
pip install pillow pyserial
```

| 工具 | 說明 |
|---|---|
| `lib/font/source/` | [font/source/README.md](lib/font/source/README.md) |
| `lib/GIF/source/` | [GIF/source/README.md](lib/GIF/source/README.md) |

用 `python -B` 執行，不會產生 `__pycache__`。

## 文件

| 文件 | 內容 |
|---|---|
| [lib/README.md](lib/README.md) | 函式庫總覽、依賴關係、**比較表**（輸入怎麼處理、哪些有裁切、浮點、RAM、時間）、共同的寫法 |
| [lib/Uno/README.md](lib/Uno/README.md) | 腳位、SPI、串口、編譯期檢查 |
| [lib/ST7789/README.md](lib/ST7789/README.md) | 驅動、Bus 介面；[module/](lib/ST7789/module/README.md) 加新模組 |
| [lib/graphics/README.md](lib/graphics/README.md) | 圖形和文字的函式 |
| [lib/color/README.md](lib/color/README.md) | 顏色 |
| [lib/font/README.md](lib/font/README.md) | 字型；[source/](lib/font/source/README.md) 轉換、[output/](lib/font/output/README.md) 格式 |
| [lib/GIF/README.md](lib/GIF/README.md) | 播放器；[source/](lib/GIF/source/README.md) 轉換和串流協定、[output/](lib/GIF/output/README.md) 格式 |
| [src/test/README.md](src/test/README.md) | 測試清單 |
