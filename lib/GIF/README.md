# GIF

在 ST7789 螢幕上播放 GIF 動畫，`namespace GIF`（播放器）和 `namespace animation`（產生的動畫）。資料格式和串流協定寫在 [format.md](format.md)。

## 資料夾

```
lib/GIF/
├── GIF.h              入口：播放器 + output/selected.h
├── decoder.h          GIF::Decoder<Screen>：幀格式，畫一幀
├── flash_player.h     GIF::FlashPlayer<Screen>：播放 Flash 裡的幀（--target uno）
├── stream_player.h    GIF::StreamPlayer<Screen, Link, CHUNK>：播放 Mac 傳來的幀（--target mac）
├── assets/            原始 .gif
├── output/
│   ├── selected.h     程式裡能用的動畫（convert.py 自動加，可以手動刪）
│   ├── *.h            給板子的檔案：uno 有幀資料，mac 只有尺寸和路徑
│   └── mac/*.h        mac 的幀資料，只有 stream.py 會讀，永遠不會被 include
└── source/
    ├── config.json    兩個目標的參數範圍和預設值
    ├── convert.py     .gif → output/*.h
    ├── check.py       檢查 selected.h
    └── stream.py      不用參數：自動找板子，板子要哪個檔案就送哪個（需要 pip install pyserial）
```

## 三種目標

| | `--target uno` | `--target mac` | `--target uno mac` |
|---|---|---|---|
| 幀資料在哪 | `output/x.h`，燒進 Flash | `output/mac/x.h`，留在 Mac | `output/x.h` |
| `output/x.h` 裡有什麼 | 幀資料 + 路徑 | 只有尺寸、時間、路徑（很小） | 幀資料 + 路徑 |
| 能用的播放器 | FlashPlayer（StreamPlayer 也行，會警告） | StreamPlayer | 兩個都可以 |
| 顏色（`--palette`） | 16（預設）、256、565（警告） | 16、565（預設） | 16（預設）、565（警告） |
| 範圍 | uno 的 | mac 的 | 兩者都允許的（交集） |

- `output/x.h` 三種都會加進 `selected.h`。
- mac 的描述檔只有幾十 bytes，好幾 MB 的幀資料在 `output/mac/`，不會被 include，也不會拖慢編譯。
- 沒被程式用到的動畫，就算在 `selected.h` 裡也不佔 Flash，編譯器會丟掉。

**用錯會被抓到**：
- FlashPlayer 拿到只有 mac 的檔案：**編譯錯誤**（檔案裡沒有幀資料，播不了）。
- StreamPlayer 拿到只有 uno 的檔案：**編譯警告**（可以播，但沒有用 mac 的範圍檢查過）。
- StreamPlayer 拿到 256 色的檔案：**編譯錯誤**（板子沒有 512 B RAM 放調色盤）。

## FlashPlayer

```bash
cd lib/GIF
python source/convert.py assets/orbit.gif output --target uno --width 120 --scale 2 --palette 16 --colors 8 --keyframe 8
```

```cpp
#include "GIF.h"
GIF::FlashPlayer<Screen> player(screen);
if (player.start<animation::orbit_uno_120x120_palette_16>(60, 40)) {   // 放不下就回傳 false
    while (true) {
        player.update();   // 時間到了才畫下一幀
    }
}
```

- `frame()` 是目前的幀，`skippedFrames()` 是為了追時間跳過的幀數。
- `--keyframe N`：每 N 幀放一個 I 幀。間隔越短，落後時越快追上，但資料越大。

## StreamPlayer

