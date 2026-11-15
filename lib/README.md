# lib

全部都是只有標頭檔的函式庫。每個資料夾都有自己的 README。

| 資料夾 | namespace | 內容 | 入口 |
|---|---|---|---|
| [Uno](Uno/README.md) | `Uno` | ATmega328P 暫存器：腳位、SPI、UART、PWM、Flash、時間 | `Uno_ST7789.h`、`Uno_serial.h` |
| [ST7789](ST7789/README.md) | `ST7789` | ST7789 指令、畫窗口、模組初始化 | `module/M130T.h`（或 `ST7789.h`） |
| [graphics](graphics/README.md) | `graphics` | 點、線、矩形、三角形、圓、文字 | `graphics.h` |
| [color](color/README.md) | `color` | RGB565 顏色常數和轉換 | `color.h` |
| [font](font/README.md) | `font` | 從 .ttf 產生的字型 | `font.h` |
| [GIF](GIF/README.md) | `GIF`、`animation` | GIF 動畫：從 Flash 播，或從 Mac 串流 | `GIF.h` |
| [temp_sensor](temp_sensor/README.md) | `temp_sensor` | DS18B20 溫度感測器（1-Wire） | `temp_sensor.h` |
| [I2C](I2C/README.md) | `SSD1306` | I2C OLED 螢幕（SSD1306）：指令、板子 RAM 裡的畫面 | `module/OLED_128x64.h`（或 `I2C.h`） |
| [image](image/README.md) | `image`、`picture` | 靜態圖片（jpg / png / webp / heic）：從 Flash 畫，或從 Mac 串流；2 / 16 / 256 / 565 色 | `image.h` |
| [switch](switch/README.md) | `switches` | 開關（GND、VCC、OUT）：去彈跳、按下 / 放開、按了多久 | `switch.h` |

## 依賴關係

```
Uno_ST7789.h ──(Bus)──▶ ST7789::Datasheet ─▶ Window ─▶ M130T
                                                         │
                         graphics::Shapes ◀──────────────┘
                                │
          font ──────▶ graphics::Text          = graphics::Canvas<M130T<Bus>>
                                │
Uno_serial.h ──(Link)──▶ GIF::StreamPlayer     GIF::FlashPlayer
                                └──── 都畫在 Canvas 上 ────┘

Uno_one_wire.h ──(Wire)──▶ temp_sensor::DS18B20   （跟螢幕無關，可以單獨用）

Uno_I2C.h ──(Bus)──▶ SSD1306::Datasheet ─▶ Frame ─▶ OLED128x64
                                                      │
                     graphics::Canvas<OLED128x64<Bus>> ◀┘   （跟 ST7789 用同一套圖形和文字）

image::FlashImage、image::StreamImage ──(writeWindow)──▶ 任何 Screen：M130T 的 Canvas 或 OLED 的 Canvas
Uno_serial.h ──(Link)──▶ image::StreamImage
GIF::FlashPlayer ──(setWindow / writeData16)──▶ ST7789 的 Window，或 OLED 的 Frame

Uno_pin.h ──(Pin)──▶ switches::Switch          （跟螢幕無關，可以單獨用）
```

- **換主板**：只換 `Uno/`，寫一個有同樣 static 函式的 Bus（清單在 `ST7789/basic/datasheet.h` 開頭）。
- **換螢幕模組**：在 `ST7789/module/` 加一個檔案（見 [ST7789/module/README.md](ST7789/module/README.md)）。
- **換成 OLED**：`graphics` 只需要 `width`、`height`、`fillWindow`（文字另外要 Bus 的讀 Flash），所以 `SSD1306::Frame` 提供這幾個，圖形和文字就能直接畫上去。
- `color` 不依賴任何東西；`graphics` 本身沒有顏色常數。

## 比較

### 不合理的輸入怎麼處理

