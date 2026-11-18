# src

要燒進 UNO 的程式。每個程式是一個 `.cpp`，一次只能有一個（PlatformIO 會編譯 `src/` 裡所有的 `.cpp`，但只能有一個 `setup()` / `loop()`），其他的改成 `.cpp.bak`。

```bash
python src/hide.py week_7_L298N/tests/6_car     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload                       # 編譯、燒錄
pio device monitor                      # 看串口（1 Mbaud，platformio.ini 的 monitor_speed）
```

## 檔案

```
src/
├── README.md       這份：程式的寫法
├── hide.py         只留一個程式，其他的改成 .bak（用 git mv，git 保留檔案的歷史）
├── clean.py        刪掉快取和轉好的素材（__pycache__、.pio/build、lib/<種類>/output/*.h）
└── week_{N}_{lib}/ 每週一個資料夾，名稱是週數加上這週新用到的函式庫（硬體在前、輔助在後）
    ├── README.md
    ├── tests/
    │   └── {n}_{說明}.cpp    測試：一個零件或函式庫的功能，一項一項確認
    └── assignments/
        └── {n}_{說明}.cpp    作業：用這些零件做出一個東西
```

| 資料夾 | 內容 |
|---|---|
| [week_1_analog/](week_1_analog/README.md) | 類比感測器：旋鈕、光敏電阻、搖桿、NTC、聲音模組 |
| [week_2_LED/](week_2_LED/README.md) | LED 的亮度和效果（閃爍、呼吸、漸變、心跳） |
| [week_3_ST7789_DS18B20_graphics_color_font_image_video/](week_3_ST7789_DS18B20_graphics_color_font_image_video/README.md) | M130T 測試、溫度感測器、彩色圖片、作業一～三 |
| [week_4_SSD1306_switch_buzzer_audio/](week_4_SSD1306_switch_buzzer_audio/README.md) | OLED、黑白圖片、開關、按鈕手勢、蜂鳴器、播任意音檔的測試，作業一～四 |
| [week_5_NEC/](week_5_NEC/README.md) | 紅外線遙控器的測試，作業一～四（RGB LED、移動方塊、五線譜、鋼琴） |
| [week_6_ultrasonic/](week_6_ultrasonic/README.md) | HC-SR04 超音波測距、中位數濾波的測試，作業一、二 |
| [week_7_L298N/](week_7_L298N/README.md) | L298N 的測試：一顆、兩顆馬達、控制電壓、PWM、步進馬達、兩輪車、背景遙控 |
| [week_8_TCRT5000/](week_8_TCRT5000/README.md) | HW-006 循跡模組的測試 |

## hide.py

| 指令 | 結果 |
|---|---|
| `python src/hide.py week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen` | 只留 `week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp`，其他全部 `.bak` |
| `python src/hide.py src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp.bak` | 同上（`src/` 之前和第一個 `.` 之後會被拿掉，可以直接貼路徑） |
| `python src/hide.py week_8` | 含有 `week_8` 的程式只有一個時，留那一個 |
| `python src/hide.py '7*car'` | `*` 是任意字：依序含有 `7`、`car` 的那一個（要加引號） |
| `python src/hide.py` | 全部 `.bak`，什麼都不編譯 |

符合的不是剛好一個時，什麼都不改，印出錯誤。

## clean.py

刪掉工具產生、而且能再產生的東西：每個 `__pycache__`、PlatformIO 的 `.pio/build`、`lib/font`、`image`、`video`、`audio` 的 `output/*.h`（`README.md` 留著）。

| 指令 | 結果 |
|---|---|
| `python src/clean.py --dry-run` | 只列出會刪什麼 |
| `python src/clean.py` | 刪掉 |
| `python src/clean.py --rebuild` | 刪掉，再照每個檔案開頭的 `// source:`、`// options:` 全部重新轉換（檔名不變）；原始檔不見的那個不刪，印警告 |

