# lib

這個專案的函式庫，全部只有標頭檔。硬體用 template 參數傳進來，所以每個函式庫只依賴「介面」，不依賴別的函式庫；換主板只要換 `Uno/`。

## 檔案

```
lib/
├── Uno/            ATmega328P：腳位、SPI、UART、PWM、中斷、計時器；各種介面的 UNO 版本
├── ST7789/         ST7789 彩色螢幕：指令、窗口、模組（M130T）
├── SSD1306/        SSD1306 OLED：指令、板子 RAM 裡的畫面、模組（128×64、128×32）
├── graphics/       圖形和文字：畫在任何有 fillWindow 的螢幕上
├── color/          RGB565 顏色
├── font/           字型（.ttf → .h）
├── image/          靜態圖片（.jpg / .png / .webp / .heic → .h）：Flash 或從 Mac 串流
├── video/          動畫和影片（.gif / .mp4 / .mov / .webm … → .h）：Flash 或從 Mac 串流
├── audio/          旋律（.wav / .mp3 / .m4a / .ogg … → .h）和播放器
├── DS18B20/        DS18B20 溫度感測器（1-Wire）
├── switch/         開關（GND、VCC、OUT）
├── buzzer/         有源 / 無源蜂鳴器
├── NEC/            紅外線遙控器的 NEC 協定：解碼、按鍵、模組（HX1838 接收器、17 鍵遙控器）
└── LED/            LED（PWM 調亮度）：一顆、RGB
```

| 資料夾 | namespace | 入口 | 依賴的介面 |
|---|---|---|---|
| [Uno](Uno/README.md) | `Uno` | 用到哪個介面就 include 哪個檔案 | Arduino（`millis`） |
| [ST7789](ST7789/README.md) | `ST7789` | `ST7789.h` | Bus |
| [SSD1306](SSD1306/README.md) | `SSD1306` | `SSD1306.h` | Bus |
| [graphics](graphics/README.md) | `graphics` | `graphics.h` | Screen |
| [color](color/README.md) | `color` | `color.h` | — |
| [font](font/README.md) | `font` | `font.h` | — |
| [image](image/README.md) | `image`、`picture` | `image.h` | Screen、Link |
| [video](video/README.md) | `video`、`animation` | `video.h` | Screen、Link |
| [audio](audio/README.md) | `audio`、`melody` | `audio.h` | Sound |
| [DS18B20](DS18B20/README.md) | `DS18B20` | `DS18B20.h` | Wire |
| [switch](switch/README.md) | `switches` | `switch.h` | Pin |
| [buzzer](buzzer/README.md) | `buzzer` | `buzzer.h` | Pin、Tone |
| [NEC](NEC/README.md) | `NEC` | `NEC.h` | InterruptPin、Remote |
| [LED](LED/README.md) | `LED` | `LED.h` | Pwm |

## 分類和名稱

規則：**`lib/xxx/` 是晶片或通訊方式，`module/xxx.h` 是實際買到的零件**。程式由晶片的指令或通訊方式決定，同一顆晶片、同一種協定的零件共用程式，只在 `module/` 寫各零件不同的地方（尺寸、初始化、腳位、按鍵碼）。

函式庫分成三大類，每一類的結構固定：

| 大類 | 資料夾 | 名稱用 | 結構 |
|---|---|---|---|
| **主板** | `Uno` | 主板 | 每個介面一個 `Uno_*.h` |
| **硬體**（零件的驅動） | 有晶片：`ST7789`、`SSD1306`、`DS18B20`<br>用協定：`NEC`<br>沒有晶片（例外）：`switch`、`buzzer`、`LED` | 晶片、協定；沒有晶片的用零件種類 | 核心（`basic/` 或根目錄）+ `module/`（實際買到的零件） |
| **資料**（從電腦的檔案轉，或程式算出來的） | 有轉換工具：`font`、`image`、`video`、`audio`<br>沒有：`graphics`（畫圖的演算法）、`color`（顏色常數和換算） | 資料的種類 | 有轉換工具的：程式 + `source/`（`convert.py`、`check.py`）+ `assets/` + `output/` |

- **沒有晶片的零件為什麼是例外**：照規則要用「通訊方式」命名（`GPIO/`、`tone/`、`PWM/`），但那樣有源和無源蜂鳴器會分到兩個資料夾，找起來不直覺，所以保留零件種類的名稱。
- **`module/` 的檔名**：零件名稱；同一種零件有不同接法時加腳數（`active_buzzer_2_pins.h`、`active_buzzer_3_pins.h`、`switch_2_pins.h`）。模組只寫零件不同的地方（尺寸、初始化、電位、接法的檢查），共用的程式在核心。
- **資料要不要再分**：不用。`graphics` 和 `color` 不從檔案轉，但它們也是「給螢幕畫的東西」，跟 image、video 用同一套 Screen 介面；分出第四類只會多一層資料夾，沒有多一條規則。PlatformIO 的 `lib/` 也只認一層資料夾，要分層得另外設定 `lib_extra_dirs`。

