# stream：介面

| 需要 | 得到 |
|---|---|
| 一個 `Link`：`init()`、`available()`、`read8(byte, timeoutMs)`、`write8()`、`clearInput()`、`readFlash8()`（`Uno::UnoSerial<BAUD>`） | `stream::Puller<Link, CHUNK>`：跟電腦要檔案、一塊一塊拉回來；`image::StreamImage`、`video::StreamPlayer`、`audio::StreamPlayer`、`audio::StreamSamplePlayer` 的 `Stream` 參數 |
| 電腦：Python 3、`pip install pyserial` | `source/stream.py`：一個程式同時送圖片、動畫、聲音，板子要什麼就送什麼 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 串口 | `Link`：`Uno::UnoSerial`（D0 / D1）；不能同時用 Arduino 的 `Serial`、`pio device monitor` |
| 電腦 | `stream.py` 要開著；它載入每個 `lib/*/source/stream_handler.py` |
| 協定 | 電腦主動送的永遠不超過 1 byte（UNO 的 UART 只能暫存 2 bytes）：資料一定由板子用 `'R'` 拉 |
| 一次一種 | 回答一次一個；同時串流兩種（例如動畫播著又畫串流圖）會交錯 |
| RAM | `Puller` 全是 static；每個 `Answer` CHUNK + 3 bytes（在使用它的物件裡） |

## 檔案

```
lib/stream/
├── stream.h          入口
├── puller.h          stream::Puller<Link, CHUNK>：送指令、等 'D'、用 'R' 拉回答；Answer：一個回答一個 byte 一個 byte 讀
└── source/
    └── stream.py     電腦端：找板子、收指令、交給各函式庫的 stream_handler.py、把回答一塊一塊送出去
```

各函式庫自己的部分：

| 檔案 | 指令 | 讀什麼 |
|---|---|---|
| `lib/image/source/stream_handler.py` | `'I'` | 圖片：尺寸、調色盤、像素 |
| `lib/video/source/stream_handler.py` | `'O'`、`'N'`、`'P'`、`'S'`、`'J'`、`'V'`、`'L'` | 動畫：計時、播放控制、每一幀和前一幀的差異 |
| `lib/audio/source/stream_handler.py` | `'A'` | 聲音：音符、節奏、取樣 |

## 介面需求

| 參數 | 需要的函式 | 現成的實作 |
|---|---|---|
| `Link` | `init()`、`available()`、`read8(byte, timeoutMs)`（逾時回傳 false）、`write8(byte)`、`clearInput()`、`readFlash8(address)`（路徑在 Flash） | `Uno::UnoSerial<BAUD>`（`Uno_serial.h`，D0 / D1） |

image、video、audio 對 `Stream` 的需求（`Puller` 都有）：

| 需要 | 誰用 |
|---|---|
| `CHUNK_SIZE`、`OK`、`ERROR_NOT_FOUND` ～ `ERROR_BROKEN` | 全部 |
| `begin()`、`write8/16/32()`、`writePath(path)`、`waitAnswer(timeoutMs)` | 全部 |
| `available()` | `video::StreamPlayer`（不等待地看有沒有回答） |
| `Answer`：`begin()`、`read8/16/32()`、`failed()` | `StreamImage`、`video::StreamPlayer`、`audio::StreamPlayer` |
| `pullLength(length)`、`receive(byte)` | `audio::StreamSamplePlayer`（直接拉進自己的環形緩衝） |
| `message(text)` | 測試程式：`stream.py` 印出 `[BOARD] …` |

## 函式

| 函式 | 說明 |
|---|---|
| `begin()` | 啟動串口，丟掉還沒讀的（例如上一個回答斷掉的一半） |
| `write8(v)`、`write16(v)`、`write32(v)` | 送一個指令或參數（little-endian） |
| `writePath(path)` | 送路徑長度和路徑；路徑在 Flash（`getPath()`） |
| `available()` | 電腦有送東西來 |
| `waitAnswer(ms = 3000)` | 等 `'D'`（回答準備好了）；逾時或收到別的就回傳 false |
| `pullLength(length)` | 送 `'R'`，收這一塊的長度（1 ～ CHUNK） |
| `receive(byte)` | 收這一塊的一個 byte（200 ms 逾時） |
| `message(text)` | 送一行文字給 `stream.py` 印出來 |
| `Answer::begin()` | `waitAnswer()` 之後，開始讀一個回答 |
| `Answer::read8()`、`read16()`、`read32()` | 讀下一個值，用完一塊就自動拉下一塊；失敗後都回傳 0 |
| `Answer::failed()` | 有一塊沒收到或不合理 |

