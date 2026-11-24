# image：介面

| 需要 | 得到 |
|---|---|
| 一個 `Screen`：`writeWindow(…)` 和它的顏色格式 `ColorFormat`（`ST7789::M130T<Bus, color::RGB565>`、`SSD1306::OLED128x64<Bus, color::BW<>>`） | `image::FlashImage`：畫燒進 Flash 的圖片 |
| 串流：再加一個 `Stream`（`stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`） | `image::StreamImage`：圖片留在電腦，畫的時候才送過來 |
| 圖片檔（png、jpg、webp、heic…） | `picture::*`：`PICTURE("檔案")` 或 `source/convert.py` 轉出來的圖片 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [讀我.md](讀我.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 螢幕 | `Screen`：會裁切的 `writeWindow(x, y, w, h, colors)`、顏色格式 `ColorFormat`（`fromRGB565()` 把檔案的顏色換成螢幕的）、`BusType::readFlash8/16` |
| 資料 | `picture::*` 由 `convert.py` 產生：顏色一律 RGB565，格式版本要對 |
| 串流 | `StreamImage` 要一個 `Stream`（[lib/stream](../stream/INTERFACE.md)）+ 電腦開著 `stream.py`；`draw()` 畫完才回來 |
| 執行時的 RAM | 串流 256 色：調色盤在 `draw()` 裡 `malloc`（色數 × 顏色大小），回來前一定 `free`（`HeapPalette`）；編譯器的 RAM 數字看不到它，放不下回傳 `ERROR_NO_MEMORY` |
| RAM 跟著顏色 | 一段顏色、16 色調色盤都是 `ColorFormat::Value`：RGB888 / RGB666（4 bytes）比 RGB565 多一倍 |

## 檔案

```
lib/image/
├── image.h            入口：兩個類別 + asset.h + output/selected.h
├── asset.h            PICTURE(路徑, 選項) 巨集：編譯前自動轉換（src/tools/assets.py）
├── decoder.h          Decoder<Screen, SEGMENT>：像素格式，畫一張圖；HeapPalette：執行時配置的調色盤
├── flash_image.h      FlashImage<Screen, SEGMENT>：畫 Flash 裡的圖（--target flash）
├── stream_image.h     StreamImage<Screen, Stream, SEGMENT>：畫電腦傳來的圖（--target stream）
├── assets/            原始圖片（sunset.jpg / .png / .webp / .heic、logo.png、moon.jpg、city.png、treble_clef.png）
├── source/            config.json、convert.py、check.py、stream_handler.py（lib/stream 的 stream.py 用它讀圖片），見 source/讀我.md
└── output/            產生的圖片和 selected.h，見 output/讀我.md
```

## 介面需求

| 參數 | 需要的函式 | 現成的實作 |
|---|---|---|
| `Screen` | `writeWindow(x, y, w, h, colors)`（會裁切）；型別 `BusType`，它有 `readFlash8/16`（讀 Flash 裡的圖）、`nowMs`（`StreamImage` 計時） | `ST7789::M130T`、`SSD1306::OLED128x64` |
| `Stream`（`StreamImage`） | `CHUNK_SIZE`、`OK`、`ERROR_*`、`begin()`、`write8()`、`writePath()`、`waitAnswer()`，和 `Answer` 類別（`begin()`、`read8/16/32()`、`failed()`） | `stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`（[lib/stream](../stream/讀我.md)） |
| `Picture` | `output/selected.h` 裡的 struct（見 [output/讀我.md](output/讀我.md#產生的-struct)） | `picture::*` |

## 函式

| 函式 | 說明 |
|---|---|
| `FlashImage::draw<Picture>(x, y)` | 畫 Flash 裡的圖，左上角 (x, y) |
| `StreamImage::draw<Picture>(x, y)` | 跟 `stream.py` 要圖、邊收邊畫，畫完才回來；回傳結果 |

| `StreamImage::draw` 回傳 | 原因 |
|---|---|
| `OK`（0） | — |
| `ERROR_NOT_FOUND`（1） | `stream.py` 找不到檔案（或在 `lib/image` 外面） |
| `ERROR_UNREADABLE`（2） | 檔案讀不懂 |
| `ERROR_NOT_STREAMABLE`（3） | 這個檔案不能串流 |
| `ERROR_NO_ANSWER`（4） | 3 秒沒回答（`stream.py` 沒開），什麼都不畫 |
| `ERROR_BROKEN`（5） | 傳到一半斷了（200 ms 收不到下一塊）或標頭不合理，剩下的像素畫成黑色 |
| `ERROR_NO_MEMORY`（6） | 256 色的調色盤放不進 RAM，什麼都不畫 |

### Decoder、HeapPalette（寫新的來源時用）

`FlashImage`、`StreamImage` 都只是「讀資料」的外殼，畫圖在 `Decoder`；要從別的地方（SD 卡、I2C EEPROM…）讀圖，寫一個有 `uint8_t read8()` 的 Reader 交給它：

| 函式 | 說明 |
|---|---|
| `Decoder(screen)` | 建構：畫在哪個螢幕 |
| `setArea(x, y, width, height, scaleX, scaleY)` | 位置、存的大小、放大倍數 |
| `setFormat(colorMode, encoding)` | `COLORS_2` / `16` / `256` / `565`、`ENCODING_RAW` / `RUNS` |
| `setColor(index, rgb565)` | 2、16 色：一個一個設定調色盤（檔案的 RGB565，這裡換成螢幕的格式） |
| `setFlashPalette(palette)` | 256 色：Flash 裡的 RGB565 調色盤，每個像素換一次 |
| `setRamPalette(colors)` | 256 色：已經是螢幕格式的調色盤（例如 `HeapPalette`），`draw()` 回來前不能釋放 |
| `draw(reader)` | 讀完 width × height 個像素並畫出來 |
| `HeapPalette<Color>::take(count)` | `malloc` count 個顏色（1 ～ 256），先放掉舊的；RAM 不夠回傳 false（avr-libc 會留 128 bytes 給堆疊） |
| `release()`、`colors()` | 放掉、回傳指標；物件消失時自動放掉，不能複製 |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `SEGMENT` | 32 | 一次 `writeWindow` 畫幾個螢幕像素，佔 SEGMENT × 一個顏色的大小（RGB565：2 bytes）RAM |
| `Stream` 的 `CHUNK`（`StreamImage`） | 64 | 一次收幾個 byte，佔同樣多的 RAM |

## 怎麼畫

一列像素切成每段 `SEGMENT` 個螢幕像素，每段呼叫一次 `writeWindow`，每個存的像素重複畫 scaleX × scaleY 次。

| 好處 | 原因 |
|---|---|
| 會裁切 | `writeWindow` 會裁切：圖片可以一部分在螢幕外 |
| 任何螢幕都能用 | 只用到 `writeWindow` |
| RAM 少 | 只留一段的顏色（SEGMENT 個），不需要一整列 |

代價是比 video 的直接串流（`setWindow`）多開很多窗口：240×240 要 1920 段，每段約 56 µs，光開窗口就約 0.1 秒（估計）。

### 跟 video 比較

| | video | image |
|---|---|---|
| 內容 | 動畫（多幀、時間） | 一張圖 |
| 顏色 | 16、256、565 | 2、16、256、565 |
| 編碼 | runs | raw 或 runs，自動選比較小的 |
| 畫法 | `setWindow` + `writePixel` 直接送 | 分段 `writeWindow` |
| 裁切 | ❌ 開始時檢查，放不下就不畫 | ✅ |
| 存的大小上限 | 255（有放大時寬 ≤ 120） | 240 |
| 串流協定 | 開啟、要下一幀、播放控制 | 只有「要這張圖」 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `draw` 的 x、y | 任意 | 裁切 |
| `Picture` | `output/selected.h` 裡的 struct | 編譯錯誤 |
| 存的寬、高 | 1 ～ 240 | `convert.py` 報錯 |
| scale | 1 ～ 8 | `convert.py` 報錯 |
| 調色盤顏色數 | 2：2 色；16：≤ 16；256：≤ 256 | `convert.py` 保證；Flash 版超過時編譯錯誤 |
| `SEGMENT`、`Stream` 的 `CHUNK` | 1 ～ 255 | 0：編譯錯誤 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM（估計，RGB565） | `FlashImage<Screen>` 約 115 bytes（一段顏色 64 + 調色盤 32 + 狀態）；`StreamImage<Screen, Stream>` 約 180 bytes（再加接收緩衝 CHUNK = 64）；串流 256 色時另外在畫的期間配置調色盤。RGB888 等 4 bytes 的顏色，緩衝和調色盤加倍 |
| 時間（實測） | 串流 240×240 全彩（19 KB）約 1.7 秒，128×64 黑白約 0.07 秒 |
| 硬體 | Screen 的；`StreamImage` 另外用 D0 / D1（`UnoSerial`） |

建議放在函式裡（堆疊），畫完就還回去；跟 OLED 1 KB 的畫面一起用也夠。