| 處理方式 | 什麼時候 | 例子 |
|---|---|---|
| **編譯錯誤** | 編譯時就知道一定不行 | 腳位不存在（包括 `UnoOneWire`）、SPI 腳被拿來當 DC / RES / CS / BLK、沒有 CS 卻不是 SPI mode 3、鮑率誤差 > 5%、FlashPlayer 拿到只有 mac 的動畫、StreamPlayer 拿到 256 色動畫 |
| **編譯警告** | 能動，但不建議 | 兩個訊號同一隻腳、用到 D0 / D1、BLK 不是 PWM 腳或用 Timer0、鮑率誤差 > 2%、StreamPlayer 拿到只有 uno 的動畫 |
| **裁切** | 執行時，畫到螢幕外 | 所有圖形、文字、`writeWindow`：只畫螢幕上的部分 |
| **拒絕** | 執行時，整個不能做 | GIF 動畫放不下螢幕：`start` 回傳 false、`error()` = 4；DS18B20 的解析度不是 9 ～ 12：`ERROR_RESOLUTION` |
| **回傳錯誤** | 執行時，硬體沒回應或資料壞了 | DS18B20：`ERROR_NO_SENSOR`、`ERROR_CRC`、`ERROR_TIMEOUT`；StreamPlayer：`error()`；OLED：`init()`、`display()`、`readStatus()` 回傳 false，`ok()`；StreamImage：`draw()` 回傳錯誤代碼 |
| **夾到範圍內 / 遮掉多的位元** | 執行時，指令參數超出 | SSD1306：`setMultiplexRatio`、`setClock`、`setPrecharge` 夾到範圍；其他指令只取 datasheet 定義的位元 |
| **不畫** | 執行時，參數本身沒意義 | 寬或高 ≤ 0、半徑 < 0、邊界 ≤ 0、空字串 |
| **不檢查** | 檢查太貴或不可能檢查 | `setWindow` 超出螢幕、datasheet 的原始指令、`readFlash` 的指標不是 `PROGMEM`、`writeWindow` 的陣列不夠長、`setSeparator` 的字串被釋放、三角形的角相差 ≥ 2048 |
| **轉換時報錯 / 警告** | 電腦上產生資料時 | `font/source/convert.py`、`GIF/source/convert.py`：超過格式上限報錯，超出建議範圍警告 |

### 哪些有裁切

| 東西 | 裁切 | 原因 |
|---|---|---|
| `ST7789::Window::fillWindow`、`writeWindow` | ✅ 在這裡裁 | 最底層，一次就夠 |
| `SSD1306::Frame::fillWindow`、`writeWindow` | ✅ 在這裡裁 | 同上；畫的是板子 RAM 裡的畫面 |
| `image` 的兩個類別 | ✅ | 一列分段用 `writeWindow` 畫，所以跟著裁；GIF 則是直接串流，不裁 |
| `SSD1306::Datasheet` 的指令（`setColumnAddress`…） | ❌，多的位元遮掉 | 照 datasheet 原樣送出 |
| `graphics` 的所有圖形 | ✅ | 都經過 `fillWindow` |
| `graphics::Text` | ✅ | 每個字是一組小矩形，經過 `fillWindow` |
| `ST7789::Window::setWindow` | ❌ | 直接設定控制器的窗口 |
| `ST7789::Datasheet` 的指令（`caset`、`ramWr`…） | ❌ | 照 datasheet 原樣送出 |
| `GIF` 的兩個播放器 | ❌，改成開始時檢查 | 一幀直接串流進一個窗口，逐列裁切會讓解碼變複雜 |

### 浮點數用在哪

UNO 沒有硬體浮點，一次除法約 500 週期，而且會多佔 Flash。只有程式用到下面這些時才會編譯進去：

| 用到浮點的地方 | 用途 | 多佔的 Flash（實測） |
|---|---|---|
| `drawLineAngle` | sin / cos | 約 0.6 KB（程式已經有其他浮點時） |
| `drawTexts` 畫 `float` / `double` | 轉成十進位 | 約 1.3 KB（浮點加減乘除的程式庫 + `drawFloat`） |
| 空心三角形 | 沒有（只有整數） | — |
| 其他圖形、GIF | 沒有 | — |

### RAM 用量（AVR 上實測 sizeof）

