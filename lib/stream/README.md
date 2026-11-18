# stream

| 需要 | 得到 |
|---|---|
| 一個 `Link`：`init()`、`available()`、`read8(byte, timeoutMs)`、`write8()`、`clearInput()`、`readFlash8()`（`Uno::UnoSerial<BAUD>`） | `stream::Puller<Link, CHUNK>`：跟電腦要檔案、一塊一塊拉回來；`image::StreamImage`、`video::StreamPlayer`、`audio::StreamPlayer`、`audio::StreamSamplePlayer` 的 `Stream` 參數 |
| 電腦：Python 3、`pip install pyserial` | `source/stream.py`：一個程式同時送圖片、動畫、聲音，板子要什麼就送什麼 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

把電腦上的檔案送給板子的協定，`namespace stream`。板子這邊只負責「要」和「拉」，檔案的內容（圖片的像素、動畫的幀、聲音的音符）由 image、video、audio 自己讀，所以：

- **image、video、audio 不 include 這個函式庫**：`Stream` 是它們的 template 參數（duck typing），跟 `Screen` 一樣，還是不依賴別的函式庫。
- **電腦只要開一個 `stream.py`**：以前三個函式庫各有一個，一次只能開一個（同一個串口）；現在同一個程式都處理。
- **不會變慢**：`Puller` 全是 template 和 inline，編譯後跟以前寫在各自類別裡的程式一樣。

## 用法

**電腦**：開著就好，不用參數。它會自動找板子，燒錄時也會自動放開串口。

```bash
pip install pyserial
python lib/stream/source/stream.py
```

**UNO**：

```cpp
#include "Uno_serial.h"
#include "stream.h"
#include "image.h"

constexpr uint32_t BAUD = 1000000;          // 跟 stream.py 的 --baud 一樣
constexpr uint8_t STREAM_CHUNK = 64;        // 一次收幾個 byte（RAM）

using Link = Uno::UnoSerial<BAUD>;
using StreamLink = stream::Puller<Link, STREAM_CHUNK>;
image::StreamImage<Screen, StreamLink> pictures(screen);   // 顏色格式跟著 Screen

uint8_t result = pictures.draw<picture::sunset_stream_240x240_palette_565>(0, 0);
```

- **名稱**：不要取名 `Stream`，那是 Arduino 自己的類別（`Arduino.h` 已經有了），所以例子都用 `StreamLink`。
- **CHUNK 可以不同**：同一個程式可以有好幾個 `Puller`，例如音符用 32、取樣用 255（`src/week_5_NEC/tests/2_sound_formats.cpp`）；它們共用同一個串口。

完整的例子：`src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/3_pictures.cpp`（圖片）、`tests/1_screen.cpp` 的 `tests[19]`（動畫）、`src/week_5_NEC/tests/2_sound_formats.cpp`（聲音）。

## stream.py

```bash
python lib/stream/source/stream.py                                # 自己找板子
python lib/stream/source/stream.py --port /dev/cu.usbmodem1101    # 指定串口：Windows 是 COM3，Linux 是 /dev/ttyACM0
python lib/stream/source/stream.py --baud 1000000                 # 跟程式的 BAUD 一樣（預設 1000000）
python lib/stream/source/stream.py --tile 8                       # 動畫：找變化區域的格子大小（預設 8）
```

- **任何系統都能用**（macOS、Windows、Linux）：用 USB 晶片的 ID 找板子（Arduino、CH340、FTDI、CP210x、PL2303）；沒插板子就等，拔掉、重插、板子重置都會自己接回。
- **一個程式處理全部**：啟動時載入每個 `lib/*/source/stream_handler.py`，指令交給對應的那個（`'I'` 圖片、`'A'` 聲音、`'O'` 等是動畫）。新的種類只要加一個 `stream_handler.py`，`stream.py` 不用改。
- **安全**：每個種類只開自己資料夾（`lib/image`、`lib/video`、`lib/audio`）裡的 `.h`。
- **檔案改了會重讀**（看修改時間），不用重開。
- **先關監看視窗**：PlatformIO 的序列監看視窗也會佔用串口。
- **燒錄時自動放開串口**：另一個執行緒每 0.3 秒檢查有沒有 avrdude 或 `pio run -t upload` 在跑；有的話立刻關掉串口，燒錄結束後再接回來，所以 `stream.py` 可以一直開著。
- **打開串口會讓板子重置**：打開串口時一定會動到 DTR，UNO 就會重置，軟體沒辦法避免。
  - 板子正在跑程式時才啟動 `stream.py`，程式會從頭開始。
  - **建議先開 `stream.py`**，再燒錄或按 reset。`stream.py` 一直開著時只有第一次會重置。
  - 硬體上要完全避免，只能在 RESET 和 GND 之間接 10 µF 電容；但之後每次燒錄都要手動按 reset。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `CHUNK` | 1 ～ 255 | 編譯錯誤；各播放器另有下限（`video::StreamPlayer` ≥ 16、`audio::StreamPlayer` ≥ 4、`StreamSamplePlayer` ≤ BUFFER / 2） |
| 路徑 | 最多 255 個字元 | 截斷 |
| `message` 的文字 | 最多 255 個字元，在 RAM | 截斷 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 沒開 `stream.py` | 3 秒後 `ERROR_NO_ANSWER` | 先開 `stream.py` 再燒錄 |
| 開著 `pio device monitor` | `stream.py` 打不開串口 | 串流時關掉監看視窗；要看訊息用 `message()` |
| 同時用 Arduino 的 `Serial` | 也用 D0 / D1 | 不能同時用 |
| 型別取名 `Stream` | 跟 Arduino 的 `Stream` 類別衝突 | 取別的名字，例如 `StreamLink` |
| 同時串流兩種（例如動畫播著又畫串流圖） | 回答會交錯，板子可能讀錯 | 一次只串流一種 |
