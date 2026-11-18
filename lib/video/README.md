# video

| 需要 | 得到 |
|---|---|
| 一個 `Screen`：`setWindow(…)`、`writePixel(color)`… 和它的顏色格式 `ColorFormat`（`ST7789::M130T<Bus, color::RGB565>`、`SSD1306::OLED128x64<Bus, color::BW<>>`） | `video::FlashPlayer`：播燒進 Flash 的動畫 |
| 串流：再加一個 `Stream`（`stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`） | `video::StreamPlayer`：幀留在電腦，播的時候才送過來（播放、暫停、跳轉、速度、循環） |
| 動態圖或影片（gif、webp、mp4、mov…） | `animation::*`：`ANIMATION("檔案")` 或 `source/convert.py` 轉出來的動畫 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

在螢幕上播放動畫和影片，`namespace video`（播放器）和 `namespace animation`（產生的動畫）。來源可以是動態圖（.gif、.webp、.apng）或影片（.mp4、.mov、.webm…），由 `source/convert.py` 轉成同一種幀格式。

兩種來源：

- **預先編譯**：幀燒進 Flash（`FlashPlayer`）。
- **跟電腦拿**：幀留在電腦，經過 USB 串口送過來（`StreamPlayer`）。

## 用法

### FlashPlayer

```bash
cd lib/video
python source/convert.py assets/orbit.gif output --target flash --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8
python source/convert.py clip.mp4 output --target flash --fit 64x64 --mono --start 3 --duration 2 --max-fps 10
```

```cpp
#include "color.h"
#include "video.h"
video::FlashPlayer<Screen> player(screen);           // 顏色跟著 Screen::ColorFormat，見 lib/color
if (player.start<animation::orbit_flash_120x120_palette_16>(60, 40)) {   // 放不下螢幕就回傳 false
    while (true) {
        player.update();   // 時間到了才畫下一幀
    }
}
```

### StreamPlayer

**電腦**：執行一次就好，不用參數。它會自動找板子，拔掉、重插、板子重置都會自己接回；圖片、動畫、聲音都是同一個程式（[lib/stream](../stream/README.md)）。

```bash
cd lib/video
python source/convert.py assets/orbit.gif output --target stream --fit 120x120 --palette 565
cd ../..
python lib/stream/source/stream.py
```

**UNO**：

```cpp
#include "Uno_serial.h"
#include "stream.h"
constexpr uint32_t BAUD = 1000000;                                  // 跟 stream.py 的 --baud 一樣
constexpr uint8_t STREAM_CHUNK = 64;                                // 一次接收的 byte 數（RAM），16 ～ 255
using Link = Uno::UnoSerial<BAUD>;
using StreamLink = stream::Puller<Link, STREAM_CHUNK>;
video::StreamPlayer<Screen, StreamLink> player(screen);
player.open<animation::orbit_stream_120x120_palette_565>(60, 40);     // (x, y, play = true, loop = true, speed = 100)
while (true) {
    player.update();
}
```

### 在 OLED 上播

`SSD1306::Frame`（[SSD1306](../SSD1306/README.md)）也有 `setWindow`、`writePixel`、`endWrite`，所以 `FlashPlayer` 可以直接用：

```bash
python source/convert.py assets/orbit.gif output --target flash --fit 64x64 --mono --keyframe 8
```

```cpp
SSD1306::OLED128x64<Bus, color::BW<>> screen;
video::FlashPlayer<SSD1306::OLED128x64<Bus, color::BW<>>> player(screen);   // 亮度 ≥ 128 的點亮起來
player.start<animation::orbit_flash_64x64_palette_16_mono>(32, 0);
while (true) {
    if (player.update()) {
        screen.display();    // 畫進 RAM 的畫面，再送出改過的部分
    }
}
```

- `--mono`：轉換時就變成黑白（0x0000 / 0xFFFF），`color::BW<>` 不用再判斷；彩色的動畫也能播，由 `color::BW<門檻>` 依亮度決定每一點亮不亮。
- 實測 16.5 ～ 17 fps（動畫本身 16.7 fps），沒有跳幀。

完整的例子：`src/week_3_ST7789_DS18B20_graphics_color_font_image_video/tests/1_screen.cpp` 的 `tests[18]`、`tests[19]`（ST7789）、`src/week_4_SSD1306_switch_buzzer_audio/assignments/2_animation.cpp`（OLED）。

## 直接用檔案（推薦）

程式裡寫 `ANIMATION("路徑", "選項")`，編譯前自動轉換，不用先跑 `convert.py`，檔案也不用放在 `assets/`：

```cpp
#include "video.h"
using Orbit = ANIMATION("clips/orbit.gif", "--width 64 --mono");
player.start<Orbit>(32, 0);
```

