# lib

這個專案的函式庫，全部只有標頭檔。硬體用 template 參數傳進來，所以每個函式庫只依賴「介面」，不依賴別的函式庫；換主板只要換 `Uno/`。

## 檔案

依照四大類（見[分類和名稱](#分類和名稱)）排列：

```
lib/
│   ── 主板 ──
├── Uno/            ATmega328P：腳位（數位、PWM、類比、中斷）、SPI、UART、計時器、背景執行；各種介面的 UNO 版本
│   ── 驅動 ──
├── ST7789/         ST7789 彩色螢幕：指令、窗口、模組（M130T）
├── SSD1306/        SSD1306 OLED：指令、板子 RAM 裡的畫面、模組（128×64、128×32）
├── DS18B20/        DS18B20 溫度感測器（1-Wire）
├── TCRT5000/       TCRT5000 反射式紅外線感測器：HW-006 循跡模組、四腳類比板
├── L298N/          L298N 雙 H 橋馬達驅動：直流馬達、步進馬達、紅色模組；device：兩輪車
├── NEC/            紅外線遙控器的 NEC 協定：接收、發射、按鍵、模組（HX1838、紅外線 LED、17 鍵遙控器）
├── switch/         開關（GND、VCC、OUT）；tool：單擊、雙擊、長按
├── buzzer/         有源 / 無源蜂鳴器
├── LED/            LED（PWM 調亮度）：一顆、RGB；tool：閃爍、呼吸、漸變、心跳
├── ultrasonic/     超音波測距（Trig / Echo）：HC-SR04；tool：中位數濾波
├── analog/         類比感測器：旋鈕、光敏電阻、搖桿、NTC 熱敏、聲音模組
│   ── 素材 ──
├── font/           字型（.ttf → .h）：程式裡寫 FONT("任何路徑") 就自動轉
├── image/          靜態圖片（.jpg / .png / .webp / .heic → .h）：Flash 或從電腦串流
├── video/          動畫和影片（.gif / .mp4 / .mov / .webm … → .h）：Flash 或從電腦串流
├── audio/          聲音（.wav / .mp3 / .m4a / .ogg … → 音符、節奏或取樣）和播放器：Flash 或從電腦串流
│   ── 基礎 ──
├── color/          顏色格式：RGB565、RGB888、RGB444、RGB332、BW…；螢幕的 ColorFormat
├── graphics/       圖形和文字：畫在任何有 fillWindow 的螢幕上
└── stream/         跟電腦要檔案的協定：板子的 Puller、電腦的 stream.py（圖片、動畫、聲音共用）
```

| 大類 | 資料夾 | namespace | 入口 | 依賴的介面 |
|---|---|---|---|---|
| 主板 | [Uno](Uno/README.md) | `Uno` | 用到哪個介面就 include 哪個檔案 | Arduino（`millis`、`micros`） |
| 驅動 | [ST7789](ST7789/README.md) | `ST7789` | `ST7789.h` | Bus、ColorFormat |
| 驅動 | [SSD1306](SSD1306/README.md) | `SSD1306` | `SSD1306.h` | Bus、ColorFormat |
| 驅動 | [DS18B20](DS18B20/README.md) | `DS18B20` | `DS18B20.h` | Wire |
| 驅動 | [TCRT5000](TCRT5000/README.md) | `TCRT5000` | `TCRT5000.h` | Pin、AnalogPin |
| 驅動 | [L298N](L298N/README.md) | `L298N` | `L298N.h` | Pin（輸出）、Pwm |
| 驅動 | [NEC](NEC/README.md) | `NEC` | `NEC.h` | InterruptPin、Carrier、Remote |
| 驅動 | [switch](switch/README.md) | `switches` | `switch.h` | Pin |
| 驅動 | [buzzer](buzzer/README.md) | `buzzer` | `buzzer.h` | Pin、Tone |
| 驅動 | [LED](LED/README.md) | `LED` | `LED.h` | Pwm |
| 驅動 | [ultrasonic](ultrasonic/README.md) | `ultrasonic` | `ultrasonic.h` | Trigger、Echo |
| 驅動 | [analog](analog/README.md) | `analog` | `analog.h` | AnalogPin、Pin |
| 素材 | [font](font/README.md) | `font` | `font.h` | — |
| 素材 | [image](image/README.md) | `image`、`picture` | `image.h` | Screen（含 ColorFormat）、Stream |
| 素材 | [video](video/README.md) | `video`、`animation` | `video.h` | Screen（含 ColorFormat）、Stream |
| 素材 | [audio](audio/README.md) | `audio`、`melody` | `audio.h` | Sound、SampleOut、Stream |
| 基礎 | [color](color/README.md) | `color` | `color.h` | — |
| 基礎 | [graphics](graphics/README.md) | `graphics` | `graphics.h` | Screen（含 ColorFormat） |
| 基礎 | [stream](stream/README.md) | `stream` | `stream.h` | Link |

## 分類和名稱

規則：**`lib/xxx/` 是晶片或通訊方式，`module/xxx.h` 是實際買到的零件**。程式由晶片的指令或通訊方式決定，同一顆晶片、同一種協定的零件共用程式，只在 `module/` 寫各零件不同的地方（尺寸、初始化、腳位、按鍵碼）。

函式庫分成四大類，每一類的結構固定：

| 大類 | 是什麼 | 資料夾 | 名稱用 | 結構 |
|---|---|---|---|---|
| **主板**（board） | 某一塊板子的硬體：腳位、計時器、串口…；其他類的介面的實作 | `Uno` | 主板 | 每個介面一個 `Uno_*.h` |
| **驅動**（driver） | 接在板子上的零件 | 有晶片：`ST7789`、`SSD1306`、`DS18B20`、`TCRT5000`、`L298N`<br>用協定：`NEC`<br>沒有晶片（例外）：`switch`、`buzzer`、`LED`、`ultrasonic`、`analog` | 晶片、協定；沒有晶片的用零件種類 | 核心（`basic/` 或根目錄）+ `module/`（實際買到的零件）+ 需要時：`tool/`（建在它上面的常用功能）、`device/`（接好的一整台東西） |
| **素材**（asset） | 從電腦的檔案轉來的內容，和播放它的類別 | `font`、`image`、`video`、`audio` | 素材的種類 | 程式 + `source/`（`convert.py`、`check.py`、`stream_handler.py`）+ `assets/` + `output/` |
| **基礎**（common） | 不接硬體、不轉檔案，給其他類共用的規格和演算法 | `color`（顏色格式）、`graphics`（畫圖）、`stream`（跟電腦要檔案的協定） | 它定義的東西 | 根目錄的標頭檔；`stream` 另有電腦端的 `source/stream.py` |

- **怎麼分**：接硬體 → 主板或驅動（主板是「板子本身」，驅動是「接上去的零件」）；有 `convert.py` → 素材；其他 → 基礎。
- **依賴的方向**：素材、驅動、基礎都不 include 別的函式庫，彼此只透過 template 參數（介面）合作；主板實作這些介面。
- **`module/`、`tool/`、`device/`**：

  | 資料夾 | 放什麼 | 例子 |
  |---|---|---|
  | `module/` | 實際買到的零件：只寫它跟別的零件不同的地方 | `HC_SR04.h`、`KY_023.h`、`L298N_board.h` |
  | `tool/` | 建在核心上的常用功能，任何零件都能用 | `switch/tool/clicks.h`（單擊、雙擊、長按）、`LED/tool/effect.h`（呼吸、閃爍）、`ultrasonic/tool/median.h`、`ST7789/tool/init.h` |
  | `device/` | 用這個晶片接成的一整台東西，一個 class 就是一種接法，參數可以調 | `L298N/device/two_wheel_car.h` |

  三種都只用自己函式庫的核心，所以還是不依賴別的函式庫；跨函式庫的組合（例如循跡車 = L298N + TCRT5000）寫在程式裡。
- **沒有晶片的零件為什麼是例外**：照規則要用「通訊方式」命名（`GPIO/`、`tone/`、`PWM/`），但那樣有源和無源蜂鳴器會分到兩個資料夾，找起來不直覺，所以保留零件種類的名稱。
- **`module/` 的檔名**：零件名稱；同一種零件有不同接法時加腳數（`active_buzzer_2_pins.h`、`active_buzzer_3_pins.h`、`switch_2_pins.h`）。模組只寫零件不同的地方（尺寸、初始化、電位、接法的檢查），共用的程式在核心。
- **大類只寫在文件裡，資料夾不分層**：PlatformIO 的 `lib/` 只認一層資料夾，要分層得另外設定 `lib_extra_dirs`，也會讓 include 路徑變長。

## include 的規則

**程式只 include 每個函式庫的入口**（跟資料夾同名：`SSD1306.h`、`buzzer.h`、`LED.h`…），入口會把核心和所有 `module/` 都 include 進來。不要寫 `#include "module/xxx.h"`。

| 為什麼 | 說明 |
|---|---|
| `module/` 有很多個 | PlatformIO 把每個函式庫的資料夾都加進 include 路徑，`"module/xxx.h"` 會在每個函式庫裡找，**找到第一個就用**。兩個函式庫剛好有同名的模組時，會不聲不響地用錯，不會報錯 |
| 入口不會重名 | 入口的檔名就是函式庫的名稱，一定不同 |
| 函式庫裡面用相對路徑 | 入口的 `"module/xxx.h"`、模組的 `"../basic/xxx.h"` 都會先找自己所在的資料夾，所以不會找到別的函式庫 |
| 不多佔空間 | 模組都是 template 和 inline，沒用到的不會編譯進程式 |

所以不用把資料夾改成 `{晶片}_module/`：只要程式不直接 include 模組，同名也不會用錯。

**改過的名稱**（舊的程式要跟著改）：

- `I2C/` → `SSD1306/`：I2C 是接線方式，這個資料夾裝的是 SSD1306 晶片的指令；I2C 本身在 `Uno_I2C.h`。
- `temp_sensor/` → `DS18B20/`：用晶片命名，類別從 `temp_sensor::DS18B20` 改成 `DS18B20::Thermometer`。
- `infrared/` → `NEC/`：「紅外線」太籠統，程式處理的是 NEC 協定；接收器和遙控器是 `module/`。

## 字型、圖片、影片、聲音：直接用檔案

程式裡直接寫檔案，編譯前會自動轉換，不用先跑 `convert.py`，檔案也不用放在 `assets/`：

```cpp
using Title = FONT("~/Fonts/Inter.ttf", "--height 16");                      // font::…
using Cat   = PICTURE("photos/cat.jpg", "--fit 128x64 --palette 2");          // picture::…
using Orbit = ANIMATION("orbit.gif", "--width 64 --mono");                    // animation::…
using Chime = MELODY("success.m4a");                                          // melody::…
```

| 項目 | 說明 |
|---|---|
| 路徑 | 絕對路徑、`~/…`、相對於寫它的 .cpp；找不到時再試專案資料夾、`lib/<種類>/assets/` |
| 選項 | 一個字串，**就是** `convert.py` 的選項（見各 `source/README.md`），預設也一樣；可以省略 |
| 省略時 | `convert.py` 的預設：字型 `--height 16`；圖片、影片、聲音 `--target flash`（聲音還有 `--sound passive --quality medium`） |
| 轉到哪 | `lib/<種類>/output/`，跟手動跑 `convert.py` 的**完全一樣**（同樣的檔名、同樣的內容），不另外分類 |
| 誰轉 | `tools/assets.py`（`platformio.ini` 的 `extra_scripts`）：每次編譯前找出 `src/` 裡的這四個巨集；`output/` 裡已經有「同一個檔案、同樣的選項」轉出來的就直接用，**沒有才轉** |
| 怎麼找到已經轉好的 | 每個 .h 開頭有 `// source: 路徑 sha256 …` 和 `// options: …`（只看檔名和檔案裡的資訊）；選項的順序、`--fit=…` 的寫法不影響 |
| 名稱 | `assets/` 裡的檔案：跟以前一樣（`sunset_flash_120x80_palette_16`）；其他地方：前面加上資料夾（相對於專案，不在專案裡就相對於家目錄），例如 `~/Mine/4_assets/audio/…/16 戦い：ウォロ.aiff` → `mine_4_assets_audio_…_16_7b5dc7_flash_passive_medium`（日文拿掉，加 6 碼雜湊）；撞名加 `_2`、`_3`，不會蓋掉別的 |
| 原始檔改過 / 不見了 | 改過：編譯時警告，`python tools/assets.py --force` 重轉；移走或刪掉：輸出還在就照樣能編譯 |
| 怎麼變成型態 | 巨集在編譯時把路徑和選項算成一個雜湊值，`output/registry.h` 裡有每個雜湊值對應的型態（模擬「工廠」：給檔案，得到它的型態）；那個型態也能直接用名稱寫 |
| 限制 | 兩個參數都要是字串常數（不能是變數或巨集），因為是在編譯前讀原始碼 |
| 空間 | 跟以前一樣：沒用到的型態不佔 Flash |

`output/selected.h` 每次編譯前自動寫，列出 `output/` **全部**的 .h，所以手動轉的、巨集轉的都能直接用名稱（`picture::sunset_flash_…`），不用手動編輯。全部列出**不佔 Flash**：資料在 `inline` 函式裡的 `static const … PROGMEM` 陣列，函式沒被呼叫就不會產生，連結器也會丟掉沒用到的（實測 Flash、RAM 跟只列用到的完全一樣）。

不要的檔案：`python tools/assets.py --unused` 列出沒有程式（包括 `.bak`）用的，自己刪。全部刪掉或全部重新轉換：`python src/clean.py`（`--rebuild`），見 [src/README.md](../src/README.md#cleanpy)。

## 以後加東西放哪

| 要加的 | 放哪 | 需要的 Uno 介面 |
|---|---|---|
| 別的 SPI / I2C 螢幕（ILI9341、SH1106…） | 新的控制器資料夾（`ILI9341/`），或同一顆晶片的新板子放 `module/` | 現有的 Bus |
| RC522 RFID 讀卡（SDA、SCK、MOSI、MISO、IRQ、RST、3.3V） | `RFID/`（namespace `MFRC522`，像螢幕一樣用晶片命名），`module/RC522.h` | 新的 `Uno_SPI.h`（一般的 SPI 裝置：CS + 讀寫；`Uno_ST7789.h` 是螢幕專用的） |
| 別的 NEC 遙控器（電視、21 鍵 Car MP3） | `NEC/module/` 一個檔案 | — |
| 別的紅外線協定（Sony SIRC、Philips RC5） | 用協定命名：`SIRC/`、`RC5/` | 現有的 InterruptPin |
| DHT11 溫濕度 | 用晶片命名：`DHT11/` | Pin + `nowUs`（它自己的單線協定） |
| 別的馬達驅動晶片（TB6612FNG、L9110S、A4988 步進） | 用晶片命名：`TB6612FNG/`…，`DCMotor`、`Stepper` 的寫法可以照 `L298N/` | Pin、Pwm |
| 伺服馬達（SG90） | `servo/module/`（沒有共同的晶片，用零件種類） | 50 Hz 的 Pwm（Timer1） |
| 別的類比感測器（土壤濕度、壓力、火焰、霍爾類比） | `analog/module/` 一個檔案（多半是 `Input` 加上換算） | AnalogPin |
| 28BYJ-48 步進馬達 + ULN2003 板 | 用晶片命名：`ULN2003/`（單極步進，4 隻 IN） | Pin（輸出） |
| 別的超音波測距（JSN-SR04T 防水、US-100） | `ultrasonic/module/` 一個檔案（改 `MIN_MM`、`MAX_MM`） | 現有的 Trigger、Echo |
| 別的 TCRT5000 板子（KY-033） | `TCRT5000/module/` 一個檔案 | Pin |
| 紅外線避障模組（FC-51、HW-201：LED 和接收管分開） | 用零件種類命名：`IR_obstacle/`（沒有共同的晶片） | Pin |
| 七段顯示器、LED 矩陣（MAX7219） | 用晶片命名：`MAX7219/` | `Uno_SPI.h` |

每一個都只依賴 `Uno/` 的介面，不依賴其他函式庫；新的硬體功能先在 `Uno/` 加一個 `Uno_*.h`。

## 介面

介面是「只有 static 函式的 struct」（duck typing）：函式庫用 template 參數接收它，編譯後跟直接寫暫存器一樣快。每個介面的完整清單寫在使用它的標頭檔開頭，也列在各函式庫 INTERFACE.md 的「介面需求」。

UNO 的實作全部繼承 `Uno::UnoBoard`（`nowMs`、`nowUs`、`delayMs`、`delayUs`、`readFlash8/16`、`stopInterrupts`、`restoreInterrupts`），腳位再從 `UnoBasicPin` 往下分（見 [Uno 的分級](Uno/INTERFACE.md#分級)），所以下表「需要的函式」裡的時間、Flash、關中斷，每個 UNO 介面都有。

| 介面 | 誰需要 | 需要的函式（摘要） | UNO 的實作 |
|---|---|---|---|
| Bus（SPI 螢幕） | `ST7789` | `init`、`write8/16`、`cmdMode`、`dataMode`、`select`…、`delayMs`、`nowMs`、`readFlash8/16` | `Uno::UnoST7789`（`Uno_ST7789.h`） |
| Bus（I2C） | `SSD1306` | `init`、`start`、`write8`、`read8`、`stop`、`delayMs`、`nowMs`、`readFlash8/16` | `Uno::UnoI2C`（`Uno_I2C.h`） |
| Screen | `graphics`、`image`、`video` | 型別 `ColorFormat`、`BusType`（它的 `readFlash8/16`、`nowMs`）、`width`、`height`、`fillWindow`、`writeWindow`；`video` 另外要 `setWindow`、`writePixel`、`endWrite` | `ST7789::M130T`、`SSD1306::OLED128x64` |
| Link | `stream::Puller` | `init`、`available`、`read8`、`write8`、`clearInput`、`readFlash8` | `Uno::UnoSerial`（`Uno_serial.h`） |
| Stream | `image::StreamImage`、`video::StreamPlayer`、`audio::StreamPlayer`、`StreamSamplePlayer` | `CHUNK_SIZE`、`OK`、`ERROR_*`、`begin`、`write8/16/32`、`writePath`、`waitAnswer`、`available`、`pullLength`、`receive`、類別 `Answer` | `stream::Puller<Link, CHUNK>`（`lib/stream`） |
| ColorFormat | `ST7789::Window`、`SSD1306::Frame`（螢幕宣告它），graphics、font、image、video 從 `Screen::ColorFormat` 拿 | `Value`、`BITS`、常數、`color(r, g, b)`、`red/green/blue`、`fromRGB565`；ST7789 另外要 `R/G/B_BITS`、`ORDER`、`…Field` | `color::Packed<…>`（`RGB565`、`RGB888`、`RGB444`…）、`color::BW<門檻>`（`lib/color`） |
| Wire | `DS18B20` | `init`、`reset`、`write8`、`read8`、`readBit`、`nowMs` | `Uno::UnoOneWire`（`Uno_one_wire.h`） |
| Pin（輸入） | `switch`、`TCRT5000`、搖桿的按鈕 | `init`、`read`、`nowMs` | `Uno::UnoInputPin`（`Uno_pin.h`）、`UnoInterruptPin`、`UnoAnalogPin`（都能讀數位） |
| Pin（輸出） | `buzzer::ActiveBuzzer2Pins`、`3Pins`、`ultrasonic::Ranger`（Trigger，只用 `init`、`write`）、`L298N::Bridge`（IN） | `init`、`write`、`nowMs`、`nowUs`、`readFlash8/16` | `Uno::UnoOutputPin`（`Uno_pin.h`） |
| Pwm | `LED` 的所有模組、`L298N::Bridge`（EN）、`InputPwmBridge`（IN，另外要 `nowMs`、`nowUs`、`stopInterrupts`） | `init`、`write(duty)` | `Uno::UnoPwmPin`（`Uno_pin.h`） |
| InterruptPin | `NEC::Receiver`、`ultrasonic::Ranger`（Echo） | `init(onChange)`、`read`、`nowUs`、`nowMs`、`stopInterrupts`、`restoreInterrupts` | `Uno::UnoInterruptPin`（`Uno_interrupt_pin.h`） |
| AnalogPin | `analog` 的全部、`TCRT5000::AnalogReflector` | `init`、`readAnalog`、`readAnalogAverage`、`referenceMillivolts`、`nowMs`、常數 `MAX` | `Uno::UnoAnalogPin`（`Uno_analog_pin.h`） |
| 背景 | 任何有 `update()` 的東西（`L298N` 的類別有保護） | 一個沒有參數的函式 | `Uno::UnoBackground`（`Uno_background.h`） |
| Carrier | `NEC::Transmitter` | `init`、`on`、`off`、`delayUs`、`nowMs` | `Uno::UnoIrCarrier`（`Uno_ir_carrier.h`） |
| Remote | 用 `NEC` 的程式 | `ADDRESS`、`toKey(code)`、`toCommand(key, command)` | `NEC::Remote17KeyNumbersTop`、`Remote17KeyArrowsTop` |
| Tone | `buzzer::PassiveBuzzer2Pins`、`3Pins` | `init`、`start`、`stop`、`realHz`、`nowMs`、`readFlash8/16`、常數 `IDLE_LEVEL_HIGH` | `Uno::UnoTone`（`Uno_tone.h`） |
| Sound | `audio::Player`、`StreamPlayer` | 常數 `HAS_PITCH`、`init`、`start`、`stop`、`nowMs`、`readFlash8/16` | `buzzer` 的所有模組 |
| SampleOut | `audio::SamplePlayer`、`StreamSamplePlayer` | `init`、`start(rate, next)`、`stop`、`realRate`、`stopInterrupts`、`restoreInterrupts`、`readFlash8/16` | `Uno::UnoPcmOut`（`Uno_pcm_out.h`） |

## 提示和錯誤

三個地方會出現訊息，各自的規則：

| 什麼時候 | 誰說 | 在哪裡看到 | 寫在哪 |
|---|---|---|---|
| 在電腦上跑 Python（`convert.py`、`check.py`、`stream.py`、`tools/assets.py`、`src/hide.py`、`src/clean.py`） | 那個腳本 | 終端機 | 腳本自己 `print` |
| 編譯 C++ | 函式庫的 `static_assert`、警告 | 終端機（`pio run`）、VS Code 的問題清單 | 檢查的那一行旁邊（標頭檔裡） |
| 板子執行時 | 函式回傳結果代碼；程式決定要不要顯示 | **不在終端機**：串口（`pio device monitor`、`stream.py` 的 `[BOARD]`）、螢幕、LED | 函式庫回傳代碼，程式印出來 |

**為什麼不做 `lib/error` 或 `lib/Uno/error`**：編譯的訊息必須寫在 `static_assert` 那一行，搬不走；執行時的代碼屬於各個類別（`StreamImage::ERROR_NO_ANSWER`）。做成一個共用的函式庫，每個函式庫都要 include 它，就違反「函式庫不互相依賴」。所以統一的是**規則**，寫在這裡，各自照著寫。

### Python

| 標籤 | 用在 |
|---|---|
| `[INFO]` | 正在做什麼、結果的摘要 |
| `[SUCCESS]` | 一件事完成了（存好檔案、送完一個回答） |
| `[WARNING]` | 能繼續，但結果可能不是你要的（超出建議範圍、很慢、會用很多 RAM） |
| `[ERROR]` | 做不下去；程式內部的錯寫成 `[ERROR] internal: …` |
| `[BOARD]` | `stream.py` 轉印板子送來的文字 |

- **格式**：`[標籤] 誰: 什麼事`，例如 `[ERROR] week_3/…/1_screen.cpp:12: convert.py failed`。
- **補充說明**：下一行起縮排 4 個空白，不加標籤。
- **分隔線**：一次做好幾件工作時（例如轉好幾個檔案），每件前面一行 `---------- 名稱 ----------`，跟程式的段落標記一樣。
- **參數錯誤**：用 `argparse` 的 `parser.error()`（印用法和錯誤，結束碼 2），不印 traceback。

### 編譯（C++）

| 種類 | 寫法 | 例子 |
|---|---|---|
| 錯誤：一定不能用 | `static_assert(條件, "namespace::類別: 哪裡不對（怎麼改）")` | `"Uno::UnoST7789: DC is required and must be 0 ~ 19"` |
| 警告：能用但不建議 | `namespace::warn::名稱<條件>`：一個 deprecated 的常數，訊息說「會怎樣、怎麼避免」 | `image::warn::PaletteInRam`、`Uno::warn::…` |
| 巨集的錯誤 | `"巨集名稱(...): …"` | `"PICTURE(...): …"` |

- 訊息用英文（跟程式碼一樣），開頭一定是 `namespace::類別:`，在幾百行的編譯輸出裡也找得到是誰說的。
- 能在編譯時查的就不要留到執行時。

### 執行時（板子）

| 規則 | 說明 |
|---|---|
| 只有成功 / 失敗 | 回傳 `bool`（例如 OLED 的 `init()`） |
| 有好幾種原因 | 回傳 `uint8_t` 代碼：`OK` = 0，失敗是 `ERROR_名稱`（≥ 1），常數放在類別裡 |
| 同一種協定的代碼一樣 | 串流全部用 `Stream` 的：`OK`、`ERROR_NOT_FOUND`（1）～ `ERROR_NO_MEMORY`（6）；類別自己的接在後面（`video::StreamPlayer::ERROR_TOO_BIG` = 7） |
| 函式庫不印東西 | 函式庫不知道有沒有串口、螢幕；只回傳代碼 |
| 程式印的時候 | 串口上跟 Python 一樣：出問題的那一行用 `[ERROR] …`、`[WARNING] …` 開頭；平常的量測結果不加標籤 |

## 前提條件總覽

各函式庫跨在一起的假設，細節在各自 INTERFACE.md 的「前提條件」：

| 前提 | 誰有 | 一致的做法 |
|---|---|---|
| **顏色格式** | 螢幕（`ST7789`、`SSD1306`）宣告；`graphics`、`font`、`image`、`video` 使用 | 只寫在螢幕上：`M130T<Bus, color::RGB565>`、`OLED128x64<Bus, color::BW<>>`；其他函式庫一律用 `Screen::ColorFormat`、顏色的型別一律 `Screen::ColorFormat::Value`。螢幕不支援的格式在編譯時擋下 |
| **檔案的顏色** | `image`、`video`（`convert.py` 的輸出、串流） | 一律 RGB565，畫的時候用 `ColorFormat::fromRGB565()` 換；調色盤在開始時換一次 |
| **串流** | `image::StreamImage`、`video::StreamPlayer`、`audio::StreamPlayer`、`StreamSamplePlayer` | 都用 `Stream` 參數（`stream::Puller<Link, CHUNK>`），結果代碼都一樣（`OK`、`ERROR_NOT_FOUND` ～ `ERROR_NO_MEMORY`）；電腦開一個 `stream.py` |
| **執行時配置的 RAM** | 只有串流 256 色的調色盤（`image`、`video` 的 `HeapPalette`） | 編譯時警告（`PaletteInRam`）；`malloc` 失敗回傳 `ERROR_NO_MEMORY`；`image` 在 `draw()` 回來前、`video` 在 `close()` / 下一次開啟 / 物件消失時 `free`。其他 RAM 全在物件或 static 裡，編譯器算得到 |
| **一直呼叫 `update()`** | `audio`、`video`、`switch`、`LED::Effect`、`TCRT5000`、`analog`、`ultrasonic`、`L298N` | 函式馬上回來，`update()` 做下一步；可以交給 `Uno::UnoBackground` |
| **計時器、中斷** | `Uno::UnoTone`、`UnoPcmOut`（Timer1、Timer2）、`UnoIrCarrier`（Timer2）、`UnoInterruptPin`（腳位改變中斷） | 衝突寫在 [Uno/INTERFACE.md](Uno/INTERFACE.md)；能在編譯時查的都用 `static_assert` 或警告 |
| **一條串口** | `stream`（`Uno::UnoSerial`，D0 / D1） | 不能同時用 Arduino 的 `Serial`、`pio device monitor` |

## 依賴關係

```
Uno_ST7789.h ─(Bus)─▶ ST7789::Datasheet ▶ Window ▶ M130T ──────┐
Uno_I2C.h ───(Bus)─▶ SSD1306::Datasheet ▶ Frame ▶ OLED128x64 ─┤ Screen
                                                                ▼
                     graphics::Shapes ▶ Text（◀─ font）= graphics::Canvas<Screen>
                                                                │
                     image::FlashImage、StreamImage ◀───────────┤（writeWindow：任何螢幕）
                     video::FlashPlayer、StreamPlayer ◀─────────┘（setWindow：任何螢幕）
color ─(ColorFormat)─▶ ST7789::M130T、SSD1306::OLED… ─(Screen::ColorFormat)─▶ graphics、font、image、video
Uno_serial.h ─(Link)─▶ stream::Puller ─(Stream)─▶ image::StreamImage、video::StreamPlayer、audio::StreamPlayer、StreamSamplePlayer

Uno_one_wire.h ─(Wire)─▶ DS18B20::Thermometer
Uno_pin.h ─────(Pin)──▶ switches::Switch2Pins、Switch3Pins ─▶ tool: switches::Clicks
Uno_pin.h ─────(Pin)──▶ TCRT5000::Reflector ▶ HW006
Uno_analog_pin.h ─(AnalogPin)─▶ TCRT5000::AnalogReflector（四腳板）
Uno_analog_pin.h ─(AnalogPin)─▶ analog::Input ▶ Potentiometer、Photoresistor；Joystick ▶ KY023；Thermistor ▶ KY013；SoundLevel ▶ KY037
Uno_pin.h ───(Trigger)─┐
Uno_interrupt_pin.h ─(Echo)─▶ ultrasonic::Ranger ▶ HCSR04 ─▶ tool: ultrasonic::Median
Uno_pin.h ─────(Pin)──▶ buzzer::ActiveBuzzer2Pins、3Pins ──┐
Uno_tone.h ────(Tone)─▶ buzzer::PassiveBuzzer2Pins、3Pins ─┴─(Sound)─▶ audio::Player、StreamPlayer
Uno_pcm_out.h ─(SampleOut)──────────────────────────────────────▶ audio::SamplePlayer、StreamSamplePlayer
Uno_pin.h ─────(Pwm)──▶ LED::SingleLED ▶ LED2Pins、RGBLED4Pins、TrafficLight4Pins；tool: LED::Effect
Uno_pin.h ─(Pin、Pwm)─▶ L298N::Bridge（L298NBoard::A、B）─(Bridge)─▶ L298N::DCMotor、Stepper ─▶ device: TwoWheelCar
Uno_background.h ─(背景)─▶ 任何 update()（例如 car.update()）
Uno_interrupt_pin.h ─(InterruptPin)─▶ NEC::Receiver（HX1838）─(Code)─▶ NEC::Remote17Key… ─▶ NEC::Key
Uno_ir_carrier.h ─(Carrier)─▶ NEC::Transmitter（IRLED、KY005）◀─(Key → command)─ NEC::Remote17Key…
```

- **換主板**：只換 `Uno/`，為新的主板寫同樣函式的 struct。
- **換螢幕模組**：在 `ST7789/module/` 或 `SSD1306/module/` 加一個檔案。
- **換遙控器**：在 `NEC/module/` 加一個有 `ADDRESS`、`toKey` 的 struct，程式只改一行 `using Remote`。
- **換發聲的東西**：寫一個有 Sound 函式的 struct，`audio::Player` 就能播。
- **`color` 和 `font` 不依賴任何東西**；`graphics` 本身沒有顏色常數。
- **換螢幕的顏色格式**：只改螢幕的 `ColorFormat`（例如 `M130T<Bus, color::RGB888>`），graphics、font、image、video 都不用改；新的格式寫一個有 ColorFormat 成員的 struct（見 [color/INTERFACE.md](color/INTERFACE.md)）。
- **串流新的種類**：板子用 `stream::Puller`，電腦加一個 `lib/<種類>/source/stream_handler.py`，`stream.py` 會自己載入。

## README 的格式

每份 README 用同樣的結構和用詞，沒有的段落就省略。

**每個函式庫兩份文件**：

| 檔案 | 給誰 | 段落（依序，沒有的就省略） |
|---|---|---|
| `README.md` | 使用者：怎麼用、怎麼接、要小心什麼 | 標題 + 「需要 / 得到」表 → 一句「細節在 INTERFACE.md」→ 一段說明 → `## 用法` → 其他用法 → `## 接線` → 給使用者的主題段落 → `## 輸入限制` → `## 注意事項` |
| `INTERFACE.md` | 開發者：寫新套件、換主板、換零件 | `# 名稱：介面` + 同一張「需要 / 得到」表 → 一句「用法見 README.md」→ `## 前提條件` → `## 檔案` → `## 介面需求`（或 `## 提供的介面`）→ `## 函式` → `## 參數` → 給開發者的主題段落 → `## 輸入限制` → `## 資源` |

兩份會有重複（「需要 / 得到」表、輸入限制）：讓兩邊都不用跳來跳去。

| 段落 | 內容 |
|---|---|
| 「需要 / 得到」表 | 第一眼就看到：要的介面（duck typing，括號裡是現成的 UNO 實作）→ 得到的類別和它能做的事 |
| `## 用法` | 最短的完整範例（腳位先寫成 `constexpr uint8_t X_PIN = …;` 再放進 `Uno::Uno…<X_PIN>`），最後一行「完整的例子：`src/…`」 |
| 其他用法 | 額外的類別或功能的範例，例如 `## Clicks`、`## 發射`、`## 直接用檔案（推薦）` |
| `## 接線` | 硬體的腳位表、電源、上拉、電流 |
| 給使用者的主題段落 | 零件的名稱、原理、怎麼選（格式和品質、三種目標、解析度…） |
| `## 前提條件` | 表格「項目 / 條件」：這個函式庫假設了什麼——介面、顏色格式、串流、要一直呼叫 `update()`、計時器和中斷、執行時配置的 RAM 和什麼時候釋放 |
| `## 檔案` | 資料夾樹，每個檔案一行說明 |
| `## 介面需求` | template 參數要的 static 函式（duck typing）、現成的實作 |
| `## 函式` | 每個公開函式，說明以動詞開頭（「畫…」「回傳…」「設定…」） |
| `## 參數` | template 參數、預設值 |
| 給開發者的主題段落 | 協定、資料格式、畫法、為什麼這樣設計 |
| `## 輸入限制` | 表格「項目 / 範圍 / 超出時」 |
| `## 資源` | RAM、Flash、時間、佔用的硬體（腳位、計時器、中斷） |
| `## 注意事項` | 表格「情況 / 結果 / 做法」：不能做的事、常見問題 |

**`source/`、`output/`、`module/` 的 README**：本來就是給開發者的，只有一份。

**`source/` 的 README**：一段說明（需要的套件）→ `## 檔案` → `## 用法` → `## convert.py`（支援的檔案、參數、流程、錯誤和警告）→ `## check.py` →（`## stream_handler.py`：給 `lib/stream` 的 `stream.py` 用）。

**`output/` 的 README**：一段說明 → `## 檔案` → `## selected.h、registry.h` → `## 目前的…` → `## 產生的 struct` → `## 格式（format N）` → `## 名稱和來源`。

**`src/` 的 README 和程式**：見 [src/README.md](../src/README.md)。

**用詞**：

| 用 | 不用 |
|---|---|
| 函式、回傳、呼叫 | 方法、返回值、調用 |
| 腳位、Flash、RAM、週期、工作週期 | 引腳、flash、記憶體（指 RAM 時）、cycle、佔空比 |
| 預設、範圍、上限、下限 | 默認、區間 |
| 介面（duck typing 的 struct） | 接口、協議 |
| 「超出時」只用：編譯錯誤、編譯警告、裁切、夾到範圍內、不畫 / 安靜、回傳錯誤、`convert.py` 報錯、不檢查、— | 其他說法 |
| 數字和單位之間空一格：20 ms、2 KB；範圍寫 0 ～ 19 | 20ms、0-19 |

## 比較

### 不合理的輸入怎麼處理

| 處理 | 什麼時候 | 例子 |
|---|---|---|
| **編譯錯誤** | 編譯時就知道一定不行 | 腳位不存在、SPI 腳被拿來當 DC / RES / CS / BLK、沒有 CS 卻不是 SPI mode 3、鮑率誤差 > 5%、`FlashImage` / `FlashPlayer` 拿到只有 stream 的圖片、動畫、螢幕不支援的顏色格式（ST7789 + RGB332 / BW、SSD1306 + 彩色） |
| **編譯警告** | 能動，但不建議 | 兩個訊號同一隻腳、用到 D0 / D1、BLK 不是 PWM 腳或用 Timer0、鮑率誤差 > 2%、`StreamImage` / `StreamPlayer` 拿到只有 flash 的圖片、動畫或 256 色（`PaletteInRam`：執行時 `malloc`） |
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
| `ST7789::M130T<Bus, ColorFormat>` | 約 10 | 寬、高、偏移量；12 bit 的半個像素 |
| `graphics::Canvas<M130T<Bus, ColorFormat>>` | 約 29 | 上面 + 字型指標、文字設定 |
| `SSD1306::OLED128x64<Bus, ColorFormat>` | 1043 | 整張畫面 1024 + 每個 page 改過的範圍 16 + 位址、狀態 3 |
| `graphics::Canvas<OLED128x64<Bus, ColorFormat>>` | 1062 | 上面 + 字型指標、文字設定：UNO 的 RAM 剩不到一半 |
| `image::FlashImage<Screen>` | 約 115 | 一段顏色 64 + 調色盤 32 + 狀態 |
| `image::StreamImage<Screen, Stream>` | 約 180 | 上面 + `Stream::Answer`（CHUNK = 64）；串流 256 色時畫的期間另外 `malloc` 調色盤 |
| `video::FlashPlayer<Screen>` | 309 | 解碼器 293（一列緩衝 240、調色盤 32）+ 時間 |
| `video::StreamPlayer<Screen, Stream>` | 約 394 | 解碼器 + `Stream::Answer`（CHUNK = 64）+ 狀態；CHUNK = 16 時約 346；串流 256 色時另外 `malloc` 調色盤，直到 `close()` |
| `audio::Player<Sound>` | 約 32 | 狀態、時間、旋律指標、編碼；旋律在 Flash |
| `audio::StreamPlayer<Sound, Stream>` | 約 30 + CHUNK | 狀態 + `Stream::Answer` |
| `audio::StreamSamplePlayer<SampleOut, Stream>` | 約 30 + BUFFER（預設 512） | 取樣的環形緩衝（static） |
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

### 會不會等（loop() 會不會卡住）

大部分的類別都是「開始一件事，然後每次 `loop()` 呼叫 `update()`」：函式馬上回來，時間到了 `update()` 才做下一步，所以同一個 `loop()` 可以同時顧好幾樣東西（車在走、LED 在呼吸、蜂鳴器在唱、按鈕在讀）。`update()` 也可以交給 `Uno::UnoBackground` 在背景跑，`loop()` 就算 `delay()` 也不影響。

| 不會等（馬上回來，要一直呼叫 `update()`） | 會等（做完才回來） |
|---|---|
| `L298N`：`DCMotor`、`Stepper`、`TwoWheelCar` 的每個動作（`car.forward(SPEED)`、`car.forward(SPEED, ms)` 都只是排進佇列） | `TwoWheelCar::wait()`：等到排的動作做完 |
| `audio::Player`、`SamplePlayer`：`play()` | `audio::StreamPlayer`、`StreamSamplePlayer` 的 `open()`：等電腦回答（最多 3 s）；之後 `update()` 每次最多等約 1 ms |
| `video::FlashPlayer`、`StreamPlayer`：`start()` / `open()` | `image` 的 `draw()`：整張畫完（`StreamImage` 還要等電腦傳完） |
| `switches`、`Clicks`、`LED::Effect`、`TCRT5000`、`analog` 的濾波 | `graphics` 的每個畫圖函式：畫完才回來（見上面的時間）；`SSD1306` 的 `display()`：送完才回來（整張約 64 ms） |
| `ultrasonic::Ranger::update()` | `Ranger::measure()`：最多約 40 ms |
| `DS18B20`：`startConversion()` → `isConversionDone()` → `readRaw()` | `DS18B20::readCentiCelsius()`：最多 `conversionMs()` + 50 ms（12 位元約 800 ms） |
| `NEC::Receiver`：中斷收進佇列，程式忙也不會漏 | `NEC::Transmitter::send()`：一次按鍵 67.5 ms |
| — | `Uno::UnoAnalogPin::readAnalog()`：約 112 µs；`delayMs`、`delayUs` |

會等的函式不要在需要準時的東西旁邊用：例如車在 `loop()` 裡靠 `update()` 走，`readCentiCelsius()` 一卡 800 ms，這段時間車的動作就不會換。改用不會等的寫法，或把 `update()` 放進 `Uno::UnoBackground`。

## 參數在哪個階段決定

font、image、video、audio 的資料都由 `source/convert.py` 事先轉好。參數分成四個階段，越前面的越省板子的 Flash、RAM 和時間，但改了就要重新轉換或重新燒錄：

| 階段 | 怎麼改 | 改了要做什麼 |
|---|---|---|
| ① 轉換時（`convert.py`，或 `FONT(...)` 等巨集：編譯前自動轉） | 命令列參數 / 巨集的選項字串，寫進 `.h` 的資料本身 | 巨集：改字串再燒錄（自動重轉）；手動：重新轉換、重新燒錄 |
| ② 編譯時（template 參數、用哪個型態） | 程式裡的型別，例如 `StreamImage<Screen, StreamLink>`、螢幕的 `ColorFormat` | 重新燒錄 |
| ③ 開始時（`start`、`open`、`play` 的參數） | 每次開始一個新的都能不同，但播放中不能改 | — |
| ④ 隨時 | 一般的函式，播放中也能呼叫 | — |

### font

| 階段 | 參數 |
|---|---|
| ① 轉換時 | 字型檔、大小（`--height` / `--width` / `--size`）、字元範圍（`--first`、`--last`）、找不到的字用哪個（`--default`） |
| ② 編譯時 | 用哪個字型型態（全部都列在 `selected.h`、`registry.h`；沒用到的不佔 Flash） |
| ④ 隨時 | `setFont<Font>()`（在編進去的字型之間換）、每次畫的位置和顏色、`setDecimals`、`setSeparator`、`setLineSpacing`、`setAutoLineBreak` |
| 不能改 | 字的大小、粗細：要另一個大小就再轉一個字型 |

### image

| 階段 | 參數 |
|---|---|
| ① 轉換時 | `--target`（Flash 或串流）、尺寸（`--width` / `--height` / `--resize` / `--fit` / `--fill`）、`--scale`、`--resample`、顏色（`--palette`、`--colors`、`--dither`、`--threshold`、`--invert`、`--background`）、`--encoding`、`--max-bytes` |
| ② 編譯時 | `SEGMENT`（一次畫幾個像素，RAM）、Stream 的 `CHUNK`（`StreamImage` 一次收幾個 byte）、Screen（含顏色格式）和 Stream 的型別、用哪張圖片（型態） |
| ③ 開始時 | `draw<Picture>(x, y)`：哪張圖、畫在哪（超出螢幕的部分裁掉） |
| 不能改 | 放大倍數、顏色、反白：都在資料裡 |

### video

| 階段 | 參數 |
|---|---|
| ① 轉換時 | `--target`、尺寸和 `--scale`、`--resample`、顏色（`--palette`、`--colors`、`--dither`、`--mono`、`--threshold`、`--invert`、`--background`）、剪輯（`--start`、`--duration`）、幀率（`--max-fps`、`--min-fps`）、大小（`--max-bytes`、`--reduce`）、`--keyframe`、`--tile` |
| ② 編譯時 | Stream 的 `CHUNK`（`StreamPlayer`）、Screen（含顏色格式）和 Stream 的型別、用哪個動畫（型態） |
| ③ 開始時 | `FlashPlayer::start<Animation>(x, y)`；`StreamPlayer::open<Animation>(x, y, play, loop, speed)` |
| ④ 隨時 | `StreamPlayer`：`play`、`pause`、`stop`、`seek`、`setSpeed`、`setLoop`、`close`。`FlashPlayer` 只有 `update()`：要換動畫或位置就重新 `start` |
| 不能改 | 幀率、放大倍數、顏色；`FlashPlayer` 的速度和循環（它固定照原本的時間、一直循環） |

### audio

| 階段 | 參數 |
|---|---|
| ① 轉換時 | `--target`、`--sound`（無源的音符 / 有源的節奏 / 取樣）、`--quality`；音符：`--quantize`、`--transpose`、`--range`、`--min-note`、`--min-rest`、`--speed`；節奏：`--hz`；取樣：`--rate`、`--encoding`、`--gain`；偵測：`--silence`、`--voicing`、`--onset`；剪輯：`--start`、`--duration` |
| ② 編譯時 | 哪隻腳、哪種蜂鳴器（`UnoTone<PIN, IDLE_HIGH>`、`PassiveBuzzer2Pins` / `3Pins`、`UnoPcmOut<PIN>`）、`GAP_MS`（音和音之間的空隙）、`CHUNK`、`BUFFER`（串流）、用哪個聲音（型態） |
| ③ 開始時 | `play<Melody>(repeat)`、`open<Melody>(repeat)`：哪首、要不要重複 |
| ④ 隨時 | `setVolume`（全部的播放器）、`stop`；`Player` 的 `tone`、`beep`、`beeps`、`slide` 的頻率和長度（每次呼叫都能不同） |
| 不能改 | 轉好的聲音的速度、移調、音色：在資料裡（`--speed`、`--transpose`）；要即時改就自己用 `tone()` 一個一個播，例如 `src/week_5_NEC/assignments/4_piano.cpp` |

**為什麼大部分在轉換時**：UNO 只有 2 KB RAM、16 MHz。縮放、換顏色、重新取樣在電腦上算一次就好，在板子上每一幀、每個取樣都算會太慢。所以會用到很多組參數時，就轉成好幾個檔案（巨集寫好幾次、選項不同），沒用到的不會編進去。

## 共同的寫法

- **編譯期檢查**：腳位、鮑率、動畫目標用 `static_assert` 報錯；不建議的用法用 deprecated 成員印警告（`Uno::warn::*`、`image::warn::*`、`video::warn::*`、`audio::warn::*`）；訊息的寫法見[提示和錯誤](#提示和錯誤)。
- **命名**：
  - 資料夾、檔案、namespace 都小寫；縮寫和專有名詞維持原樣（ST7789、SSD1306、DS18B20、NEC、HX1838、I2C、LED、RGB565、M130T、Uno）。
  - 多個字的名稱用底線（`flash_player.h`、`Uno_serial.h`、`remote_17_key.h`）。
  - class、struct、template 型別參數用大駝峰。
- **類別**：先宣告，`// ================= definitions =================` 之後再定義；函式都標 `inline`，不寫成一行。
- **產生的資料**（font、image、video、audio）：
  - `source/convert.py` 產生 `output/*.h`，一次轉換一個檔案，全部在同一層（`--target stream` 的資料在同一個檔案的 `#if 0` 裡，板子不編譯）；程式裡 `FONT(...)` 等巨集的檔案由 `tools/assets.py` 在編譯前用同一個 `convert.py` 轉，放在同一個地方。
  - `output/selected.h`（全部）和 `output/registry.h`（巨集對應的型態）都是 `tools/assets.py` 每次編譯前寫的。
  - `source/check.py` 檢查 `output/` 的檔案格式是目前的、原始檔沒變；`convert.py` 每次轉完會自動跑。
  - 資料放在 inline 函式裡的 `static PROGMEM` 陣列：程式沒用到的會被編譯器丟掉，不佔 Flash。
- **Python**：直接用 `python` 執行；會 import 專案裡其他檔案的腳本，第一行寫 `sys.dont_write_bytecode = True`（在任何 import 之前），不會產生 `__pycache__`。
- **程式（`src/`）的寫法**：開頭的 settings、parts、段落、命名，見 [src/README.md](../src/README.md#程式的寫法)。

## 編譯

- **C++17**：`platformio.ini` 把預設的 `-std=gnu++11` 換成 `-std=gnu++17`。
- **`-mcall-prologues`**：函式開頭、結尾保存暫存器改用共用的程式，省約 1.5 KB，每次呼叫多幾個週期。
- **`inline`**：有沒有寫對大小沒有影響（實測完全一樣）；開了 LTO 由編譯器自己決定要不要展開。
- **`lib_deps`**：`platformio.ini` 直接列出 `lib/` 的每個資料夾（新增函式庫時也要加進去），所以程式放在 `src/` 的任何子資料夾都能編譯（PlatformIO 找依賴時會跳過某些子資料夾）。
