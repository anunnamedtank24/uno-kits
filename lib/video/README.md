# video

在螢幕上播放動畫和影片，`namespace video`（播放器）和 `namespace animation`（產生的動畫）。來源可以是動態圖（.gif、.webp、.apng）或影片（.mp4、.mov、.webm…），由 `source/convert.py` 轉成同一種幀格式。

兩種來源：

- **預先編譯**：幀燒進 Flash（`FlashPlayer`）。
- **跟 Mac 拿**：幀留在電腦，經過 USB 串口送過來（`StreamPlayer`）。

## 檔案

```
lib/video/
├── video.h            入口：播放器 + output/selected.h
├── decoder.h          Decoder<Screen>：幀格式，畫一幀
├── flash_player.h     FlashPlayer<Screen>：播放 Flash 裡的幀（--target uno）
├── stream_player.h    StreamPlayer<Screen, Link, CHUNK>：播放 Mac 傳來的幀（--target mac）
├── assets/            原始動畫（orbit.gif）
├── source/            config.json、convert.py、check.py、stream.py，見 source/README.md
└── output/            產生的動畫和 selected.h，見 output/README.md
```

## 用法

### FlashPlayer

```bash
cd lib/video
python -B source/convert.py assets/orbit.gif output --target uno --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8
python -B source/convert.py clip.mp4 output --target uno --fit 64x64 --mono --start 3 --duration 2 --max-fps 10
```

```cpp
#include "video.h"
video::FlashPlayer<Screen> player(screen);
if (player.start<animation::orbit_uno_120x120_palette_16>(60, 40)) {   // 放不下螢幕就回傳 false
    while (true) {
        player.update();   // 時間到了才畫下一幀
    }
}
```

### StreamPlayer

**Mac**：執行一次就好，不用參數。它會自動找板子，拔掉、重插、板子重置都會自己接回。

```bash
cd lib/video
python -B source/convert.py assets/orbit.gif output --target mac --fit 120x120 --palette 565
python -B source/stream.py
```

**UNO**：

```cpp
using Link = Uno::UnoSerial<1000000>;
video::StreamPlayer<Screen, Link, 64> player(screen);              // 64 = 一次接收的 byte 數（RAM）
player.open<animation::orbit_mac_120x120_palette_565>(60, 40);     // (x, y, play = true, loop = true, speed = 100)
while (true) {
    player.update();
}
```

### 在 OLED 上播

`SSD1306::Frame`（[SSD1306](../SSD1306/README.md)）也有 `setWindow`、`writeData16`、`endWrite`，所以 `FlashPlayer` 可以直接用：

```bash
python -B source/convert.py assets/orbit.gif output --target uno --fit 64x64 --mono --keyframe 8
```

```cpp
SSD1306::OLED128x64<Bus> screen;
video::FlashPlayer<SSD1306::OLED128x64<Bus>> player(screen);
player.start<animation::orbit_uno_64x64_palette_16_mono>(32, 0);
while (true) {
    if (player.update()) {
        screen.display();    // 畫進 RAM 的畫面，再送出改過的部分
    }
}
```

- `--mono`：OLED 只有亮暗，0 是暗、其他都是亮。
- 實測 16.5 ～ 17 fps（動畫本身 16.7 fps），沒有跳幀。

完整的例子：`src/week_3/test_1.cpp` 的 `tests[18]`、`tests[19]`（ST7789）、`src/week_4/assignment_2.cpp`（OLED）。

## 介面需求