## include 的規則

**程式只 include 每個函式庫的入口**（跟資料夾同名：`SSD1306.h`、`buzzer.h`、`LED.h`…），入口會把核心和所有 `module/` 都 include 進來。不要寫 `#include "module/xxx.h"`。

| 為什麼 | 說明 |
|---|---|
| `module/` 有很多個 | PlatformIO 把每個函式庫的資料夾都加進 include 路徑，`"module/xxx.h"` 會在每個函式庫裡找，**找到第一個就用**。兩個函式庫剛好有同名的模組時，會不聲不響地用錯，不會報錯 |
| 入口不會重名 | 入口的檔名就是函式庫的名稱，一定不同 |
| 函式庫裡面用相對路徑 | 入口的 `"module/xxx.h"`、模組的 `"../basic/xxx.h"` 都會先找自己所在的資料夾，所以不會找到別的函式庫 |
| 不多佔空間 | 模組都是 template 和 inline，沒用到的不會編譯進程式 |

所以不用把資料夾改成 `{晶片}_module/`：只要程式不直接 include 模組，同名也不會用錯。
- **改過的名稱**：
  - `I2C/` → `SSD1306/`：I2C 是接線方式，這個資料夾裝的是 SSD1306 晶片的指令；I2C 本身在 `Uno_I2C.h`。
  - `temp_sensor/` → `DS18B20/`：用晶片命名，類別從 `temp_sensor::DS18B20` 改成 `DS18B20::Thermometer`。
  - `infrared/` → `NEC/`：「紅外線」太籠統，程式處理的是 NEC 協定；接收器和遙控器是 `module/`。

## 以後加東西放哪

| 要加的 | 放哪 | 需要的 Uno 介面 |
|---|---|---|
| 別的 SPI / I2C 螢幕（ILI9341、SH1106…） | 新的控制器資料夾（`ILI9341/`），或同一顆晶片的新板子放 `module/` | 現有的 Bus |
| RC522 RFID 讀卡（SDA、SCK、MOSI、MISO、IRQ、RST、3.3V） | `RFID/`（namespace `MFRC522`，像螢幕一樣用晶片命名），`module/RC522.h` | 新的 `Uno_SPI.h`（一般的 SPI 裝置：CS + 讀寫；`Uno_ST7789.h` 是螢幕專用的） |
| 別的 NEC 遙控器（電視、21 鍵 Car MP3） | `NEC/module/` 一個檔案 | — |
| 別的紅外線協定（Sony SIRC、Philips RC5） | 用協定命名：`SIRC/`、`RC5/` | 現有的 InterruptPin |
| 紅外線發射（IR LED） | `NEC/`（同一個協定，反過來送） | 38 kHz 方波（Timer2） |
| DHT11 溫濕度 | 用晶片命名：`DHT11/` | Pin + `nowUs`（它自己的單線協定） |
| 伺服馬達、步進馬達 | `motor/module/` | Pwm（伺服要 50 Hz 的版本）、Pin |
| 可變電阻、光敏電阻、搖桿 | `analog/`（或 `knob/`） | 新的 `UnoAnalogPin`（ADC） |
| 超音波測距（HC-SR04） | `distance/module/HC_SR04.h` | Pin + `nowUs` |
| 七段顯示器、LED 矩陣（MAX7219） | 用晶片命名：`MAX7219/` | `Uno_SPI.h` |

每一個都只依賴 `Uno/` 的介面，不依賴其他函式庫；新的硬體功能先在 `Uno/` 加一個 `Uno_*.h`。

## 介面

介面是「只有 static 函式的 struct」（duck typing）：函式庫用 template 參數接收它，編譯後跟直接寫暫存器一樣快。每個介面的完整清單寫在使用它的標頭檔開頭，也列在各 README 的「介面需求」。