- **只刪不轉時**：下次編譯只會重轉程式裡 `FONT(...)` 這類巨集的檔案；直接用名稱的（`font::menlo_…`、`picture::sunset_…`）要 `--rebuild`，或 `git restore lib` 拿回來（轉好的檔案都在 git 裡）。
- **`--rebuild` 什麼時候用**：改了 `convert.py`、想讓每個輸出都是最新的寫法（約 10 秒）。轉換用的 Python 跟 `tools/assets.py` 一樣：`platformio.ini` 的 `custom_asset_python`。
- `.pio/build` 刪掉後，下一次編譯會全部重新編譯（慢一點，結果一樣）。

## 檔名

| 什麼 | 名稱 | 例子 |
|---|---|---|
| 一週 | `week_{N}_{lib}`：N 是第幾週；`{lib}` 是這週**第一次用到的函式庫**（`lib/` 的資料夾名稱，照 `lib/` 的大小寫），多個用 `_` 接起來 | `week_3_ST7789_DS18B20_graphics_color_font_image_video` |
| 測試 | `week_…/tests/{n}_{說明}.cpp`：n 從 1 開始；說明是小寫英文、底線分開的幾個字 | `week_7_L298N/tests/6_car.cpp` |
| 作業 | `week_…/assignments/{n}_{說明}.cpp`：n 照作業的編號 | `week_5_NEC/assignments/4_piano.cpp` |

- **`{lib}` 的順序**：先放硬體的函式庫（`ST7789`、`DS18B20`、`switch`、`L298N`…，這週的主角），再放輔助的，固定照 `graphics`、`color`、`font`、`image`、`video`、`audio`、`stream` 的順序（先畫圖的基礎，再素材）。前幾週已經出現過的不再寫（week_4 也用 `graphics`，但它在 week_3 已經寫了）；`Uno` 每週都用，不寫。
- **新的程式放在哪一週**：它主要測的硬體函式庫第一次出現的那一週。可以用前面幾週的零件（例如 `week_4_SSD1306_switch_buzzer_audio/tests/6_clicks` 用 week_2 的 LED 顯示按鈕手勢），不能用後面幾週的。
- **沒有對應的週**：開新的一週；比較底層、其他零件會用到的放前面的週（例如類比輸入 → `week_1_analog`）。
- **一個程式測好幾個函式庫的功能時**：拆成每個函式庫一個測試，各放在自己的週。

## 程式的寫法

每個程式由上到下都是這個順序，註解用英文：

```cpp
#include <Arduino.h>                  // 1. include：Arduino.h，Uno_*.h，再來函式庫的入口
#include "Uno_serial.h"
#include "Uno_pin.h"
#include "L298N.h"

// UNO + L298N board + 2 DC motors as a two-wheeled car: moves with times, told on the USB serial
//   pio device monitor        (platformio.ini: monitor_speed = BAUD)
//                                    2. 說明：第一行「UNO + 零件: 做什麼」，接著怎麼看結果、要先知道的事

// ---------- settings ----------
                                      // 3. 所有可以調的東西
// wiring: L298N with both EN jumpers ON
//   +12V -> battery +   GND -> battery - AND the UNO's GND
constexpr uint8_t IN1_PIN = 5;              // IN1: a PWM pin (3, 5, 6, 9, 10, 11)
constexpr uint32_t BAUD = 1000000;          // USB serial: the same as monitor_speed in platformio.ini

constexpr uint8_t SPEED = 128;              // 0 ~ 255
constexpr uint16_t PAUSE_MS = 800;          // braked between the moves

// ---------- parts ----------
                                      // 4. 用設定組出零件
using Link = Uno::UnoSerial<BAUD>;
using Car = L298N::TwoWheelCar<Board, IN12_SIDE, IN12_POSITIVE, IN34_POSITIVE>;
Car car;

// ---------- squares ----------
                                      // 5. 程式自己的功能，一段一個主題（名稱自訂）
// ---------- tests ----------
                                      // 6. 測試（只有測試程式有）
// ---------- main ----------
                                      // 7. setup()、loop()
void setup() { ... }
void loop() { ... }
```

### 1. include

