# GIF

在 ST7789 螢幕上播放 GIF 動畫，`namespace GIF`（播放器）和 `namespace animation`（產生的動畫）。

| 文件 | 內容 |
|---|---|
| 這份 | 資料夾、三種目標、兩個播放器的用法 |
| [source/README.md](source/README.md) | `convert.py` 的參數和流程、`check.py`、`stream.py` 和串流協定 |
| [output/README.md](output/README.md) | `selected.h`、產生的 struct、幀格式 |

## 資料夾

```
lib/GIF/
├── GIF.h              入口：播放器 + output/selected.h
├── decoder.h          GIF::Decoder<Screen>：幀格式，畫一幀
├── flash_player.h     GIF::FlashPlayer<Screen>：播放 Flash 裡的幀（--target uno）
├── stream_player.h    GIF::StreamPlayer<Screen, Link, CHUNK>：播放 Mac 傳來的幀（--target mac）
├── assets/            原始 .gif
├── output/            產生的動畫
└── source/            電腦上的工具：config.json、convert.py、check.py、stream.py
```

## 三種目標

| | `--target uno` | `--target mac` | `--target uno mac` |
|---|---|---|---|
| 幀資料在哪 | `output/x.h`，燒進 Flash | `output/mac/x.h`，留在 Mac | `output/x.h` |
| 能用的播放器 | FlashPlayer（StreamPlayer 也行，會警告） | StreamPlayer | 兩個都可以 |
| 適合 | 小、色少、不需要電腦 | 大、全彩、長 | 兩種都想用 |

- `output/x.h` 三種都會加進 `selected.h`。mac 的只有幾十 bytes，幀資料在 `output/mac/`，不會被 include，也不會拖慢編譯。
- 沒被程式用到的動畫，就算在 `selected.h` 裡也不佔 Flash，編譯器會丟掉。

**用錯會被抓到**：
- FlashPlayer 拿到只有 mac 的檔案：**編譯錯誤**（檔案裡沒有幀資料，播不了）。
- StreamPlayer 拿到只有 uno 的檔案：**編譯警告**（可以播，但沒有用 mac 的範圍檢查過）。
- StreamPlayer 拿到 256 色的檔案：**編譯錯誤**（板子沒有 512 B RAM 放調色盤）。

## FlashPlayer

```bash
cd lib/GIF
python -B source/convert.py assets/orbit.gif output --target uno --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8
```

```cpp
#include "GIF.h"
GIF::FlashPlayer<Screen> player(screen);
if (player.start<animation::orbit_uno_120x120_palette_16>(60, 40)) {   // 放不下螢幕就回傳 false（動畫不裁切）
    while (true) {
        player.update();   // 時間到了才畫下一幀
    }
}
```

- `frame()` 是目前的幀，`skippedFrames()` 是為了追時間跳過的幀數。
- **時間控制**：第 0 幀在 `update()` 第一次被呼叫時畫出，之後每一幀在「前面所有幀的延遲加總」時到期。
- **落後超過一幀時**：
  - 中間有 I 幀：跳到最後一個已經到期的 I 幀，跳過的幀數累加到 `skippedFrames()`。
  - 中間沒有 I 幀：照順序畫下一幀，並從現在重新計時。動畫會變慢，但不會畫錯。
- `--keyframe N`：每 N 幀放一個 I 幀。間隔越短，落後時越快追上，但資料越大。

## StreamPlayer

**Mac**：只要執行一次，不用參數。它會自動找板子，拔掉、重插、板子重置都會自己接回。
```bash
pip install pillow pyserial
cd lib/GIF
python -B source/convert.py assets/orbit.gif output --target mac --fit 120x120 --palette 565
python -B source/stream.py
```

**UNO**：記得要播哪個檔案。路徑在 `animation::xxx::getPath()` 裡，`open` 時傳給 Mac。
```cpp
using Link = Uno::UnoSerial<1000000>;
GIF::StreamPlayer<Screen, Link, 64> player(screen);   // 64 = 一次接收的 byte 數（RAM）
player.open<animation::orbit_mac_120x120_palette_565>(60, 40);   // (x, y, play = true, loop = true, speed = 100)
while (true) {
    player.update();
}
```

| 函式 | 作用 |
|---|---|
| `open<Animation>(x, y, play, loop, speed)` | 換一個動畫；之後 `update()` 會自己連線，斷線也會自己重連並從原本的位置繼續 |
| `play()`、`pause()`、`stop()` | 播放、暫停；`stop()` 是暫停並回到開頭 |
| `seek(ms)` | 跳到某個時間點 |
| `setSpeed(percent)` | 速度：100 是正常，1～1000 |
| `setLoop(bool)` | 循環；關掉時播到最後一幀就停，`isEnded()` 變成 true |
| `close()` | 不再要幀 |
| `isConnected()`、`isPlaying()`、`isEnded()` | 狀態 |
| `positionMs()`、`frame()`、`durationMs()`、`frameCount()` | 目前位置、目前幀、總長、總幀數 |
| `drawnCount()`、`error()` | 畫了幾幀；錯誤代碼：找不到檔案、讀不懂、256 色、放不下螢幕（或放大後一列超過 120 點）、沒有回應 |