| 參數 | 需要的函式 | 現成的實作 |
|---|---|---|
| `Screen` | `width()`、`height()`、`setWindow(x0, y0, x1, y1)`、`writeData16(color)`、`endWrite()`；型別 `BusType`，它有 `readFlash8/16`、`nowMs` | `ST7789::M130T`、`SSD1306::OLED128x64` |
| `Link`（`StreamPlayer`） | `init()`、`available()`、`read8(byte, timeoutMs)`、`write8(byte)`、`clearInput()` | `Uno::UnoSerial` |
| `Animation` | `output/selected.h` 裡的 struct（見 [output/README.md](output/README.md#產生的-struct)） | `animation::*` |

## 函式

### FlashPlayer

| 函式 | 說明 |
|---|---|
| `start<Animation>(x, y)` | 開始播一個動畫，左上角 (x, y)；放不下螢幕時回傳 false |
| `update()` | 時間到了就畫下一幀，回傳有沒有畫（OLED 用它決定要不要 `display()`） |
| `frame()` | 回傳目前的幀 |
| `skippedFrames()` | 回傳為了追時間跳過的幀數 |

### StreamPlayer

| 函式 | 說明 |
|---|---|
| `open<Animation>(x, y, play, loop, speed)` | 換一個動畫；之後 `update()` 自己連線，斷線也會自己重連並從原本的位置繼續 |
| `update()` | 收資料、畫幀 |
| `play()`、`pause()`、`stop()` | 播放、暫停；`stop()` 是暫停並回到開頭 |
| `seek(ms)` | 跳到某個時間點 |
| `setSpeed(percent)` | 設定速度：100 是正常，1 ～ 1000 |
| `setLoop(on)` | 設定循環；關掉時播到最後一幀就停，`isEnded()` 變成 true |
| `close()` | 不再要幀 |
| `isConnected()`、`isPlaying()`、`isEnded()` | 回傳狀態 |
| `positionMs()`、`frame()`、`durationMs()`、`frameCount()` | 回傳目前位置、目前幀、總長、總幀數 |
| `drawnCount()`、`error()` | 回傳畫了幾幀、錯誤代碼 |

- 控制指令（`play`、`seek`…）立刻送出，等 Mac 回答才返回（最多 1 秒），回傳 true 表示 Mac 收到了。

| `error()` | 原因 |
|---|---|
| 1 | `stream.py` 找不到檔案（或在 `lib/video` 外面） |
| 2 | 檔案讀不懂 |
| 3 | 256 色不能串流 |
| 4 | 放不下螢幕（或放大後一列超過 120 點） |
| 5 | 沒有回應 |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `CHUNK`（`StreamPlayer`） | — | 一次接收幾個 byte，1 ～ 255，佔同樣多的 RAM |

## 三種目標

| | `--target uno` | `--target mac` | `--target uno mac` |
|---|---|---|---|
| 幀資料在哪 | `output/x.h`，燒進 Flash | `output/mac/x.h`，留在 Mac | `output/x.h` |
| 能用的播放器 | `FlashPlayer`（`StreamPlayer` 也行，會警告） | `StreamPlayer` | 兩個都可以 |
| 適合 | 小、色少、短、不需要電腦 | 大、全彩、長 | 兩種都想用 |

| 用錯時 | 結果 | 原因 |
|---|---|---|
| `FlashPlayer` 拿到只有 mac 的檔案 | 編譯錯誤 | 檔案裡沒有幀資料 |
| `StreamPlayer` 拿到只有 uno 的檔案 | 編譯警告 | 能播，但沒用 mac 的範圍檢查過 |
| `StreamPlayer` 拿到 256 色的檔案 | 編譯錯誤 | 板子沒有 512 B RAM 放調色盤 |

## 時間控制

| | FlashPlayer | StreamPlayer |
|---|---|---|
| 誰計時 | 板子（`nowMs`） | Mac；板子只照 Mac 給的位置畫 |
| 第一幀 | 第一次 `update()` 時畫第 0 幀（I 幀） | 連上後先送整張畫面 |
| 之後 | 每一幀在「前面所有幀的延遲加總」時到期 | Mac 送「現在該顯示的幀」跟螢幕上那幀的差異 |
| 落後超過一幀 | 跳到最後一個已經到期的 I 幀（跳過的算進 `skippedFrames()`）；中間沒有 I 幀就照順序畫，並從現在重新計時（變慢，但不會畫錯） | 直接跳過，不需要 I 幀 |

- `--keyframe N`：每 N 幀放一個 I 幀。間隔越短，落後時越快追上，但資料越大。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `start` / `open` 的 x、y | 整個動畫（放大後）在螢幕上：x ≥ 0、x + 寬 ≤ 螢幕寬，y 也一樣 | 不畫：`start` 回傳 false、`error()` = 4（直接串流進一個窗口，不經過裁切） |
| `Animation` | `output/selected.h` 裡的 struct | 編譯錯誤 |
| 放大後的一列 | 有放大時，存的寬度 ≤ 120（板子的一列緩衝） | 不畫（`convert.py` 已經保證） |
| `seek(ms)` | 0 ～ 總長；超過時循環取餘數，不循環停在結尾 | — |
| `setSpeed(percent)` | 1 ～ 1000 | 夾到範圍內（Mac 端） |
| `CHUNK` | 1 ～ 255 | — |
| 時間精度 | 1 ms（`millis`） | — |

## 資源

| | FlashPlayer | StreamPlayer |
|---|---|---|
| RAM（AVR 實測） | 309 B | 392 B（CHUNK = 64；CHUNK = 16 時 344 B） |
| `update()` 會等多久 | 畫一幀的時間 | 畫一幀 + 收資料；一塊資料最多等 200 ms |
| 控制指令會等多久 | — | 等 Mac 回答，最多 1 秒 |
| 同時用幾個 | 幾個都可以（各自佔 RAM） | 一個（只有一條串口） |
| 硬體 | Screen 的 | Screen 的 + D0 / D1（`UnoSerial`） |

播放器建議放在函式裡（堆疊），播完 RAM 就還回去。

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 沒有一直呼叫 `update()` | 不用中斷，`update()` 被呼叫時才會畫：`FlashPlayer` 跳幀追時間，`StreamPlayer` 讓 Mac 跳幀 | 中間不要做太久的事 |
| 在動畫範圍內畫別的東西 | P 幀只畫有變化的地方：`FlashPlayer` 被蓋掉的地方要等到下一個 I 幀；`StreamPlayer` 會一直錯，直到那裡的像素又變化 | 不要蓋到動畫；`StreamPlayer` 要重畫就重新 `open` |
| 動畫一部分在螢幕外 | 不畫（不裁切） | 改位置，或轉換時改尺寸 |
| 同時用 Arduino 的 `Serial` | `UnoSerial` 也用 D0 / D1 | 不能同時用；也要先關掉序列監看視窗 |
| `stream.py` 開著時燒錄 | 燒錄時它自動放開串口，可以一直開著 | 先開 `stream.py`，再燒錄（打開串口會讓板子重置，見 [source/README.md](source/README.md#streampy)） |
| 同時開 image 的 `stream.py` | 兩個都要用同一個串口 | 一次只開一個 |
| ST7789 上大面積變化 | M130T 沒有 TE 腳，可能看到畫面撕裂 | — |
