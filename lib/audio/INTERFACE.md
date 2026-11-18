# audio：介面

| 需要 | 得到 |
|---|---|
| 播音符、節奏：`Sound`（[buzzer](../buzzer/README.md) 的模組） | `audio::Player`：嗶聲、旋律、滑音，播放時不會等 |
| 播取樣：`SampleOut`（`Uno::UnoPcmOut<9>`） | `audio::SamplePlayer`：真正的波形（說話、音樂） |
| 串流：再加一個 `Stream`（`stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`） | `audio::StreamPlayer`、`StreamSamplePlayer`：聲音留在電腦，播放時才送過來 |
| 音檔（wav、mp3、m4a…） | `melody::*`：`MELODY("檔案")` 或 `source/convert.py` 轉出來的聲音 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 發聲 | `Sound`：buzzer 的模組（無源用 `Uno::UnoTone`：Timer1）；`SampleOut`：`Uno::UnoPcmOut`（Timer1 PWM + Timer2 取樣中斷），兩者不能同時用 |
| 呼叫 | `update()` 每次 loop 都要呼叫；播放函式不會等，時間到才換下一個音 |
| 資料 | `melody::*` 由 `convert.py` 產生，格式要是目前的 `FORMAT_VERSION`（`check.py` 檢查） |
| 串流 | `Stream`（[lib/stream](../stream/INTERFACE.md)）+ 電腦開著 `stream.py`；`StreamSamplePlayer` 全是 static：一個 `SampleOut` 型別一個 |
| RAM | `StreamSamplePlayer` 的 BUFFER（預設 512 bytes）是 static，編譯器算得到 |

## 檔案