- 控制指令會立刻送出，等 Mac 回答才返回（最多 1 秒），回傳 true 表示 Mac 收到了。
- `UnoSerial` 用 D0/D1，所以不能同時用 Arduino 的 `Serial`，也要先關掉序列監看視窗。
- `stream.py` 可以一直開著：燒錄時它會自動放開串口。但它打開串口時會讓板子重置，所以要先開 `stream.py`，再燒錄（詳見 [source/README.md](source/README.md#streampy)）。

## 在 OLED 上播

`SSD1306::Frame`（[I2C](../I2C/README.md)）也有 `setWindow`、`writeData16`、`endWrite`，所以 FlashPlayer 可以直接用：

```bash
python -B source/convert.py assets/orbit.gif output --target uno --fit 64x64 --mono --keyframe 8
```

```cpp
SSD1306::OLED128x64<Bus> screen;
GIF::FlashPlayer<SSD1306::OLED128x64<Bus>> player(screen);
player.start<animation::orbit_uno_64x64_palette_16_mono>(32, 0);
while (true) {
    if (player.update()) {
        screen.display();    // 畫進 RAM 的畫面，再送出改過的部分
    }
}
```

- `--mono`：OLED 只有亮暗，0 是暗、其他都是亮。
- 實測 16.7 fps，沒有跳幀。

## 兩個播放器比較

| | FlashPlayer | StreamPlayer |
|---|---|---|
| 幀資料 | Flash（燒進 UNO） | Mac，經 USB 串口 |
| 需要電腦 | 不用 | 要開 `stream.py` |
| 顏色 | 16、256、565 | 16、565（256 色的調色盤要 512 B RAM，放不下） |
| 誰計時 | 板子（`millis`） | Mac；板子只照 Mac 給的位置畫 |
| 落後時 | 跳到已經到期的 I 幀，沒有 I 幀就照順序畫、重新計時 | Mac 直接送該顯示的幀和螢幕上那幀的差異，不需要 I 幀 |
| 控制 | 只有 `start`、`update`（沒有暫停、跳轉） | 播放、暫停、停止、跳轉、速度、循環 |
| RAM（AVR 實測） | 309 B | 392 B（CHUNK = 64；CHUNK = 16 時 344 B） |
| `update()` 會等多久 | 只有畫一幀的時間 | 畫一幀 + 收資料；一塊資料最多等 200 ms |
| 控制指令會等多久 | — | 等 Mac 回答，最多 1 秒 |
| 斷線 | — | 3 秒沒回應就重新開啟，從原本的位置繼續；`error()` = 5 |
| 同時用幾個 | 幾個都可以（各自佔 RAM） | 一個（只有一條串口） |

## 使用注意

- **`update()` 要一直呼叫**：兩個播放器都不用中斷，`update()` 被呼叫時才會畫。中間做別的事太久，FlashPlayer 會跳幀追時間，StreamPlayer 會讓 Mac 跳幀。
- **不要在動畫範圍內畫別的東西**：P 幀只畫有變化的地方，假設螢幕上還是上一幀。
  - FlashPlayer：被蓋掉的地方要等到下一個 I 幀才會恢復。
  - StreamPlayer：Mac 不知道螢幕被改了，被蓋掉的地方會一直錯，直到那裡的像素又變化；要重畫就重新 `open`。
- **動畫不裁切**：整個動畫（放大後）要在螢幕上，不然 `start` 回傳 false，或 `error()` = 4，什麼都不畫。
- **開始的第一幀**：FlashPlayer 在第一次 `update()` 時畫第 0 幀（I 幀）；StreamPlayer 連上後先送整張畫面。

## 輸入限制

| 參數 | 型別 | 範圍 | 超出時 |
|---|---|---|---|
| `start` / `open` 的 x、y | `int16_t` | 整個動畫（放大後）在螢幕上：x ≥ 0、x + 寬 ≤ 240，y 也一樣 | 不畫：`start` 回傳 false、`error()` = 4（動畫直接串流進一個窗口，不經過裁切） |
| `Animation` | `animation::*` | `output/selected.h` 裡的 struct | 編譯錯誤 |
| 放大後的一列 | — | 有放大時，存的寬度 ≤ 120（板子的一列緩衝） | `start` 回傳 false、`error()` = 4，不畫（`convert.py` 已經保證） |
| `seek(ms)` | `uint32_t` | 0 ～ 總長；超過時循環取餘數，不循環停在結尾 | — |
| `setSpeed(percent)` | `uint16_t` | 1 ～ 1000 | Mac 端限制在這個範圍 |
| `CHUNK` | `uint8_t` | 1 ～ 255，佔用同樣多的 RAM | — |

## 其他限制

- M130T 沒有 TE 腳，大面積變化時可能看到畫面撕裂。
- 時間的精度是 `millis`（1 ms），延遲是 GIF 裡的值（小於 20 ms 的當成 100 ms）。