`#include <Arduino.h>` → 用到的 `Uno_*.h`（UNO 的介面）→ 函式庫的入口（`L298N.h`、`audio.h`…）。只 include 入口，不要 include `module/` 裡的檔案（原因見 [lib/README.md](../lib/README.md#include-的規則)）。

### 2. 說明

| 行 | 寫什麼 | 例子 |
|---|---|---|
| 第一行 | `// UNO + 零件（模組名）: 做什麼` | `// UNO + HC-SR04 ultrasonic distance module: every measurement goes to the USB serial` |
| 接著 | 怎麼看結果（`pio device monitor`、要先跑的 `stream.py`） | `//   pio device monitor        (platformio.ini: monitor_speed = BAUD)` |
| 接著 | 會發生什麼、要先知道的事、怎麼調 | `// SPIN_90_MS depends on the car and the floor: measure it` |

接線不寫在這裡，寫在 settings 的腳位旁邊。

### 3. settings

換腳、調參數只改這一段，不用往下找。**能調的都放這裡，越多越好。**

| 放什麼 | 寫法 | 例子 |
|---|---|---|
| 固定的接線（電源、GND、硬體 SPI） | `// wiring:` 註解 | `// wiring: M130T VCC -> 3.3V, GND -> GND, SCL -> D13, SDA -> D11 (hardware SPI: fixed)` |
| 每隻可以換的腳 | `constexpr uint8_t X_PIN = …;`，註解寫零件上的腳名和限制；類比腳寫 `A0`；沒接寫 `Uno::NONE` | `constexpr uint8_t ENA_PIN = 5;  // ENA: a PWM pin (3, 5, 6, 9, 10, 11)` |
| 接法的選項 | `bool`、enum | `SWITCH_ACTIVE_HIGH`、`IN12_SIDE` |
| 串口速度 | `BAUD`（跟 `platformio.ini` 的 `monitor_speed` 一樣） | `constexpr uint32_t BAUD = 1000000;` |
| 函式庫 template 參數的預設值 | 列出來，即使跟預設一樣 | `DEBOUNCE_MS`、`I2C_KHZ`、`GAP_MS`、`IR_QUEUE`、`STREAM_CHUNK` |
| 螢幕的顏色格式 | ST7789：`using ScreenColor = …;`（換格式只改這行）；OLED 只能黑白，放門檻 `BW_THRESHOLD` | `using ScreenColor = color::RGB565;` |
| 時間、速度、亮度、音量、門檻 | 名稱加單位 | `HOLD_MS`、`TONE_HZ`、`MIN_MM`、`SUPPLY_V`、`VOLUME` |
| 用哪個檔案 | `FONT(...)`、`PICTURE(...)`、`ANIMATION(...)`、`MELODY(...)` | `using Song = MELODY("~/x.aiff", "--quality high");` |
| 測試的順序表、對應表 | 陣列 | `LADDER[]`、`NOTE_HZ[]`、`MOVES[]` |
| 只跑某幾個測試 | `ONLY_TESTS`：0 = 全部，bit i = 只跑測試 i | `constexpr uint32_t ONLY_TESTS = (1UL << 14) \| (1UL << 15);` |

- **每行一個 `constexpr`**，型別用剛好的（`uint8_t`、`uint16_t`、`int16_t`…），註解寫範圍、單位或接到哪。
- **名稱**：大寫加底線；腳位 `_PIN`；單位放最後：`_MS`、`_US`、`_HZ`、`_KHZ`、`_MM`、`_CM`、`_V`；個數 `_COUNT`。
- **分組**：接線（腳位、接法、`BAUD`）在前，空一行，再來行為的參數。

### 4. parts

用 settings 組出零件，這裡不出現數字：

```cpp
using Link = Uno::UnoSerial<BAUD>;
using Button = switches::Switch3Pins<Uno::UnoInputPin<SWITCH_PIN>, SWITCH_ACTIVE_HIGH, DEBOUNCE_MS>;
Button button;
```

- **不要直接寫 `Uno::UnoInputPin<2>`**：腳位一定先寫成 `X_PIN`。
- **型別用 `using` 取名**：大駝峰，用零件的角色命名（`Bus`、`Screen`、`Link`、`Speaker`、`Car`、`Button`），不用型號。
- 物件（變數）用小駝峰（`screen`、`car`、`speaker`）。
- **顏色**：常數用 `ScreenColor::RED`、`ScreenColor::color(r, g, b)`；存顏色的變數、參數用 `using Color = ScreenColor::Value;`，不要寫 `uint16_t` 或 `RGB565::`（換格式時才不用改程式）。
- 別的接法寫在旁邊的註解：`// 3-pin module: PassiveBuzzer3Pins<Uno::UnoTone<BUZZER_PIN, true>>`。

### 5. 程式自己的段落

`// ---------- 名稱 ----------`，名稱小寫英文，說這段在做什麼（`rings`、`keys`、`distance -> pitch`）。函式名稱小駝峰、動詞開頭（`drawRing`、`printResult`）；每個函式上面一行註解說它做什麼。

### 6. tests（測試程式）

```cpp
// ---------- tests ----------

// 0. palette 16, scale 2: 60 x 40 stored, 120 x 80 on the screen
//    expected: a sunset, flat color bands
void testPalette16() { ... }

void (*const tests[])() = {
    testPalette16,
    ...
};
```

- 每個測試一個函式 `testXxx()`，上面註解 `// N. 測什麼`，下一行 `//    expected: 應該看到什麼`；N 是在 `tests[]` 的位置，從 0 開始。
- 用 `ONLY_TESTS` 挑測試，不要把 `tests[]` 裡的註解掉。
- 只有一個流程的測試（例如一直印距離）沒有 `tests[]`，段落名稱用 `test`。

### 7. main

`setup()`、`loop()` 放最後，前面一定是 `// ---------- main ----------`。

- **`loop()` 不要卡住**：零件的動作用「開始 + 每次 `loop()` 呼叫 `update()`」的寫法（車、LED 效果、蜂鳴器、按鈕都是），`delay()` 只用在測試的「停一下讓人看」。哪些函式會等，見 [lib/README.md 的「會不會等」](../lib/README.md#會不會等loop-會不會卡住)。
- **印到串口**：`Link::printText`、`printNumber`、`printDecimal`（小數）、`printHex`、`printPin`（腳位名稱，換腳後跟著變），換行寫 `"\r\n"`。出問題的那一行用 `[ERROR] …` 或 `[WARNING] …` 開頭（跟 Python 工具一樣，見 [lib/README.md 的「提示和錯誤」](../lib/README.md#提示和錯誤)），平常的量測結果不加標籤。不用 Arduino 的 `Serial`（它比較大，而且跟 `Uno::UnoSerial` 搶同一個 UART）。

## 週的 README

每個 `week_{N}_{lib}/README.md` 都是這個順序：

| 段落 | 內容 |
|---|---|
| `# week_{N}_{lib}` | 一段說明（這週的零件，連到 `lib/` 的 README）、`hide.py` 的用法、程式的總表「檔案 / 內容 / 需要的硬體」 |
| `## 接線` | 每個零件的腳 → UNO 的腳（跟程式的 `*_PIN` 一樣），哪個程式用到 |
| `# tests/{n}_{說明}.cpp：說明`、`# assignments/…` | 每個程式一段，**測試在前、作業在後**：會看到什麼、怎麼操作、settings 裡可以調的、看到的不對時怎麼辦 |

- 程式的段落用 `#`（一級），裡面的小節用 `##`；在週的 README 裡提到自己這週的程式寫 `tests/6_car`，別週的寫完整的 `week_7_L298N/tests/6_car`。
- 寫到設定時用設定的名稱：「改 `SCL_PIN`、`SDA_PIN`」，不寫「改 `Uno::UnoI2C<13, 11>` 的數字」。
- 用詞跟 lib 的 README 一樣（[lib/README.md 的「用詞」](../lib/README.md#readme-的格式)）。
