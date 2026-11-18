# UNO + ST7789

Arduino UNO 驅動 1.3 吋 240×240 ST7789 螢幕（M130T 模組）的函式庫：圖形、文字、動畫和影片（gif / mp4 / mov / webm…），動畫可以存在 Flash，也可以從電腦串流。另外有靜態圖片（jpg / png / webp / heic，從 Flash 或電腦）、I2C OLED（SSD1306）、DS18B20 溫度感測器、開關、蜂鳴器和旋律（從 wav / mp3 / m4a / ogg 等音檔轉）、紅外線遙控器、LED；OLED 用同一套圖形、文字、圖片和動畫。

- 用 template 做硬體抽象，編譯後跟直接寫暫存器一樣快。
- 字型、動畫、圖片、旋律：程式裡直接寫檔案（`FONT("~/x.ttf", "--height 16")`、`PICTURE(...)`、`ANIMATION(...)`、`MELODY(...)`），編譯前自動轉換；動畫和圖片依 UNO 的畫圖時間挑最快的畫法。
- 腳位、類比輸入、背景執行、馬達、感測器都有函式庫；每個介面都繼承同一個基底（`Uno::UnoBoard`）。
- 串流（`lib/stream`）和顏色格式（`lib/color`）也是 template 參數：電腦只開一個 `stream.py`；顏色格式只宣告在螢幕上（`M130T<Bus, color::RGB565>`），換成 RGB888、RGB444 也只改這裡。
- **每個函式庫的文件分兩份**：`README.md` 給使用者（用法、接線、注意事項），`INTERFACE.md` 給開發者（前提條件、介面需求、每個函式、參數、資源）。
- **圖形和文字會裁切**：超出螢幕的部分不畫（只在最底層的 `fillWindow` / `writeWindow` 裁一次）。動畫不裁切，開始播放時檢查整個動畫在不在螢幕上。各 README 的「輸入限制」寫了每個參數的範圍。

## 資料夾

```
.
├── platformio.ini     PlatformIO 設定（Arduino UNO，C++17；編譯前執行 tools/assets.py）
├── tools/
│   └── assets.py      編譯前自動轉換程式裡寫的字型、圖片、影片、聲音檔
├── lib/               函式庫，見 lib/README.md
│   ├── Uno/           ATmega328P：SPI 螢幕 Bus、USB 串口、1-Wire、軟體 I2C、輸入 / 輸出 / PWM / 中斷 / 類比腳、方波、背景執行
│   ├── ST7789/        ST7789 驅動 + 模組（M130T）
│   ├── graphics/      圖形和文字
│   ├── color/         顏色格式：RGB565、RGB888、RGB444、RGB332、BW…；螢幕的 ColorFormat
│   ├── font/          字型（.ttf → .h）
│   ├── video/         動畫和影片（.gif / .mp4 / .mov / .webm … → .h，Flash 播放或從電腦串流）
│   ├── DS18B20/       DS18B20 溫度感測器（1-Wire）
│   ├── SSD1306/       I2C OLED（SSD1306）：指令 + 板子 RAM 裡的畫面
│   ├── image/         靜態圖片（.jpg / .png / .webp / .heic → .h，Flash 或從電腦串流）
│   ├── switch/        開關（GND、VCC、OUT）
│   ├── audio/         旋律（.wav / .mp3 / .m4a / .ogg … → .h）和播放器
│   ├── buzzer/        有源 / 無源蜂鳴器
│   ├── NEC/           紅外線遙控器的 NEC 協定：接收器（HX1838）、按鍵、17 鍵遙控器
│   ├── LED/           LED（PWM）：一顆、RGB；閃爍、呼吸
│   ├── ultrasonic/    超音波測距（HC-SR04）
│   ├── TCRT5000/      反射式紅外線感測器（HW-006 循跡、四腳類比板）
│   ├── L298N/         馬達驅動：直流、步進、兩輪車
│   ├── analog/        類比感測器：旋鈕、光敏、搖桿、熱敏、聲音
│   └── stream/        跟電腦要檔案：板子的 stream::Puller、電腦的 stream.py（圖片、動畫、聲音共用）
└── src/
    ├── hide.py                                                 只留一個程式，其他的改成 .bak（用 git mv）
    ├── README.md                                               程式的寫法：settings、parts、段落、命名、週的 README
    ├── week_1_analog/                                          類比感測器的測試，見 src/week_1_analog/README.md
    ├── week_2_LED/                                             LED 的亮度和效果，見 src/week_2_LED/README.md
    ├── week_3_ST7789_DS18B20_graphics_color_font_image_video/  M130T 測試、溫度感測器、彩色圖片、作業，見 src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md
    ├── week_4_SSD1306_switch_buzzer_audio/                     OLED、黑白圖片、開關、按鈕手勢、蜂鳴器、播任意音檔的測試和作業，見 src/week_4_SSD1306_switch_buzzer_audio/README.md
    ├── week_5_NEC/                                             紅外線遙控器的測試，RGB LED、移動方塊、五線譜、鋼琴的作業，見 src/week_5_NEC/README.md
    ├── week_6_ultrasonic/                                      超音波測距 HC-SR04、中位數濾波的測試，距離控制音量、音高的作業，見 src/week_6_ultrasonic/README.md
    ├── week_7_L298N/                                           L298N 馬達驅動的測試（一顆、兩顆、電壓、PWM、步進、兩輪車、背景遙控），見 src/week_7_L298N/README.md
    └── week_8_TCRT5000/                                        循跡模組 HW-006（TCRT5000）的測試，見 src/week_8_TCRT5000/README.md
```