```
lib/audio/
├── audio.h                   入口：全部（程式只 include 它）
├── asset.h                   MELODY(路徑, 選項) 巨集：編譯前自動轉換（tools/assets.py）
├── note.h                    noteHz(A, 4) = 440、midiHz、Pitch、REST、beatMs：手寫旋律
├── format.h                  交換格式：KIND_*、編碼、TARGET_*、decodeStep、Adpcm
├── player.h                  Player<Sound, GAP_MS>：tone / beep / beeps / slide、Flash 的音符和節奏
├── stream_player.h           StreamPlayer<Sound, Stream, GAP_MS>：電腦傳來的音符和節奏
├── sample_player.h           SamplePlayer<SampleOut>：Flash 的取樣（PCM）
├── stream_sample_player.h    StreamSamplePlayer<SampleOut, Stream, BUFFER>：電腦傳來的取樣
├── assets/                   範例音檔（7 種格式 + macOS `say` 的人聲 hello.aiff）
├── source/                   convert.py、check.py、stream_handler.py（lib/stream 的 stream.py 用它讀聲音），見 source/README.md
└── output/                   產生的聲音和 selected.h，見 output/README.md
```

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Sound`（`Player`、`StreamPlayer`） | `HAS_PITCH`（常數）、`init()`、`start(hz)`（再呼叫一次就換頻率）、`stop()`、`nowMs()`、`readFlash8(address)`、`readFlash16(address)` | `buzzer` 的模組（`ActiveBuzzer2Pins`、`PassiveBuzzer2Pins`…） |
| `SampleOut`（`SamplePlayer`、`StreamSamplePlayer`） | `init()`、`start(rate, next)`（每秒 rate 次在中斷裡呼叫 `next()` 拿下一個取樣）、`stop()`、`realRate()`、`stopInterrupts()`、`restoreInterrupts(saved)`、`readFlash8`、`readFlash16` | `Uno::UnoPcmOut` |
| `Stream`（串流） | `CHUNK_SIZE`、`OK`、`ERROR_*`、`begin()`、`write8()`、`writePath()`、`waitAnswer()`；`StreamPlayer` 另外要 `Answer` 類別，`StreamSamplePlayer` 另外要 `pullLength()`、`receive()` | `stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`（[lib/stream](../stream/README.md)） |
| `Melody` / `Clip` | `output/selected.h` 裡的 struct：`target`、`kind`、`encoding`、`count`、`rate`、`rhythmHz`、`getPath()`；`--target flash` 的還有 `getNotes()` 或 `getSamples()` | `melody::*` |

## 函式

### Player（Flash 的音符、節奏，以及手動的聲音）

| 函式 | 說明 |
|---|---|
| `init()` | 初始化 Sound，安靜 |
| `update()` | 時間到了就換下一個音或停止，不會等；每次 loop 都要呼叫 |
| `tone(hz)` | 一直響，直到 `stop()` 或下一個聲音 |
| `beep(hz, ms)` | 響一次 |
| `beeps(hz, count, onMs, offMs)` | 響 count 次，間隔 offMs（最後一次後面不再等） |
| `play<Melody>(repeat)` | 播 `output/` 的音符或節奏，例如 `play<melody::scale_flash_passive_medium>()` |
| `play(notes, count, repeat)` | 播手寫的 `{hz, ms, ...}`（見[手寫旋律](README.md#手寫旋律)） |
| `slide(fromHz, toHz, ms)` | 滑音（以 Hz 直線變化）：警笛、雷射 |
| `stop()` | 立刻安靜 |
| `setVolume(volume)`、`volume()` | 設定、回傳音量 0 ～ 255（預設 255）：無源改工作週期、有源只有 0 = 靜音（見 [buzzer 的音量](../buzzer/README.md#音量)） |
| `isPlaying()`、`currentHz()`、`noteIndex()` | 回傳有沒有在播、現在的頻率（0 = 安靜）、播到第幾步 |

### StreamPlayer（電腦的音符、節奏）

| 函式 | 說明 |
|---|---|
| `init()` | 初始化 Sound，安靜 |
| `open<Melody>(repeat)` | 跟 `stream.py` 要聲音、等它回答（最多 3 秒）並開始；回傳 `OK` 或錯誤 |
| `update()` | 時間到了就播下一步；需要時拉下一塊資料（約 1 ms） |
| `stop()`、`isPlaying()`、`currentHz()`、`noteIndex()`、`error()` | 停止、回傳狀態、錯誤 |

### SamplePlayer、StreamSamplePlayer（取樣，都是 static）

| 函式 | 說明 |
|---|---|
| `init()` | 初始化 SampleOut，安靜 |
| `SamplePlayer::play<Clip>(repeat)` | 播 Flash 的取樣 |
| `StreamSamplePlayer::open<Clip>(repeat)` | 跟 `stream.py` 要取樣、先把緩衝填滿再開始；回傳 `OK` 或錯誤 |
| `update()` | 播完就停（或從頭）；串流的還會在緩衝空出 CHUNK 時補資料。每次 loop 都要呼叫 |
| `stop()`、`isPlaying()`、`position()` | 停止、回傳有沒有在播、播了幾個取樣 |
| `setVolume(volume)`、`volume()` | 音量 0 ～ 255（預設 255 = 轉換時的大小）：每個取樣以 128 為中心縮小，128 = -6 dB；要比轉換時更大聲用 `convert.py --gain` |
| `StreamSamplePlayer::underruns()` | 回傳緩衝空掉的次數（那時取樣停在上一個值，會聽到喀的一聲） |
| `StreamSamplePlayer::error()`、`rate()` | 回傳錯誤、取樣率 |

| 串流的錯誤（播放器的常數，例如 `streamSpeaker.OK`、`StreamSamples::ERROR_NO_ANSWER`；數字跟 [lib/stream](../stream/INTERFACE.md#函式) 一樣） | 原因 |
|---|---|
| `OK`（0） | — |
| `ERROR_NOT_FOUND`（1） | `stream.py` 找不到檔案（或在 `lib/audio` 外面） |
| `ERROR_UNREADABLE`（2） | 檔案讀不懂 |
| `ERROR_WRONG_KIND`（3） | 取樣給了 `StreamPlayer`，或音符給了 `StreamSamplePlayer` |
| `ERROR_NO_ANSWER`（4） | 3 秒沒回答（`stream.py` 沒開，或串口被佔用） |
| `ERROR_BROKEN`（5） | 傳到一半斷了（200 ms 收不到下一塊）或資料不合理 |

### 其他

| 函式 | 說明 |
|---|---|
| `noteHz(pitch, octave)` | 回傳音高的頻率：十二平均律、A4 = 440 Hz，`CS` 是 C#；C4 是中央 C（262 Hz） |
| `midiHz(midi)` | 回傳 MIDI 音的頻率：60 = C4、69 = A4；12 ～ 119 以外回傳 0 |
| `beatMs(bpm, beats, divide)` | 回傳音符長度：`beatMs(120, 1, 2)` = 每分鐘 120 拍時的半拍 = 250 ms |
| `stream::Puller<Link, CHUNK>::message(text)` | 送一行字給 `stream.py` 印出來（測試用，見 [lib/stream](../stream/INTERFACE.md#函式)） |

- **一次一個聲音**：開始新的就取代舊的。音符和取樣都用 Timer1，不能同時播。
- **節奏不會飄**：下一個音從「上一個音應該結束的時間」算起，`update()` 晚一點呼叫，也不會讓整首變慢。

### format.h（寫新的播放器時用）

| 函式 | 說明 |
|---|---|
| `stepBytes(encoding)` | 回傳音符 / 節奏的一步佔幾個 byte |
| `decodeStep(encoding, first, second, rhythmHz, hz, ms)` | 把一步的兩個字解成 hz（0 = 安靜）和 ms；節奏的「開」用 rhythmHz |
| `Adpcm::reset()`、`decode<readFlash16>(code)` | IMA ADPCM：每個 4 bit 的碼解成一個 8 bit 取樣（0 ～ 255），跟 `convert.py` 的解碼器一樣 |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `GAP_MS`（`Player`、`StreamPlayer`） | 20 | 音符的最後這麼多 ms 安靜，連續兩個相同的音才聽得出是兩個（短於 2 × GAP_MS 的音不切；節奏不切，它自己有停） |
| `CHUNK`（`StreamPlayer`） | 32 | 一次拉幾個 byte，佔同樣多的 RAM |
| `BUFFER`（`StreamSamplePlayer`） | 512 | 板子上存幾個 byte 的取樣，2 的次方、16 ～ 1024；要撐得過程式做別的事的時間 |
| `CHUNK`（`StreamSamplePlayer`） | 255 | 一次拉幾個 byte，最多 BUFFER / 2；大 = 來回次數少、比較快 |

## 串流

跟 image、video 一樣用 [lib/stream](../stream/INTERFACE.md#協定)：板子送指令、電腦只回 `'D'`、板子用 `'R'` 一塊一塊拉（UNO 的串口只有 2 個 byte 的緩衝，沒有中斷接收）。聲音的部分：

| 方向 | 內容 |
|---|---|
| 板子 → 電腦 | `'A'`, CHUNK, 路徑長度, 路徑：要這個聲音 |
| 答案 | `'H'`, kind, encoding, rate（u16）, count（u32）, rhythmHz（u16）, 資料大小（u32）, 資料；或 `'E'`, 代碼, 訊息長度, 訊息 |

- **取樣**：中斷每個取樣從緩衝拿一個 byte（ADPCM 是半個），`update()` 看到空出 CHUNK 就拉一塊。拉的時候在等串口，但中斷照常播放。
- **音符**：一步才 2 ～ 4 bytes，用到才拉，每次約 1 ms。

**實測**（`src/week_5_NEC/tests/2_sound_formats.cpp`，無源蜂鳴器在 D9）：

| 測試 | 結果 |
|---|---|
| Flash 的取樣，8 kHz ADPCM | 實際 8000 Hz；第一秒 7989 個取樣，2.54 秒的聲音播 2543 ms |
| 電腦的音符 | 14 步，6405 ms，跟 Flash 的一樣 |
| 電腦的取樣，16 kHz 8 bits，BUFFER 512 / CHUNK 255 | 7.46 秒的聲音播 7462 ～ 7465 ms；緩衝空掉 0 ～ 44 次（約 16 KB/秒，剛好在極限） |
| 同上，BUFFER 256 / CHUNK 64 | 空掉 3583 次：不夠 |

- **串流大約每秒 16 KB 就到頂**：要穩定就用每秒 12 KB 以內：11025 Hz 8 bits、或 22050 Hz ADPCM（11 KB/秒）。
- **踩過的坑**：`stream.py` 原本每 0.3 秒跑一次 `ps`（檢查有沒有在燒錄），那幾十 ms 不回應板子，速度只剩 13 KB/秒；現在放在另一個執行緒。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `hz` | Sound 能發的範圍（`UnoTone`：16 ～ 20000） | 夾到範圍內（Sound 做）；0 = 安靜 |
| `ms`、`onMs`、`offMs` | 0 ～ 65535 | — |
| `beeps` 的 `count` | 1 ～ 255 | 0：安靜 |
| `Player` 的步數 | 1 ～ 65535（Flash 也有限制） | 編譯錯誤（`play<Melody>`）；`nullptr` 或 0：安靜 |
| 取樣率 | 2000 ～ 32000（`UnoPcmOut`：1954 ～ 32000） | `convert.py` 報錯；板子夾到範圍內 |
| 放錯播放器、`--target stream` 給 Flash 的播放器 | — | 編譯錯誤 |
| 音符給有源蜂鳴器 | — | 編譯警告 |
| `BUFFER`、`CHUNK` | 見參數 | 編譯錯誤 |
| `noteHz` 的八度 | 0 ～ 8 | 回傳 0（休止） |
| `update()` 的間隔 | 越常越好 | 音符：邊界晚，但節奏不飄；串流取樣：緩衝撐不住就 underrun |
| `notes` 指標 | 必須指向 `PROGMEM` 的陣列 | 不檢查：播出亂七八糟的音 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `Player` 約 32 bytes；`StreamPlayer` 約 30 + Stream 的 CHUNK；`SamplePlayer` 約 20；`StreamSamplePlayer` 約 30 + BUFFER（預設 512） |
| Flash | 音符 2 ～ 4 bytes / 步；取樣 4 ～ 16 KB / 秒；沒播的聲音不佔 Flash |
| 計時器 | 音符：Timer1（`UnoTone`）；取樣：Timer1（PWM）+ Timer2（取樣時鐘） |
| 中斷 | 取樣：每個取樣一次，8 bits 約 3 µs、ADPCM 約 8 µs（16 kHz ADPCM 約佔 13% CPU，估計） |
| 串口 | 串流用 D0 / D1（`UnoSerial`），1 Mbaud |