| 介面 | 誰需要 | 需要的函式（摘要） | UNO 的實作 |
|---|---|---|---|
| Bus（SPI 螢幕） | `ST7789` | `init`、`write8/16`、`cmdMode`、`dataMode`、`select`…、`delayMs`、`nowMs`、`readFlash8/16` | `Uno::UnoST7789`（`Uno_ST7789.h`） |
| Bus（I2C） | `SSD1306` | `init`、`start`、`write8`、`read8`、`stop`、`delayMs`、`nowMs`、`readFlash8/16` | `Uno::UnoI2C`（`Uno_I2C.h`） |
| Screen | `graphics`、`image`、`video` | `width`、`height`、`fillWindow`、`writeWindow`、型別 `BusType`（它的 `readFlash8/16`、`nowMs`）；`video` 另外要 `setWindow`、`writeData16`、`endWrite` | `ST7789::M130T`、`SSD1306::OLED128x64` |
| Link | `image::StreamImage`、`video::StreamPlayer` | `init`、`available`、`read8`、`write8`、`clearInput` | `Uno::UnoSerial`（`Uno_serial.h`） |
| Wire | `DS18B20` | `init`、`reset`、`write8`、`read8`、`readBit`、`nowMs` | `Uno::UnoOneWire`（`Uno_one_wire.h`） |
| Pin（輸入） | `switch` | `init`、`read`、`nowMs` | `Uno::UnoPin`（`Uno_pin.h`） |
| Pin（輸出） | `buzzer::ActiveBuzzer2Pins`、`3Pins` | `init`、`write`、`nowMs`、`readFlash16` | `Uno::UnoOutputPin`（`Uno_pin.h`） |
| Pwm | `LED` 的所有模組 | `init`、`write(duty)` | `Uno::UnoPwmPin`（`Uno_pin.h`） |
| InterruptPin | `NEC::Receiver` | `init(onChange)`、`read`、`nowUs`、`nowMs`、`stopInterrupts`、`restoreInterrupts` | `Uno::UnoInterruptPin`（`Uno_interrupt_pin.h`） |
| Remote | 用 `NEC` 的程式 | `ADDRESS`、`toKey(code)` | `NEC::Remote17KeyNumbersTop`、`Remote17KeyArrowsTop` |
| Tone | `buzzer::PassiveBuzzer2Pins`、`3Pins` | `init`、`start`、`stop`、`realHz`、`nowMs`、`readFlash16`、常數 `IDLE_LEVEL_HIGH` | `Uno::UnoTone`（`Uno_tone.h`） |
| Sound | `audio::Player` | `init`、`start`、`stop`、`nowMs`、`readFlash16` | `buzzer` 的所有模組 |

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

Uno_one_wire.h ─(Wire)─▶ DS18B20::Thermometer
Uno_pin.h ─────(Pin)──▶ switches::Switch2Pins、Switch3Pins
Uno_pin.h ─────(Pin)──▶ buzzer::ActiveBuzzer2Pins、3Pins ──┐
Uno_tone.h ────(Tone)─▶ buzzer::PassiveBuzzer2Pins、3Pins ─┴─(Sound)─▶ audio::Player
Uno_pin.h ─────(Pwm)──▶ LED::SingleLED ▶ LED2Pins、RGBLED4Pins、TrafficLight4Pins
Uno_interrupt_pin.h ─(InterruptPin)─▶ NEC::Receiver（HX1838）─(Code)─▶ NEC::Remote17Key… ─▶ NEC::Key
```

- **換主板**：只換 `Uno/`，為新的主板寫同樣函式的 struct。
- **換螢幕模組**：在 `ST7789/module/` 或 `SSD1306/module/` 加一個檔案。
- **換遙控器**：在 `NEC/module/` 加一個有 `ADDRESS`、`toKey` 的 struct，程式只改一行 `using Remote`。
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
| `DS18B20::Thermometer::toCelsius` | 回傳 `float` | 浮點程式庫 |
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
| `DS18B20::Thermometer<Wire>` | 1 | 解析度；讀取時另外用 9 bytes 堆疊 |
| `switches::Switch3Pins<Pin>`（或 `Switch2Pins`） | 約 16 | 狀態、時間、次數 |
| `NEC::Receiver<InterruptPin>` | 約 30（static） | 佇列 4 個碼、解碼狀態、時間 |
| `LED::RGBLED4Pins<…>`、`TrafficLight4Pins<…>` | 3 | 三個顏色的亮度 |

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
  - 資料夾、檔案、namespace 都小寫；縮寫和專有名詞維持原樣（ST7789、SSD1306、DS18B20、NEC、HX1838、I2C、LED、RGB565、M130T、Uno）。
  - 多個字的名稱用底線（`flash_player.h`、`Uno_serial.h`、`remote_17_key.h`）。
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
- **`lib_deps`**：`platformio.ini` 直接列出這十四個資料夾，所以程式放在 `src/` 的任何子資料夾都能編譯（PlatformIO 找依賴時會跳過某些子資料夾）。
