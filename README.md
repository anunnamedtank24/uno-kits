# Uno-kits

Arduino UNO 的函式庫和課堂程式，分成四類（見 [lib/README.md](lib/README.md#分類和名稱)）：

| 大類 | 函式庫 |
|---|---|
| **主板** | `Uno`：腳位、PWM、類比、中斷、SPI、I2C、1-Wire、USB 串口、方波、取樣輸出、紅外線載波、背景執行 |
| **驅動** | 螢幕（ST7789 彩色、SSD1306 OLED）、DS18B20 溫度、TCRT5000 循跡、L298N 馬達、NEC 紅外線遙控、開關、蜂鳴器、LED、超音波測距、類比感測器（旋鈕、光敏、搖桿、熱敏、聲音） |
| **素材** | 字型、圖片（jpg / png / webp / heic）、動畫和影片（gif / mp4 / mov / webm…）、聲音（wav / mp3 / m4a / ogg…）：電腦上轉成 `.h`，存在 Flash 或從電腦串流 |
| **基礎** | 顏色格式（`color`）、圖形和文字（`graphics`）、跟電腦要檔案的協定（`stream`） |

- 用 template 做硬體抽象，編譯後跟直接寫暫存器一樣快。
- 字型、動畫、圖片、聲音：程式裡直接寫檔案（`FONT("~/x.ttf", "--height 16")`、`PICTURE(...)`、`ANIMATION(...)`、`MELODY(...)`），編譯前自動轉換；動畫和圖片依 UNO 的畫圖時間挑最快的畫法。
- 函式庫之間不互相 include，只透過介面（template 參數）合作；UNO 的每個介面都繼承同一個基底（`Uno::UnoBoard`），換主板只換 `lib/Uno/`。
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
│   ├── Uno/           主板：ATmega328P 的腳位、SPI、I2C、1-Wire、串口、計時器、背景執行
│   ├── ST7789/        驅動：ST7789 彩色螢幕 + 模組（M130T）
│   ├── SSD1306/       驅動：I2C OLED（SSD1306）：指令 + 板子 RAM 裡的畫面
│   ├── DS18B20/       驅動：DS18B20 溫度感測器（1-Wire）
│   ├── TCRT5000/      驅動：反射式紅外線感測器（HW-006 循跡、四腳類比板）
│   ├── L298N/         驅動：馬達驅動：直流、步進、兩輪車
│   ├── NEC/           驅動：紅外線遙控器的 NEC 協定：接收器（HX1838）、發射、按鍵、17 鍵遙控器
│   ├── switch/        驅動：開關（GND、VCC、OUT）；單擊、雙擊、長按
│   ├── buzzer/        驅動：有源 / 無源蜂鳴器
│   ├── LED/           驅動：LED（PWM）：一顆、RGB、紅綠燈；閃爍、呼吸
│   ├── ultrasonic/    驅動：超音波測距（HC-SR04）
│   ├── analog/        驅動：類比感測器：旋鈕、光敏、搖桿、熱敏、聲音
│   ├── font/          素材：字型（.ttf → .h）
│   ├── image/         素材：靜態圖片（.jpg / .png / .webp / .heic → .h，Flash 或從電腦串流）
│   ├── video/         素材：動畫和影片（.gif / .mp4 / .mov / .webm … → .h，Flash 或從電腦串流）
│   ├── audio/         素材：聲音（.wav / .mp3 / .m4a / .ogg … → 音符、節奏或取樣）和播放器
│   ├── color/         基礎：顏色格式：RGB565、RGB888、RGB444、RGB332、BW…；螢幕的 ColorFormat
│   ├── graphics/      基礎：圖形和文字
│   └── stream/        基礎：跟電腦要檔案：板子的 stream::Puller、電腦的 stream.py（圖片、動畫、聲音共用）
└── src/
    ├── hide.py                                                 只留一個程式，其他的改成 .bak（用 git mv）
    ├── clean.py                                                刪掉快取和轉好的素材；--rebuild 全部重新轉換
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
- **每個程式的開頭都一樣**：說明，然後 `// ---------- settings ----------` 放所有可以改的東西（接線的腳位 `*_PIN`、`BAUD`、時間、速度、音量、要用的檔案…），再來 `// ---------- parts ----------` 用這些設定組出零件（`Uno::UnoInputPin<SWITCH_PIN>`）。換腳、調參數只改 settings，不用往下找。完整的規則見 [src/README.md](src/README.md)。
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

字型、圖片、影片、聲音的轉換和串流在電腦上跑，需要 Python 3：

```bash
pip install pillow pyserial numpy
brew install ffmpeg      # 影片、音檔的解碼（音檔沒有它時，macOS 的 afconvert 也能讀大部分格式）
```

**平常不用自己跑**：程式裡寫 `FONT(...)` 這類巨集，`pio run` 前 `tools/assets.py` 會自動轉（見 [lib/README.md](lib/README.md#字型圖片影片聲音直接用檔案)）。轉換工具的 Python 寫在 `platformio.ini` 的 `custom_asset_python`。

| 工具 | 說明 |
|---|---|
| `tools/assets.py` | 編譯前自動轉換程式裡 `FONT(...)` 等巨集的檔案（沒轉過才轉）；手動：`python tools/assets.py`（`--force` 重轉、`--unused` 列出沒用到的） |
| `src/clean.py` | 刪掉 `__pycache__`、`.pio/build`、轉好的素材（`lib/<種類>/output/*.h`）；`--rebuild` 全部重新轉換、`--dry-run` 只列出來（見 [src/README.md](src/README.md#cleanpy)） |
| `lib/font/source/` | [font/source/README.md](lib/font/source/README.md) |
| `lib/video/source/` | [video/source/README.md](lib/video/source/README.md) |
| `lib/image/source/` | [image/source/README.md](lib/image/source/README.md)（HEIC 可以另外裝 `pillow-heif`，沒裝就用 macOS 的 `sips`） |
| `lib/audio/source/` | [audio/source/README.md](lib/audio/source/README.md) |
| `lib/stream/source/stream.py` | 串流：`python lib/stream/source/stream.py`，開著就好，板子要哪張圖、哪個動畫、哪個聲音就送哪個；見 [stream/README.md](lib/stream/README.md#streampy) |

不會產生 `__pycache__`：會 import 別的檔案的腳本（`convert.py` import `check.py`、`tools/assets.py`、`stream.py` 載入各個 `stream_handler.py`）開頭都寫了 `sys.dont_write_bytecode = True`，所以直接 `python` 就好。

## 文件

每個函式庫有兩份：`README.md` 給使用者，`INTERFACE.md` 給開發者（寫新套件、換主板、換零件）。

| 函式庫 | README.md（使用者） | INTERFACE.md（開發者） |
|---|---|---|
| [lib/README.md](lib/README.md) | 函式庫總覽、四大類（主板、驅動、素材、基礎）、介面、提示和錯誤的規則、前提條件、依賴關係、文件格式、**比較表**（輸入怎麼處理、裁切、浮點、RAM、時間、會不會等） | — |
| Uno | [README](lib/Uno/README.md)：用法、輸入限制、注意事項（計時器、中斷衝突） | [INTERFACE](lib/Uno/INTERFACE.md)：每個介面的函式、分級、編譯期檢查、資源 |
| ST7789 | [README](lib/ST7789/README.md)：用法、顏色模式、接線；[module/](lib/ST7789/module/README.md) 加新模組 | [INTERFACE](lib/ST7789/INTERFACE.md)：Bus 介面、函式、裁切、資源 |
| SSD1306 | [README](lib/SSD1306/README.md)：用法、接線、注意事項 | [INTERFACE](lib/SSD1306/INTERFACE.md)：指令表、I2C 格式、為什麼要在 RAM 留畫面、init 流程、速度 |
| DS18B20 | [README](lib/DS18B20/README.md)：用法、接線、解析度、注意事項 | [INTERFACE](lib/DS18B20/INTERFACE.md)：函式、錯誤碼、時間 |
| TCRT5000 | [README](lib/TCRT5000/README.md)：用法、四腳板、接線、原理、可變電阻 | [INTERFACE](lib/TCRT5000/INTERFACE.md)：函式、參數、資源 |
| L298N | [README](lib/L298N/README.md)：用法、接線、板子每隻腳、真值表、規格和極限、步進馬達 | [INTERFACE](lib/L298N/INTERFACE.md)：直流 / 步進馬達、兩輪車的函式 |
| NEC | [README](lib/NEC/README.md)：用法、發射、接線、零件名稱、遙控器的指令碼、連按 | [INTERFACE](lib/NEC/INTERFACE.md)：函式、NEC 協定、資源 |
| switch | [README](lib/switch/README.md)：用法、Clicks、接線、兩隻腳和三隻腳、去彈跳 | [INTERFACE](lib/switch/INTERFACE.md)：函式、參數 |
| buzzer | [README](lib/buzzer/README.md)：用法、接線、兩隻腳和三隻腳、音量、有源和無源、注意事項 | [INTERFACE](lib/buzzer/INTERFACE.md)：函式、參數、資源 |
| LED | [README](lib/LED/README.md)：用法、Effect、接線、電阻、共陰 / 共陽、PWM 和眼睛的亮度 | [INTERFACE](lib/LED/INTERFACE.md)：函式、參數、資源 |
| ultrasonic | [README](lib/ultrasonic/README.md)：用法、Median、接線、原理、結果、溫度和準確度 | [INTERFACE](lib/ultrasonic/INTERFACE.md)：函式、參數、資源 |
| analog | [README](lib/analog/README.md)：用法、接線、零件名稱（旋鈕、光敏、搖桿、NTC、聲音模組） | [INTERFACE](lib/analog/INTERFACE.md)：函式、資源 |
| font | [README](lib/font/README.md)：用法、直接用檔案；[source/](lib/font/source/README.md) 轉換、[output/](lib/font/output/README.md) 格式 | [INTERFACE](lib/font/INTERFACE.md)：流程 |
| image | [README](lib/image/README.md)：用法、直接用檔案、顏色和來源、在 OLED 上畫；[source/](lib/image/source/README.md) 轉換和串流協定、[output/](lib/image/output/README.md) 格式 | [INTERFACE](lib/image/INTERFACE.md)：函式、參數、怎麼畫、資源 |
| video | [README](lib/video/README.md)：用法、直接用檔案、三種目標、時間控制；[source/](lib/video/source/README.md) 轉換和串流指令、[output/](lib/video/output/README.md) 格式 | [INTERFACE](lib/video/INTERFACE.md)：函式、參數、資源 |
| audio | [README](lib/audio/README.md)：用法、直接用檔案、接線、格式和品質、Flash 或串流、手寫旋律；[source/](lib/audio/source/README.md) 轉換、[output/](lib/audio/output/README.md) 格式 | [INTERFACE](lib/audio/INTERFACE.md)：函式、參數、串流、資源 |
| color | [README](lib/color/README.md)：用法、有哪些格式、用在哪、換算 | [INTERFACE](lib/color/INTERFACE.md)：ColorFormat 的成員、寫一個新的格式 |
| graphics | [README](lib/graphics/README.md)：用法、空心和實心、各種值畫成什麼 | [INTERFACE](lib/graphics/INTERFACE.md)：函式、一個像素一個顏色、資源 |
| stream | [README](lib/stream/README.md)：用法、`stream.py`、注意事項 | [INTERFACE](lib/stream/INTERFACE.md)：函式、參數、協定、資源 |

| 程式 | 內容 |
|---|---|
| [src/README.md](src/README.md) | 程式的寫法：settings、parts、段落、命名；`hide.py`、`clean.py`；週的 README 的格式 |
| [src/week_1_analog/README.md](src/week_1_analog/README.md) | 類比感測器的測試：旋鈕、光敏電阻、搖桿、NTC、聲音模組 |
| [src/week_2_LED/README.md](src/week_2_LED/README.md) | LED 的亮度、閃爍、呼吸、漸變、心跳 |
| [src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md](src/week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md) | M130T 測試、溫度感測器、彩色圖片、作業一～三 |
| [src/week_4_SSD1306_switch_buzzer_audio/README.md](src/week_4_SSD1306_switch_buzzer_audio/README.md) | OLED、黑白圖片、開關、蜂鳴器的測試，作業一～四 |
| [src/week_5_NEC/README.md](src/week_5_NEC/README.md) | 紅外線遙控器的測試，作業一～四（RGB LED、移動方塊、五線譜、鋼琴） |
| [src/week_6_ultrasonic/README.md](src/week_6_ultrasonic/README.md) | HC-SR04 超音波測距的測試，作業一（開關 + 距離 → 蜂鳴器音量）、作業二（距離 → 音高） |
| [src/week_7_L298N/README.md](src/week_7_L298N/README.md) | L298N 的測試：一顆、兩顆馬達、控制電壓、PWM、步進馬達、兩輪車、背景遙控 |
| [src/week_8_TCRT5000/README.md](src/week_8_TCRT5000/README.md) | HW-006 循跡模組的測試 |
