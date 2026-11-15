# lib

這個專案的函式庫，全部只有標頭檔。硬體用 template 參數傳進來，所以每個函式庫只依賴「介面」，不依賴別的函式庫；換主板只要換 `Uno/`。

## 檔案

```
lib/
├── Uno/            ATmega328P：腳位、SPI、UART、PWM、計時器；各種介面的 UNO 版本
├── ST7789/         ST7789 彩色螢幕：指令、窗口、模組（M130T）
├── I2C/            SSD1306 OLED：指令、板子 RAM 裡的畫面、模組（128×64、128×32）
├── graphics/       圖形和文字：畫在任何有 fillWindow 的螢幕上
├── color/          RGB565 顏色
├── font/           字型（.ttf → .h）
├── image/          靜態圖片（.jpg / .png / .webp / .heic → .h）：Flash 或從 Mac 串流
├── video/          動畫和影片（.gif / .mp4 / .mov / .webm … → .h）：Flash 或從 Mac 串流
├── audio/          旋律（.wav / .mp3 / .m4a / .ogg … → .h）和播放器
├── temp_sensor/    DS18B20 溫度感測器（1-Wire）
├── switch/         開關（GND、VCC、OUT）
└── buzzer/         有源 / 無源蜂鳴器
```

| 資料夾 | namespace | 入口 | 依賴的介面 |
|---|---|---|---|
| [Uno](Uno/README.md) | `Uno` | 用到哪個介面就 include 哪個檔案 | Arduino（`millis`） |
| [ST7789](ST7789/README.md) | `ST7789` | `module/M130T.h`（或 `ST7789.h`） | Bus |
| [I2C](I2C/README.md) | `SSD1306` | `module/OLED_128x64.h`（或 `I2C.h`） | Bus |
| [graphics](graphics/README.md) | `graphics` | `graphics.h` | Screen |
| [color](color/README.md) | `color` | `color.h` | — |
| [font](font/README.md) | `font` | `font.h` | — |
| [image](image/README.md) | `image`、`picture` | `image.h` | Screen、Link |
| [video](video/README.md) | `video`、`animation` | `video.h` | Screen、Link |
| [audio](audio/README.md) | `audio`、`melody` | `audio.h` | Sound |
| [temp_sensor](temp_sensor/README.md) | `temp_sensor` | `temp_sensor.h` | Wire |
| [switch](switch/README.md) | `switches` | `switch.h` | Pin |
| [buzzer](buzzer/README.md) | `buzzer` | `module/active_buzzer.h`、`module/passive_buzzer.h`（或 `buzzer.h`） | Pin、Tone |

## 介面

介面是「只有 static 函式的 struct」（duck typing）：函式庫用 template 參數接收它，編譯後跟直接寫暫存器一樣快。每個介面的完整清單寫在使用它的標頭檔開頭，也列在各 README 的「介面需求」。

| 介面 | 誰需要 | 需要的函式（摘要） | UNO 的實作 |
|---|---|---|---|
| Bus（SPI 螢幕） | `ST7789` | `init`、`write8/16`、`cmdMode`、`dataMode`、`select`…、`delayMs`、`nowMs`、`readFlash8/16` | `Uno::UnoST7789`（`Uno_ST7789.h`） |
| Bus（I2C） | `SSD1306` | `init`、`start`、`write8`、`read8`、`stop`、`delayMs`、`nowMs`、`readFlash8/16` | `Uno::UnoI2C`（`Uno_I2C.h`） |
| Screen | `graphics`、`image`、`video` | `width`、`height`、`fillWindow`、`writeWindow`、型別 `BusType`（它的 `readFlash8/16`、`nowMs`）；`video` 另外要 `setWindow`、`writeData16`、`endWrite` | `ST7789::M130T`、`SSD1306::OLED128x64` |
| Link | `image::StreamImage`、`video::StreamPlayer` | `init`、`available`、`read8`、`write8`、`clearInput` | `Uno::UnoSerial`（`Uno_serial.h`） |
| Wire | `temp_sensor` | `init`、`reset`、`write8`、`read8`、`readBit`、`nowMs` | `Uno::UnoOneWire`（`Uno_one_wire.h`） |
| Pin（輸入） | `switch` | `init`、`read`、`nowMs` | `Uno::UnoPin`（`Uno_pin.h`） |
| Pin（輸出） | `buzzer::ActiveBuzzer` | `init`、`write`、`nowMs`、`readFlash16` | `Uno::UnoOutputPin`（`Uno_pin.h`） |
| Tone | `buzzer::PassiveBuzzer` | `init`、`start`、`stop`、`realHz`、`nowMs`、`readFlash16` | `Uno::UnoTone`（`Uno_tone.h`） |
| Sound | `audio::Player` | `init`、`start`、`stop`、`nowMs`、`readFlash16` | `buzzer::ActiveBuzzer`、`buzzer::PassiveBuzzer` |