**Mac**：只要執行一次，不用參數。它會自動找板子，拔掉、重插、板子重置都會自己接回。
```bash
pip install pyserial
cd lib/GIF
python source/convert.py assets/orbit.gif output --target mac --fit 120x120 --palette 565
python source/stream.py
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
| `drawnCount()`、`error()` | 畫了幾幀；錯誤代碼：找不到檔案、讀不懂、256 色、放不下螢幕、沒有回應 |

- 控制指令會立刻送出，等 Mac 回答才返回（最多 1 秒），回傳 true 表示 Mac 收到了。
- **先關監看視窗**：PlatformIO 的序列監看視窗也會佔用串口，執行 `stream.py` 前要先關掉。
- **啟動順序**：打開串口會讓板子重置，`stream.py` 先開或後開都可以，板子會一直重試。

## 尺寸參數

所有尺寸都是指「螢幕上」的大小，也就是放大之後：

| 參數 | 意思 |
|---|---|
| （都不給） | GIF 原本的大小 |
| `--width W` 或 `--height H` | 只給一個：維持比例 |
| `--width W --height H` | 兩個都給：剛好這個大小（會拉伸） |
| `--resize F` / `--resize FXxFY` | GIF 原尺寸的倍數，可以是小數，x 和 y 可以不同，例如 `0.75` 或 `1.5x1` |
| `--fit WxH` | 維持比例，塞進 W×H 的框裡，盡量大 |
| `--fill WxH` | 維持比例，蓋滿 W×H，超出的部分從中間裁掉 |
| `--scale N` / `--scale NXxNY` | 板子把每個點重複畫 NX×NY 次，只能是整數。資料只存「螢幕大小 ÷ scale」 |
| `--resample` | 縮放方法：`nearest`（像素風用）、`bilinear`、`bicubic`、`lanczos`（預設） |

- 非整數倍的放大用 `--resize` 或 `--width`，在轉換時重新取樣；`--scale` 只負責整數倍、省資料。
- 尺寸不是 scale 的整數倍時會四捨五入，並印警告說實際大小。
- 硬限制：螢幕上最大 240×240；有放大時，存的寬度最多 120（板子只有一行的緩衝）。

## 顏色參數

| 參數 | 意思 |
|---|---|
| `--palette 16` | 共用調色盤，最多 16 色，每個 run 1 byte。適合扁平色塊、像素風 |
| `--palette 256` | 共用調色盤，最多 256 色，每個 run 2 bytes。只有 uno：調色盤直接從 Flash 讀 |
| `--palette 565` | 每個像素自己的 RGB565 顏色，每個 run 3 bytes。適合照片、漸層 |
| `--colors N` | 調色盤只用 N 色（16 / 256 模式） |
| `--dither` | 抖色：漸層比較平滑，但 run 變短、資料變大 |
| `--background` | 透明的地方填什麼顏色，預設 000000 |

## 幀率和大小參數

| 參數 | 意思 |
|---|---|
| `--max-fps F` | 間隔小於 1/F 的幀丟掉，時間加給前一幀，整段動畫總長不變 |
| `--min-fps F` | `--max-bytes` 降幀率時的下限；估計板子達不到這個 fps 時也會警告 |
| `--max-bytes N` | 資料超過 N 就自動降，直到放得下為止；降不下來就報錯 |
| `--reduce fps` / `size` | `--max-bytes` 先降幀率還是先降尺寸（預設 size），一邊到下限就換另一邊 |
| `--keyframe N` | uno：每 N 幀一個 I 幀（只有 mac 時忽略：Mac 直接送差異） |
| `--tile N` | 找變化區域用的小方塊大小，預設 8 |
| `--name` | 自訂檔名和 struct 名稱 |

## 檔名和檔案內容

- **預設檔名**：`{gif}_{target}_{寬}x{高}_palette_{16|256|565}.h`，target 是 `uno`、`mac` 或 `uno_mac`，struct 同名。
- **檔案開頭**：不管檔名寫了多少，每個設定都會寫在檔案開頭的 `// settings:` 區塊，包括原本的指令、裁切範圍、實際尺寸、顏色、幀率、資料量、估計可達到的 fps。
- **轉換時會檢查的事**：
  - 超過硬限制（格式或板子做不到的）：報錯，不產生檔案。
  - 超出 `config.json` 的建議範圍：警告。
  - 估計板子達不到 GIF 的 fps、或低於 `--min-fps`：警告。

## 其他限制

- M130T 沒有 TE 腳，大面積變化時可能看到畫面撕裂。
- GIF 裡小於 20 ms 的延遲會當成 100 ms，跟瀏覽器的做法一樣。
- 畫圖時間是用 UNO 的週期數估算的（`config.json` 的 `board`），實際會有些差距。