| 項目 | 說明 |
|---|---|
| 路徑 | 絕對路徑、`~/…`、相對於寫它的 .cpp；找不到時再試專案資料夾、`lib/video/assets/` |
| 選項 | 一個字串，就是 `source/convert.py` 的選項：`--target`、`--width` / `--fit`…、`--palette`、`--mono`、`--max-fps`…；省略 `--target` 時是 `flash`（跟 `convert.py` 一樣） |
| 轉到哪 | `lib/video/output/`，跟手動跑 `convert.py` 的一樣（`tools/assets.py` 每次編譯前找：同一個檔案、同樣的選項已經轉過就直接用，沒有才轉） |
| 型態 | `ANIMATION(...)` 就是那個 `animation::` struct，跟手動轉換的一樣用 |
| 限制 | 兩個參數都要是字串常數（不能是變數） |

完整的說明：[lib/README.md](../README.md#字型圖片影片聲音直接用檔案)；例子：`src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp`（四種都有）。上面「用法」是手動轉換的方式，一樣能用。

## 三種目標

| | `--target flash` | `--target stream` | `--target flash stream` |
|---|---|---|---|
| 幀資料在哪 | `output/x.h`，燒進 Flash | `output/x.h` 的 `#if 0` 裡，留在電腦 | `output/x.h` |
| 能用的播放器 | `FlashPlayer`（`StreamPlayer` 也行，會警告） | `StreamPlayer` | 兩個都可以 |
| 適合 | 小、色少、短、不需要電腦 | 大、全彩、長 | 兩種都想用 |

| 用錯時 | 結果 | 原因 |
|---|---|---|
| `FlashPlayer` 拿到只有 stream 的檔案 | 編譯錯誤 | 檔案裡沒有幀資料 |
| `StreamPlayer` 拿到只有 flash 的檔案 | 編譯警告 | 能播，但沒用 stream 的範圍檢查過 |
| `StreamPlayer` 拿到 256 色的檔案 | 編譯警告 | 調色盤跟著標頭先傳，放在執行時配置的 RAM，直到 `close()` 或下一次開啟才釋放（原理見 [image 的說明](../image/README.md#串流-256-色調色盤先傳放在執行時配置的-ram)） |

## 時間控制

| | FlashPlayer | StreamPlayer |
|---|---|---|
| 誰計時 | 板子（`nowMs`） | 電腦；板子只照電腦給的位置畫 |
| 第一幀 | 第一次 `update()` 時畫第 0 幀（I 幀） | 連上後先送整張畫面 |
| 之後 | 每一幀在「前面所有幀的延遲加總」時到期 | 電腦送「現在該顯示的幀」跟螢幕上那幀的差異 |
| 落後超過一幀 | 跳到最後一個已經到期的 I 幀（跳過的算進 `skippedFrames()`）；中間沒有 I 幀就照順序畫，並從現在重新計時（變慢，但不會畫錯） | 直接跳過，不需要 I 幀 |

- `--keyframe N`：每 N 幀放一個 I 幀。間隔越短，落後時越快追上，但資料越大。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `start` / `open` 的 x、y | 整個動畫（放大後）在螢幕上：x ≥ 0、x + 寬 ≤ 螢幕寬，y 也一樣 | 不畫：`start` 回傳 false、`error()` = `ERROR_TOO_BIG`（直接串流進一個窗口，不經過裁切） |
| `Animation` | `output/selected.h` 裡的 struct | 編譯錯誤 |
| 放大後的一列 | 有放大時，存的寬度 ≤ 120（板子的一列緩衝） | 不畫（`convert.py` 已經保證） |
| `seek(ms)` | 0 ～ 總長；超過時循環取餘數，不循環停在結尾 | — |
| `setSpeed(percent)` | 1 ～ 1000 | 夾到範圍內（電腦端） |
| `Stream` 的 `CHUNK` | 16 ～ 255 | 編譯錯誤 |
| 時間精度 | 1 ms（`millis`） | — |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 沒有一直呼叫 `update()` | 不用中斷，`update()` 被呼叫時才會畫：`FlashPlayer` 跳幀追時間，`StreamPlayer` 讓電腦跳幀 | 中間不要做太久的事 |
| 在動畫範圍內畫別的東西 | P 幀只畫有變化的地方：`FlashPlayer` 被蓋掉的地方要等到下一個 I 幀；`StreamPlayer` 會一直錯，直到那裡的像素又變化 | 不要蓋到動畫；`StreamPlayer` 要重畫就重新 `open` |
| 動畫一部分在螢幕外 | 不畫（不裁切） | 改位置，或轉換時改尺寸 |
| 同時用 Arduino 的 `Serial` | `UnoSerial` 也用 D0 / D1 | 不能同時用；也要先關掉序列監看視窗 |
| `stream.py` 開著時燒錄 | 燒錄時它自動放開串口，可以一直開著 | 先開 `stream.py`，再燒錄（打開串口會讓板子重置，見 [lib/stream](../stream/README.md#streampy)） |
| ST7789 上大面積變化 | M130T 沒有 TE 腳，可能看到畫面撕裂 | — |