## 開始

需要 [PlatformIO](https://platformio.org/)（VS Code 外掛或 CLI）。

```bash
pio run                  # 編譯
pio run -t upload        # 燒錄
```

**終端機打 `pio` 說 `command not found`**：VS Code 的 PlatformIO 外掛把 `pio` 裝在 `~/.platformio/penv/bin/`，只有外掛自己開的終端機（PlatformIO 側欄的「New Terminal」）會加進 PATH。一般的終端機要自己加一次：

```bash
echo 'export PATH="$PATH:$HOME/.platformio/penv/bin"' >> ~/.zshrc
source ~/.zshrc
which pio                # ~/.platformio/penv/bin/pio
```

加在 PATH 的最後，`python`、`pip` 還是用原本的（`penv/bin` 裡也有一個 PlatformIO 自己的 `python`）。

- `src/` 裡只能有一個程式有 `setup()` / `loop()`，其他的要改成 `.bak`：`python src/hide.py week_4_SSD1306_switch_buzzer_audio/tests/1_oled` 只留下那一個。
- **每個程式的開頭都一樣**：說明，然後 `// ---------- settings ----------` 放所有可以改的東西（接線的腳位 `*_PIN`、`BAUD`、時間、速度、音量、要用的檔案…），再來 `// ---------- parts ----------` 用這些設定組出零件（`Uno::UnoPin<SWITCH_PIN>`）。換腳、調參數只改 settings，不用往下找。完整的規則見 [src/README.md](src/README.md)。
- 接線見 [src/week_1_analog/README.md](src/week_1_analog/README.md#接線)（類比感測器）、[src/week_2_LED/README.md](src/week_2_LED/README.md#接線)（LED）、[src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md](src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md#接線)（M130T）、[src/week_4_SSD1306_switch_buzzer_audio/README.md](src/week_4_SSD1306_switch_buzzer_audio/README.md#接線)（OLED、開關、蜂鳴器）、[src/week_5_NEC/README.md](src/week_5_NEC/README.md#接線)（紅外線接收器、RGB LED）、[src/week_6_ultrasonic/README.md](src/week_6_ultrasonic/README.md#接線)（HC-SR04）、[src/week_7_L298N/README.md](src/week_7_L298N/README.md#接線)（L298N）、[src/week_8_TCRT5000/README.md](src/week_8_TCRT5000/README.md#接線)（HW-006）。

```cpp
#include "Uno_ST7789.h"
#include "ST7789.h"
#include "graphics.h"
#include "color.h"
#include "font.h"

constexpr uint8_t DC_PIN = 8;
constexpr uint8_t RES_PIN = 10;
constexpr uint8_t BLK_PIN = 9;

using Bus = Uno::UnoST7789<DC_PIN, RES_PIN, Uno::NONE, BLK_PIN>;   // DC, RES, CS（沒有）, BLK
using ScreenColor = color::RGB565;                                 // 螢幕的顏色格式：graphics、font、image、video 都跟著它
using Screen = graphics::Canvas<ST7789::M130T<Bus, ScreenColor>>;
Screen screen;

void setup() {
    screen.init();
    screen.drawFilledScreen(ScreenColor::BLACK);
    screen.setFont<font::menlo_height_23_first_32_last_126_default_63>();
    screen.drawTexts(10, 10, ScreenColor::WHITE, "hello ", 42);
}

void loop() {
}
```

## Python 工具

字型、圖片、影片、旋律的轉換和串流在電腦上跑，需要 Python 3：

```bash
pip install pillow pyserial numpy
brew install ffmpeg      # 影片、音檔的解碼（音檔沒有它時，macOS 的 afconvert 也能讀大部分格式）
```

**平常不用自己跑**：程式裡寫 `FONT(...)` 這類巨集，`pio run` 前 `tools/assets.py` 會自動轉（見 [lib/README.md](lib/README.md#字型圖片影片聲音直接用檔案)）。轉換工具的 Python 寫在 `platformio.ini` 的 `custom_asset_python`。

| 工具 | 說明 |
|---|---|
| `tools/assets.py` | 編譯前自動轉換程式裡 `FONT(...)` 等巨集的檔案（沒轉過才轉）；手動：`python tools/assets.py`（`--force` 重轉、`--unused` 列出沒用到的） |
| `lib/font/source/` | [font/source/README.md](lib/font/source/README.md) |
| `lib/video/source/` | [video/source/README.md](lib/video/source/README.md) |
| `lib/image/source/` | [image/source/README.md](lib/image/source/README.md)（HEIC 可以另外裝 `pillow-heif`，沒裝就用 macOS 的 `sips`） |
| `lib/audio/source/` | [audio/source/README.md](lib/audio/source/README.md) |
| `lib/stream/source/stream.py` | 串流：`python lib/stream/source/stream.py`，開著就好，板子要哪張圖、哪個動畫、哪個聲音就送哪個；見 [stream/README.md](lib/stream/README.md#streampy) |

不會產生 `__pycache__`：會 import 別的檔案的腳本（`convert.py` import `check.py`、`tools/assets.py`）開頭都寫了 `sys.dont_write_bytecode = True`，所以直接 `python` 就好。

## 文件

| 文件 | 內容 |
|---|---|
| [lib/README.md](lib/README.md) | 函式庫總覽、四大類（主板、驅動、素材、基礎）、提示和錯誤的規則、依賴關係、**比較表**（輸入怎麼處理、哪些有裁切、浮點、RAM、時間）、共同的寫法 |
| [lib/Uno/README.md](lib/Uno/README.md) | 腳位、SPI、串口、編譯期檢查 |
| [lib/ST7789/README.md](lib/ST7789/README.md) | 驅動、Bus 介面；[module/](lib/ST7789/module/README.md) 加新模組 |
| [lib/graphics/README.md](lib/graphics/README.md) | 圖形和文字的函式 |
| [lib/color/README.md](lib/color/README.md) | 顏色格式：RGB565、RGB888、RGB444、RGB332、BW…，宣告在螢幕上（`M130T<Bus, color::RGB565>`） |
| [lib/stream/README.md](lib/stream/README.md) | 串流：板子的 `stream::Puller`、電腦的 `stream.py`、協定 |
| [lib/font/README.md](lib/font/README.md) | 字型；[source/](lib/font/source/README.md) 轉換、[output/](lib/font/output/README.md) 格式 |
| [lib/video/README.md](lib/video/README.md) | 動畫、影片的播放器；[source/](lib/video/source/README.md) 轉換和動畫的串流指令、[output/](lib/video/output/README.md) 格式 |
| [lib/DS18B20/README.md](lib/DS18B20/README.md) | DS18B20 溫度感測器：接線、函式、解析度、錯誤 |
| [lib/SSD1306/README.md](lib/SSD1306/README.md) | I2C OLED：接線、指令表、為什麼要在 RAM 留畫面、速度 |
| [lib/image/README.md](lib/image/README.md) | 圖片：支援的檔案、顏色、Flash / 串流、在 OLED 上畫；[source/](lib/image/source/README.md) 轉換和串流協定、[output/](lib/image/output/README.md) 格式 |
| [lib/switch/README.md](lib/switch/README.md) | 開關：接線、去彈跳、`ACTIVE_HIGH` |
| [lib/audio/README.md](lib/audio/README.md) | 旋律和播放器；[source/](lib/audio/source/README.md) 音檔轉換、[output/](lib/audio/output/README.md) 格式 |
| [lib/buzzer/README.md](lib/buzzer/README.md) | 蜂鳴器：有源和無源、接線、電流、不能做的事 |
| [lib/NEC/README.md](lib/NEC/README.md) | 紅外線遙控器：零件名稱、NEC 協定、遙控器的指令碼、連按 |
| [lib/LED/README.md](lib/LED/README.md) | LED：接線、電阻、共陰 / 共陽、PWM 和眼睛的亮度 |
| [lib/ultrasonic/README.md](lib/ultrasonic/README.md) | 超音波測距：原理、接線、結果、溫度和準確度 |
| [lib/TCRT5000/README.md](lib/TCRT5000/README.md) | TCRT5000 反射式感測器：原理、可變電阻、`ACTIVE_LOW` |
| [lib/L298N/README.md](lib/L298N/README.md) | L298N 馬達驅動：板子每隻腳、真值表、規格和極限、直流 / 步進馬達的函式 |
| [src/README.md](src/README.md) | 程式的寫法：settings、parts、段落、命名；`hide.py`；週的 README 的格式 |
| [src/week_1_analog/README.md](src/week_1_analog/README.md) | 類比感測器：旋鈕、光敏電阻、搖桿、NTC、聲音模組 |
| [src/week_2_LED/README.md](src/week_2_LED/README.md) | LED 的亮度、閃爍、呼吸、漸變、心跳 |
| [src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md](src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md) | M130T 測試、溫度感測器、彩色圖片、作業一～三 |
| [src/week_4_SSD1306_switch_buzzer_audio/README.md](src/week_4_SSD1306_switch_buzzer_audio/README.md) | OLED、黑白圖片、開關、蜂鳴器的測試，作業一～四 |
| [src/week_5_NEC/README.md](src/week_5_NEC/README.md) | 紅外線遙控器的測試，作業一～四（RGB LED、移動方塊、五線譜、鋼琴） |
| [src/week_6_ultrasonic/README.md](src/week_6_ultrasonic/README.md) | HC-SR04 超音波測距的測試，作業一（開關 + 距離 → 蜂鳴器音量）、作業二（距離 → 音高） |
| [src/week_7_L298N/README.md](src/week_7_L298N/README.md) | L298N 的測試：一顆、兩顆馬達、控制電壓、PWM、步進馬達、兩輪車、背景遙控 |
| [lib/analog/README.md](lib/analog/README.md) | 類比感測器：旋鈕、光敏電阻、搖桿、NTC、聲音模組 |
| [src/week_8_TCRT5000/README.md](src/week_8_TCRT5000/README.md) | HW-006 循跡模組的測試 |