## 依賴關係

```
Uno_ST7789.h ─(Bus)─▶ ST7789::Datasheet ▶ Window ▶ M130T ──────┐
Uno_I2C.h ───(Bus)─▶ SSD1306::Datasheet ▶ Frame ▶ OLED128x64 ─┤ Screen
                                                                ▼
                     font ─▶ graphics::Shapes ▶ Text = graphics::Canvas<Screen>
                                                                │
                     image::FlashImage、StreamImage ◀───────────┤（writeWindow：任何螢幕）
                     video::FlashPlayer、StreamPlayer ◀─────────┘（setWindow：任何螢幕）
Uno_serial.h ─(Link)─▶ image::StreamImage、video::StreamPlayer

Uno_one_wire.h ─(Wire)─▶ temp_sensor::DS18B20
Uno_pin.h ─────(Pin)──▶ switches::Switch
Uno_pin.h ─────(Pin)──▶ buzzer::ActiveBuzzer ──┐
Uno_tone.h ────(Tone)─▶ buzzer::PassiveBuzzer ─┴─(Sound)─▶ audio::Player
```

- **換主板**：只換 `Uno/`，為新的主板寫同樣函式的 struct。
- **換螢幕模組**：在 `ST7789/module/` 或 `I2C/module/` 加一個檔案。
- **換發聲的東西**：寫一個有 Sound 函式的 struct，`audio::Player` 就能播。
- **`color` 和 `font` 不依賴任何東西**；`graphics` 本身沒有顏色常數。

## README 的格式

每份 README 用同樣的結構和用詞，沒有的段落就省略。

**函式庫的 README**：

| 段落 | 內容 |
|---|---|
| 標題 + 一段說明 | 是什麼、`namespace`、依賴哪些介面 |
| `## 檔案` | 資料夾樹，每個檔案一行說明 |
| `## 接線` | 硬體的腳位表、電源、上拉、電流 |
| `## 用法` | 最短的完整範例，最後一行「完整的例子：`src/…`」 |
| `## 介面需求` | template 參數要的 static 函式（duck typing）、現成的實作 |
| `## 函式` | 每個公開函式，說明以動詞開頭（「畫…」「回傳…」「設定…」） |
| `## 參數` | template 參數、預設值 |
| 主題段落 | 這個函式庫特有的內容，例如指令表、格式、協定 |
| `## 輸入限制` | 表格「項目 / 範圍 / 超出時」 |
| `## 資源` | RAM、Flash、時間、佔用的硬體（腳位、計時器、中斷） |
| `## 注意事項` | 表格「情況 / 結果 / 做法」：不能做的事、常見問題 |

**`source/` 的 README**：一段說明（需要的套件）→ `## 檔案` → `## 用法` → `## convert.py`（支援的檔案、參數、流程、錯誤和警告）→ `## check.py` →（`## stream.py`）。

**`output/` 的 README**：一段說明 → `## 檔案` → `## selected.h` → `## 目前的…` → `## 產生的 struct` → `## 格式（format N）`。

**用詞**：