| 結果（`OK`、`ERROR_*`） | 數值 | 誰發現 | 原因 |
|---|---|---|---|
| `OK` | 0 | — | — |
| `ERROR_NOT_FOUND` | 1 | `stream.py` | 找不到檔案，或在 `lib/<種類>` 外面 |
| `ERROR_UNREADABLE` | 2 | `stream.py` | 檔案讀不懂 |
| `ERROR_NOT_STREAMABLE` | 3 | `stream.py` | 這個播放器不能用（例如取樣給音符播放器） |
| `ERROR_NO_ANSWER` | 4 | 板子 | 3 秒沒回答：`stream.py` 沒開，或串口被佔用 |
| `ERROR_BROKEN` | 5 | 板子 | 一塊 200 ms 沒收完，或內容不合理 |
| `ERROR_NO_MEMORY` | 6 | 板子 | 執行時配置的 RAM（256 色的調色盤）放不下 |

`video::StreamPlayer` 另外有 `ERROR_TOO_BIG`（7）：動畫放不進螢幕。

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Link` | — | 串口，例如 `Uno::UnoSerial<BAUD>` |
| `CHUNK` | 64 | 一次拉幾個 byte，1 ～ 255；`Answer` 佔同樣多的 RAM。越大來回越少：UNO 的 USB 晶片每塊要等約 4 ms（16 kHz 的取樣要 255 才夠快） |

## 協定

原則：**電腦主動送的資料永遠不超過 1 byte**。UNO 的 UART 只能暫存 2 個 byte，板子在畫圖或做別的事時，多送的資料會遺失。所以電腦只說「好了」，資料由板子有空時自己拉。

| 步驟 | 方向 | 內容 |
|---|---|---|
| 1 | 板子 → 電腦 | 指令（`'I'`、`'A'`、`'O'`…）、CHUNK、該指令的參數、路徑長度、路徑 |
| 2 | 電腦 → 板子 | `'D'`：回答準備好了（只有 1 byte） |
| 3 | 板子 → 電腦 | `'R'`：要下一塊 |
| 4 | 電腦 → 板子 | `[長度 1 ～ CHUNK][資料]`；板子處理完再回到 3，直到整個回答拉完 |
| — | 板子 → 電腦 | `'M'`、長度、文字：`stream.py` 印成 `[BOARD] …`（測試用） |

- **一次一個回答**：前一個回答被拉完，電腦才送下一個 `'D'`。
- **回答的第一個 byte 是種類**：`'E'` 是錯誤（代碼 1 ～ 3、訊息長度、訊息），其他的由各函式庫定義：

| 函式庫 | 回答 | 詳細 |
|---|---|---|
| image | `'H'` 尺寸、顏色模式、調色盤、像素 | [image/source/README.md](../image/source/README.md#stream_handlerpy) |
| video | `'G'` 標頭、`'F'` 幀、`'K'` 沒有新幀、`'A'` 指令完成 | [video/source/README.md](../video/source/README.md#stream_handlerpy) |
| audio | `'H'` 種類、編碼、取樣率、數量、資料 | [audio/README.md](../audio/INTERFACE.md#串流) |

- **顏色**：串流的顏色跟檔案一樣是 RGB565，板子再換成螢幕的顏色格式（`Screen::ColorFormat`，見 [lib/color](../color/README.md)）。
- **256 色**：調色盤跟著標頭先傳，板子在執行時配置 RAM 放它（編譯時會警告，見 [image](../image/README.md#串流-256-色調色盤先傳放在執行時配置的-ram)）。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `CHUNK` | 1 ～ 255 | 編譯錯誤；各播放器另有下限（`video::StreamPlayer` ≥ 16、`audio::StreamPlayer` ≥ 4、`StreamSamplePlayer` ≤ BUFFER / 2） |
| 路徑 | 最多 255 個字元 | 截斷 |
| `message` 的文字 | 最多 255 個字元，在 RAM | 截斷 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `Puller` 本身 0（全是 static）；每個 `Answer` CHUNK + 3 bytes |
| 時間 | 每塊約 4 ms 的往返（UNO 的 USB 晶片）+ 傳輸時間（1 Mbaud 約 10 µs 一個 byte） |
| 硬體 | `Link` 的：`Uno::UnoSerial` 用 D0 / D1 |
