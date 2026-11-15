# UNO + ST7789

Arduino UNO 驅動 1.3 吋 240×240 ST7789 螢幕（M130T 模組）的函式庫：圖形、文字、GIF 動畫，動畫可以存在 Flash，也可以從電腦串流。另外有靜態圖片（jpg / png / webp / heic，從 Flash 或 Mac）、I2C OLED（SSD1306）和 DS18B20 溫度感測器；OLED 用同一套圖形、文字和圖片。

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
│   ├── GIF/           GIF 動畫（.gif → .h，Flash 播放或從電腦串流）
│   ├── temp_sensor/   DS18B20 溫度感測器（1-Wire）
│   ├── I2C/           I2C OLED（SSD1306）：指令 + 板子 RAM 裡的畫面
│   ├── image/         靜態圖片（.jpg / .png / .webp / .heic → .h，Flash 或從 Mac 串流）
│   └── switch/        開關（GND、VCC、OUT）
└── src/
    ├── hide.py        只留一個程式，其他的改成 .bak
    ├── week_3/        M130T 測試、溫度感測器、彩色圖片、作業，見 src/week_3/README.md
    └── week_4/        OLED、黑白圖片、開關的測試和作業，見 src/week_4/README.md
```

## 開始

需要 [PlatformIO](https://platformio.org/)（VS Code 外掛或 CLI）。

```bash
pio run                  # 編譯
pio run -t upload        # 燒錄
```

- `src/` 裡只能有一個程式有 `setup()` / `loop()`，其他的要改成 `.bak`：`python -B src/hide.py week_4/test_1` 只留下那一個。
- 接線見 [src/week_3/README.md](src/week_3/README.md#接線)（M130T）、[src/week_4/README.md](src/week_4/README.md#接線)（OLED）。

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
| `lib/image/source/` | [image/source/README.md](lib/image/source/README.md)（HEIC 可以另外裝 `pillow-heif`，沒裝就用 macOS 的 `sips`） |

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
| [lib/temp_sensor/README.md](lib/temp_sensor/README.md) | DS18B20 溫度感測器：接線、函式、解析度、錯誤 |
| [lib/I2C/README.md](lib/I2C/README.md) | I2C OLED：接線、指令表、為什麼要在 RAM 留畫面、速度 |
| [lib/image/README.md](lib/image/README.md) | 圖片：支援的檔案、顏色、Flash / 串流、在 OLED 上畫；[source/](lib/image/source/README.md) 轉換和串流協定、[output/](lib/image/output/README.md) 格式 |
| [src/week_3/README.md](src/week_3/README.md) | M130T 測試、溫度感測器、彩色圖片、作業一～三 |
| [lib/switch/README.md](lib/switch/README.md) | 開關：接線、去彈跳、`ACTIVE_HIGH` |
| [src/week_4/README.md](src/week_4/README.md) | OLED、彩色圖片、開關的測試，作業一～三 |