| 用 | 不用 |
|---|---|
| 函式、回傳、呼叫 | 方法、返回值、調用 |
| 腳位、Flash、RAM、週期 | 引腳、flash、記憶體（指 RAM 時）、cycle |
| 預設、範圍、上限、下限 | 默認、區間 |
| 介面（duck typing 的 struct） | 接口、協議 |
| 「超出時」只用：編譯錯誤、編譯警告、裁切、夾到範圍內、不畫 / 安靜、回傳錯誤、`convert.py` 報錯、不檢查、— | 其他說法 |
| 數字和單位之間空一格：20 ms、2 KB；範圍寫 0 ～ 19 | 20ms、0-19 |

## 比較

### 不合理的輸入怎麼處理

| 處理 | 什麼時候 | 例子 |
|---|---|---|
| **編譯錯誤** | 編譯時就知道一定不行 | 腳位不存在、SPI 腳被拿來當 DC / RES / CS / BLK、沒有 CS 卻不是 SPI mode 3、鮑率誤差 > 5%、`FlashPlayer` 拿到只有 mac 的動畫、`StreamPlayer` 拿到 256 色動畫 |
| **編譯警告** | 能動，但不建議 | 兩個訊號同一隻腳、用到 D0 / D1、BLK 不是 PWM 腳或用 Timer0、鮑率誤差 > 2%、`StreamPlayer` 拿到只有 uno 的動畫 |
| **裁切** | 執行時，畫到螢幕外 | 所有圖形、文字、`writeWindow`、圖片：只畫螢幕上的部分 |
| **夾到範圍內** | 執行時，參數超出 | SSD1306：`setMultiplexRatio`、`setClock`、`setPrecharge`（其他指令只取 datasheet 定義的位元）；`UnoTone`：16 ～ 20000 Hz |
| **不畫 / 安靜** | 執行時，參數本身沒意義 | 寬或高 ≤ 0、半徑 < 0、邊界 ≤ 0、空字串；動畫放不下螢幕；`audio::Player` 的 hz、count、ms 是 0 |
| **回傳錯誤** | 執行時，硬體沒回應或資料壞了 | DS18B20：`ERROR_NO_SENSOR`、`ERROR_CRC`…；OLED：`init()`、`display()` 回傳 false；`StreamImage::draw()`、`StreamPlayer::error()` |
| **`convert.py` 報錯 / 警告** | 電腦上產生資料時 | font、image、video、audio：超過格式上限報錯，超出建議範圍警告 |
| **不檢查** | 檢查太貴或不可能 | `setWindow` 超出螢幕、datasheet 的原始指令、`readFlash` 的指標不是 `PROGMEM`、`writeWindow` 的陣列不夠長、三角形的角相差 ≥ 2048、蜂鳴器的接法（外部電源、PWM） |

### 哪些有裁切

| 東西 | 裁切 | 原因 |
|---|---|---|
| `ST7789::Window`、`SSD1306::Frame` 的 `fillWindow`、`writeWindow` | ✅ 在這裡裁 | 最底層，裁一次就夠 |
| `graphics` 的圖形和文字 | ✅ | 都經過 `fillWindow` |
| `image` 的兩個類別 | ✅ | 一列分段用 `writeWindow` 畫 |
| `video` 的兩個播放器 | ❌，開始時檢查 | 一幀直接串流進一個窗口（`setWindow`） |
| `setWindow`、datasheet 的原始指令 | ❌ | 直接設定控制器 |

### 浮點數用在哪

UNO 沒有硬體浮點，一次除法約 500 週期，也會多佔 Flash。只有程式用到下面這些時才會編譯進去：

| 用到浮點的地方 | 用途 | 多佔的 Flash（實測） |
|---|---|---|
| `drawLineAngle` | sin / cos | 約 0.6 KB（程式已經有其他浮點時） |
| `drawTexts` 畫 `float` / `double` | 轉成十進位 | 約 1.3 KB |
| `temp_sensor::toCelsius` | 回傳 `float` | 浮點程式庫 |
| 其他圖形、圖片、影片、旋律 | 沒有 | — |