| 物件 | bytes | 內容 |
|---|---|---|
| `ST7789::M130T<Bus>` | 8 | 寬、高、偏移量 |
| `graphics::Canvas<M130T<Bus>>` | 27 | 上面 + 字型指標、文字設定 |
| `GIF::FlashPlayer<Screen>` | 309 | 解碼器 293（一列緩衝 240、調色盤 32）+ 時間 |
| `GIF::StreamPlayer<Screen, Link, 64>` | 392 | 解碼器 + 接收緩衝 CHUNK + 狀態；CHUNK = 16 時是 344 |
| `temp_sensor::DS18B20<Wire>` | 1 | 解析度；讀取時另外用 9 bytes 堆疊 |
| `SSD1306::OLED128x64<Bus>` | 1043 | 整張畫面 1024 + 每個 page 改過的範圍 16 + 位址、狀態 3 |
| `graphics::Canvas<OLED128x64<Bus>>` | 1062 | 上面 + 字型指標、文字設定；UNO 的 RAM 剩不到一半 |
| `image::FlashImage<Screen>` | 約 115 | 一段顏色 64（SEGMENT = 32）+ 調色盤 32 + 狀態；建議放在函式裡 |
| `image::StreamImage<Screen, Link>` | 約 180 | 上面 + 接收緩衝 64（CHUNK） |
| `switches::Switch<Pin>` | 約 16 | 狀態、時間、次數 |

播放器建議放在函式裡（堆疊），播完 RAM 就還回去，`src/week_3/test_1.cpp` 就是這樣用的。UNO 只有 2048 bytes RAM。

### 畫圖的時間（估計，16 MHz、SPI 8 MHz）

| 動作 | 約多少週期 | 說明 |
|---|---|---|
| 一個窗口 | 480 | 設定 CASET、RASET、RAMWR |
| 一個像素 | 44 | SPI 送 2 bytes；8 MHz 的硬體極限是 32 |
| 整個螢幕 | 2.5 M（約 160 ms） | 240×240 像素 |
| 三角形 | 外框方塊裡每點約 100 | 逐點檢查，比較慢 |

## 共同的寫法

- **零成本的抽象**：硬體是 template 參數（Bus、Link 都是只有 static 函式的 struct），編譯後跟直接寫暫存器一樣。
- **編譯期檢查**：腳位、鮑率、動畫目標用 `static_assert` 報錯；只是不建議的用法，用 deprecated 成員印警告（`Uno::warn::*`、`GIF::warn::*`）。
- **命名**：
  - 資料夾、檔案、namespace 都小寫；縮寫和專有名詞維持原樣（ST7789、GIF、RGB565、M130T、Uno）。
  - 多個字的檔名用底線（`flash_player.h`、`Uno_serial.h`）。
  - class、struct、template 型別參數用大駝峰。
- **類別**：先宣告，下面 `// ================= definitions =================` 之後再定義；函式都標 `inline`。
- **產生的資料**（font、GIF）：
  - `source/convert.py` 產生 `output/*.h`，並加進 `output/selected.h`。
  - `source/check.py` 檢查 `selected.h` 裡的檔案格式是目前的、原始檔沒變。
  - 資料放在 inline 函式裡的 `static PROGMEM` 陣列，程式沒用到的會被編譯器丟掉，不佔 Flash。
- **Python**：用 `python -B` 執行，不產生 `__pycache__`。

## 編譯

- **C++17**：`platformio.ini` 把預設的 `-std=gnu++11` 換成 `-std=gnu++17`。
- **`-mcall-prologues`**：函式開頭、結尾保存暫存器改用共用的程式，省約 1.5 KB，每次呼叫多幾個週期。
- **`inline`**：有沒有寫對大小沒有影響（實測完全一樣）；template 本來就能放在標頭檔，開了 LTO 由編譯器自己決定要不要展開。
- **`lib_deps`**：`platformio.ini` 直接列出這十個資料夾。測試原本放在 `src/test/` 時，PlatformIO 找依賴會跳過那個資料夾，不列的話找不到標頭檔；直接列出來，放在 `src/` 的任何子資料夾都能編譯。
