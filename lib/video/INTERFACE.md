# video：介面

| 需要 | 得到 |
|---|---|
| 一個 `Screen`：`setWindow(…)`、`writePixel(color)`… 和它的顏色格式 `ColorFormat`（`ST7789::M130T<Bus, color::RGB565>`、`SSD1306::OLED128x64<Bus, color::BW<>>`） | `video::FlashPlayer`：播燒進 Flash 的動畫 |
| 串流：再加一個 `Stream`（`stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`） | `video::StreamPlayer`：幀留在電腦，播的時候才送過來（播放、暫停、跳轉、速度、循環） |
| 動態圖或影片（gif、webp、mp4、mov…） | `animation::*`：`ANIMATION("檔案")` 或 `source/convert.py` 轉出來的動畫 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 螢幕 | `Screen`：`width()`、`height()`、`setWindow()`、`writePixel()`、`endWrite()`、顏色格式 `ColorFormat`、`BusType::readFlash8/16`、`nowMs` |
| 不裁切 | 幀直接串流進一個窗口：整個動畫（放大後）要在螢幕上，否則不畫 |
| 資料 | `animation::*` 由 `convert.py` 產生：顏色一律 RGB565，格式版本要對 |
| 呼叫 | `update()` 每次 loop 都要呼叫；畫一幀時會等 |
| 串流 | `StreamPlayer` 要一個 `Stream`（[lib/stream](../stream/INTERFACE.md)）+ 電腦開著 `stream.py` |
| 執行時的 RAM | 串流 256 色：調色盤收到標頭時 `malloc`，`close()`、下一次開啟、物件消失時 `free`；編譯器的 RAM 數字看不到它；放不下時 `error()` = `ERROR_NO_MEMORY` |
| RAM 跟著顏色 | 放大時的一列緩衝是 120 個 `ColorFormat::Value`：RGB565 240 bytes，RGB888 480 bytes |

## 檔案

```
lib/video/
├── video.h            入口：播放器 + output/selected.h
├── decoder.h          Decoder<Screen>：幀格式，畫一幀；HeapPalette：執行時配置的調色盤
├── flash_player.h     FlashPlayer<Screen>：播放 Flash 裡的幀（--target flash）
├── stream_player.h    StreamPlayer<Screen, Stream>：播放電腦傳來的幀（--target stream）
├── assets/            原始動畫（orbit.gif）
├── source/            config.json、convert.py、check.py、stream_handler.py（lib/stream 的 stream.py 用它讀動畫、計時），見 source/README.md
└── output/            產生的動畫和 selected.h，見 output/README.md
```

## 介面需求

| 參數 | 需要的函式 | 現成的實作 |
|---|---|---|
| `Screen` | `width()`、`height()`、`setWindow(x0, y0, x1, y1)`、`writeData16(color)`、`endWrite()`；型別 `BusType`，它有 `readFlash8/16`、`nowMs` | `ST7789::M130T`、`SSD1306::OLED128x64` |
| `Stream`（`StreamPlayer`） | `CHUNK_SIZE`（≥ 16）、`OK`、`ERROR_*`、`begin()`、`write8/16/32()`、`writePath()`、`available()`、`waitAnswer()`，和 `Answer` 類別 | `stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`（[lib/stream](../stream/README.md)） |
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

- 控制指令（`play`、`seek`…）立刻送出，等電腦回答才返回（最多 1 秒），回傳 true 表示電腦收到了。

| `error()` | 原因 |
|---|---|
| `OK`（0） | — |
| `ERROR_NOT_FOUND`（1） | `stream.py` 找不到檔案（或在 `lib/video` 外面） |
| `ERROR_UNREADABLE`（2） | 檔案讀不懂 |
| `ERROR_NOT_STREAMABLE`（3） | 這個檔案不能串流 |
| `ERROR_NO_ANSWER`（4） | 沒有回應 |
| `ERROR_BROKEN`（5） | 標頭不合理 |
| `ERROR_NO_MEMORY`（6） | 256 色的調色盤放不進 RAM；等一個逾時（3 秒）再重新開啟 |
| `ERROR_TOO_BIG`（7） | 放不下螢幕（或放大後一列超過 120 點） |

前五個跟 [lib/stream](../stream/INTERFACE.md#函式) 的一樣，image、audio 也是同樣的數字。

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Stream` 的 `CHUNK`（`StreamPlayer`） | — | 一次接收幾個 byte，16 ～ 255，佔同樣多的 RAM |

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

## 資源

| | FlashPlayer | StreamPlayer |
|---|---|---|
| RAM（AVR 實測） | 309 B | 392 B（CHUNK = 64；CHUNK = 16 時 344 B） |
| `update()` 會等多久 | 畫一幀的時間 | 畫一幀 + 收資料；一塊資料最多等 200 ms |
| 控制指令會等多久 | — | 等電腦回答，最多 1 秒 |
| 同時用幾個 | 幾個都可以（各自佔 RAM） | 一個（只有一條串口） |
| 硬體 | Screen 的 | Screen 的 + D0 / D1（`UnoSerial`） |

播放器建議放在函式裡（堆疊），播完 RAM 就還回去。