### RAM（AVR 上實測 sizeof，「約」是估計）

| 物件 | bytes | 內容 |
|---|---|---|
| `ST7789::M130T<Bus>` | 8 | 寬、高、偏移量 |
| `graphics::Canvas<M130T<Bus>>` | 27 | 上面 + 字型指標、文字設定 |
| `SSD1306::OLED128x64<Bus>` | 1043 | 整張畫面 1024 + 每個 page 改過的範圍 16 + 位址、狀態 3 |
| `graphics::Canvas<OLED128x64<Bus>>` | 1062 | 上面 + 字型指標、文字設定：UNO 的 RAM 剩不到一半 |
| `image::FlashImage<Screen>` | 約 115 | 一段顏色 64 + 調色盤 32 + 狀態 |
| `image::StreamImage<Screen, Link>` | 約 180 | 上面 + 接收緩衝 64 |
| `video::FlashPlayer<Screen>` | 309 | 解碼器 293（一列緩衝 240、調色盤 32）+ 時間 |
| `video::StreamPlayer<Screen, Link, 64>` | 392 | 解碼器 + 接收緩衝 + 狀態；CHUNK = 16 時 344 |
| `audio::Player<Sound>` | 約 28 | 狀態、時間、旋律指標；旋律在 Flash |
| `temp_sensor::DS18B20<Wire>` | 1 | 解析度；讀取時另外用 9 bytes 堆疊 |
| `switches::Switch<Pin>` | 約 16 | 狀態、時間、次數 |

UNO 只有 2048 bytes RAM。用完就不要的物件（圖片、播放器）放在函式裡，結束時 RAM 就還回去。

### 畫圖的時間（估計，16 MHz、SPI 8 MHz）

| 動作 | 約多少週期 | 說明 |
|---|---|---|
| 一個窗口 | 480 | 設定 CASET、RASET、RAMWR |
| 一個像素 | 44 | SPI 送 2 bytes；8 MHz 的硬體極限是 32 |
| 整個螢幕 | 2.5 M（約 160 ms） | 240×240 像素 |
| 三角形 | 外框方塊裡每點約 100 | 逐點檢查，比較慢 |

## 共同的寫法

- **編譯期檢查**：腳位、鮑率、動畫目標用 `static_assert` 報錯；不建議的用法用 deprecated 成員印警告（`Uno::warn::*`、`video::warn::*`）。
- **命名**：
  - 資料夾、檔案、namespace 都小寫；縮寫和專有名詞維持原樣（ST7789、I2C、RGB565、M130T、Uno）。
  - 多個字的名稱用底線（`flash_player.h`、`Uno_serial.h`、`temp_sensor`）。
  - class、struct、template 型別參數用大駝峰。
- **類別**：先宣告，`// ================= definitions =================` 之後再定義；函式都標 `inline`，不寫成一行。
- **產生的資料**（font、image、video、audio）：
  - `source/convert.py` 產生 `output/*.h`，並加進 `output/selected.h`。
  - `source/check.py` 檢查 `selected.h` 裡的檔案格式是目前的、原始檔沒變。
  - 資料放在 inline 函式裡的 `static PROGMEM` 陣列：程式沒用到的會被編譯器丟掉，不佔 Flash。
- **Python**：用 `python -B` 執行，不產生 `__pycache__`。

## 編譯

- **C++17**：`platformio.ini` 把預設的 `-std=gnu++11` 換成 `-std=gnu++17`。
- **`-mcall-prologues`**：函式開頭、結尾保存暫存器改用共用的程式，省約 1.5 KB，每次呼叫多幾個週期。
- **`inline`**：有沒有寫對大小沒有影響（實測完全一樣）；開了 LTO 由編譯器自己決定要不要展開。
- **`lib_deps`**：`platformio.ini` 直接列出這十二個資料夾，所以程式放在 `src/` 的任何子資料夾都能編譯（PlatformIO 找依賴時會跳過某些子資料夾）。
