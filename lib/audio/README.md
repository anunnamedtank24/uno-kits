# audio

聲音和播放器，`namespace audio`（播放器、格式）和 `namespace melody`（產生的聲音）。聲音由 `source/convert.py` 從音檔（wav、flac、mp3、m4a、ogg、webm…）轉成三種交換格式之一，依**誰來播**（`--sound`）和**品質**（`--quality`）而不同，可以燒進 Flash（`--target uno`），也可以留在 Mac、播放時從串口串流（`--target mac`）。不依賴任何硬體。

## 檔案

```
lib/audio/
├── audio.h                   入口：全部（程式只 include 它）
├── note.h                    noteHz(A, 4) = 440、midiHz、Pitch、REST、beatMs：手寫旋律
├── format.h                  交換格式：KIND_*、編碼、TARGET_*、decodeStep、Adpcm
├── player.h                  Player<Sound, GAP_MS>：tone / beep / beeps / slide、Flash 的音符和節奏
├── stream_link.h             StreamLink<Link>：跟 stream.py 的協定
├── stream_player.h           StreamPlayer<Sound, Link, CHUNK, GAP_MS>：Mac 傳來的音符和節奏
├── sample_player.h           SamplePlayer<SampleOut>：Flash 的取樣（PCM）
├── stream_sample_player.h    StreamSamplePlayer<SampleOut, Link, BUFFER, CHUNK>：Mac 傳來的取樣
├── assets/                   範例音檔（7 種格式 + macOS `say` 的人聲 hello.aiff）
├── source/                   convert.py、check.py、stream.py，見 source/README.md
└── output/                   產生的聲音和 selected.h；output/mac/ 是只給 stream.py 的資料，見 output/README.md
```

## 格式和品質

| `--sound` | 誰播 | 格式（`kind`） | `--quality low` | `medium`（預設） | `high` |
|---|---|---|---|---|---|
| `passive` | 無源蜂鳴器（`Player`、`StreamPlayer`） | 音符：{ 音高, ms } | `NOTES_COMPACT`：半音編號 + 10 ms 為單位，**2 B / 音**，只能是準的半音 | `NOTES_HZ_MS`：Hz + ms，4 B / 音，對齊半音 | `NOTES_HZ_MS`，量到的頻率（滑音、顫音變成很多小步），最短 20 ms |
| `active` | 有源蜂鳴器（同上） | 節奏：{ 響 / 停, ms }，`RHYTHM_MS`，2 B / 步 | 最短的音 80 ms、休止 60 ms | 50 / 40 ms | 20 / 20 ms |
| `pcm` | 無源蜂鳴器或小喇叭，PWM（`SamplePlayer`、`StreamSamplePlayer`） | 取樣：真正的波形（人聲、樂器音色） | 8000 Hz、IMA ADPCM（4 bits）：4 KB / 秒 | 11025 Hz、8 bits：11 KB / 秒 | 16000 Hz、8 bits：16 KB / 秒 |

- **單一參數可以蓋過 `--quality`**：音符有 `--quantize`、`--min-note`、`--min-rest`；節奏有 `--hz`（在無源上播時的音高）；取樣有 `--rate`（2000 ～ 32000）、`--encoding u8 / u4 / adpcm`、`--gain`。
- **取樣的三種編碼**（同一段人聲、8 kHz 實測的訊號 / 誤差）：

| `--encoding` | 每個取樣 | 訊號 / 誤差 | 說明 |
|---|---|---|---|
| `u8` | 8 bits | 35.5 dB | 最清楚 |
| `adpcm` | 4 bits | 17.5 dB | 只存「跟上一個的差」，跟 u4 一樣大但清楚很多；解碼約 100 個週期 |
| `u4` | 4 bits | 9.2 dB | 很粗，有明顯的雜音 |

- **節奏**：響的地方不管有沒有音高都算（鼓、拍手也收得到）；兩個相連的音之間自動留 30 ms 的停，不然有源蜂鳴器會連成一聲。
- **放錯播放器**：取樣給 `Player`、音符給 `SamplePlayer`、`--target mac` 的給 Flash 的播放器，都是**編譯錯誤**；音符給有源蜂鳴器（`Sound::HAS_PITCH` 是 false）是**編譯警告**（只聽得到節奏）。

## Flash 或串流

| | `--target uno` | `--target mac` | `--target uno mac` |
|---|---|---|---|
| 資料在哪 | `output/x.h`，燒進 Flash | `output/mac/x.h`，留在 Mac；板子上的 `output/x.h` 只有長度和路徑 | `output/x.h` |
| 播放 | `Player`、`SamplePlayer` | `StreamPlayer`、`StreamSamplePlayer` + `stream.py` | 兩種都可以 |
| 長度 | 受 Flash 限制：取樣 8 kHz ADPCM 約 6 秒、16 kHz 8 bits 約 1.5 秒 | 不限 | 受 Flash 限制 |

## 接線

| 播什麼 | 接法 |
|---|---|
| 音符、節奏 | 蜂鳴器的接法，見 [buzzer](../buzzer/README.md#接線) |
| 取樣（`Uno::UnoPcmOut`） | **D9 或 D10** → 100 Ω → 無源蜂鳴器 + / 小喇叭；- 接 GND。三隻腳的無源模組（PNP）：I/O 接 D9、用 `Uno::UnoPcmOut<9, true>` |

- **取樣怎麼發聲**：Timer1 在 D9 / D10 送 62.5 kHz 的 PWM，工作週期就是取樣值；62.5 kHz 遠超過聽得到的範圍，蜂鳴器或喇叭只跟得上平均值，所以平均值變成聲音。
- **小喇叭（8 Ω）**：不能直接接腳位（電流太大），串 100 ～ 220 Ω，或用電晶體推；再串一顆 10 µF 電容可以擋掉直流（取樣的中間值是一半的電壓）。
- **蜂鳴器播取樣**：電磁式在 2 ～ 4 kHz 共振最大聲，低音很小、音色偏尖，但人聲聽得出來。

## 用法

```cpp
#include "Uno_tone.h"        // 音符、節奏（Timer1）
#include "Uno_pcm_out.h"     // 取樣（Timer1 + Timer2）
#include "Uno_serial.h"      // 串流
#include "buzzer.h"
#include "audio.h"

using Speaker = buzzer::PassiveBuzzer2Pins<Uno::UnoTone<9>>;   // 無源蜂鳴器 + 接 D9
using Link = Uno::UnoSerial<1000000>;
audio::Player<Speaker> speaker;
audio::StreamPlayer<Speaker, Link> streamSpeaker;
using Samples = audio::SamplePlayer<Uno::UnoPcmOut<9>>;
using StreamSamples = audio::StreamSamplePlayer<Uno::UnoPcmOut<9>, Link>;

void setup() {
    speaker.init();
    speaker.play<melody::ode_to_joy_uno_passive_medium>();             // Flash 的音符
    // streamSpeaker.open<melody::twinkle_star_mac_passive_medium>();  // Mac 的音符
    // Samples::play<melody::hello_uno_pcm_8000_adpcm>();              // Flash 的取樣
    // StreamSamples::open<melody::ode_to_joy_mac_pcm_16000_u8>();     // Mac 的取樣
}

void loop() {
    speaker.update();                                                  // 每次 loop 都要呼叫，不會等
}
```

```bash
cd lib/audio
python -B source/convert.py assets/hello.aiff output --sound pcm --quality low --preview hello.wav
python -B source/convert.py assets/song.mp3 output --target mac --sound pcm --quality high --duration 60
python -B source/stream.py          # 串流時一直開著；燒錄時它會自己放開串口
```

完整的例子：`src/week_5/test_2.cpp`（每種格式從 Flash 和 Mac 各播一次，把實測結果送給 `stream.py` 印出來）、`src/week_4/test_3.cpp`（有源）、`src/week_4/test_4.cpp`（無源）。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Sound`（`Player`、`StreamPlayer`） | `HAS_PITCH`（常數）、`init()`、`start(hz)`（再呼叫一次就換頻率）、`stop()`、`nowMs()`、`readFlash8(address)`、`readFlash16(address)` | `buzzer` 的模組（`ActiveBuzzer2Pins`、`PassiveBuzzer2Pins`…） |
| `SampleOut`（`SamplePlayer`、`StreamSamplePlayer`） | `init()`、`start(rate, next)`（每秒 rate 次在中斷裡呼叫 `next()` 拿下一個取樣）、`stop()`、`realRate()`、`stopInterrupts()`、`restoreInterrupts(saved)`、`readFlash8`、`readFlash16` | `Uno::UnoPcmOut` |
| `Link`（串流） | `init()`、`read8(byte, timeoutMs)`、`write8(byte)`、`clearInput()` | `Uno::UnoSerial` |
| `Melody` / `Clip` | `output/selected.h` 裡的 struct：`target`、`kind`、`encoding`、`count`、`rate`、`rhythmHz`、`getPath()`；`--target uno` 的還有 `getNotes()` 或 `getSamples()` | `melody::*` |

## 函式

### Player（Flash 的音符、節奏，以及手動的聲音）

| 函式 | 說明 |
|---|---|
| `init()` | 初始化 Sound，安靜 |
| `update()` | 時間到了就換下一個音或停止，不會等；每次 loop 都要呼叫 |
| `tone(hz)` | 一直響，直到 `stop()` 或下一個聲音 |
| `beep(hz, ms)` | 響一次 |
| `beeps(hz, count, onMs, offMs)` | 響 count 次，間隔 offMs（最後一次後面不再等） |
| `play<Melody>(repeat)` | 播 `output/` 的音符或節奏，例如 `play<melody::scale_uno_passive_medium>()` |
| `play(notes, count, repeat)` | 播手寫的 `{hz, ms, ...}`（見[手寫旋律](#手寫旋律)） |
| `slide(fromHz, toHz, ms)` | 滑音（以 Hz 直線變化）：警笛、雷射 |
| `stop()` | 立刻安靜 |
| `setVolume(volume)`、`volume()` | 設定、回傳音量 0 ～ 255（預設 255）：無源改工作週期、有源只有 0 = 靜音（見 [buzzer 的音量](../buzzer/README.md#音量)） |
| `isPlaying()`、`currentHz()`、`noteIndex()` | 回傳有沒有在播、現在的頻率（0 = 安靜）、播到第幾步 |

### StreamPlayer（Mac 的音符、節奏）

| 函式 | 說明 |
|---|---|
| `init()` | 初始化 Sound，安靜 |
| `open<Melody>(repeat)` | 跟 `stream.py` 要聲音、等它回答（最多 3 秒）並開始；回傳 `STREAM_OK` 或錯誤 |
| `update()` | 時間到了就播下一步；需要時拉下一塊資料（約 1 ms） |
| `stop()`、`isPlaying()`、`currentHz()`、`noteIndex()`、`error()` | 停止、回傳狀態、錯誤 |

### SamplePlayer、StreamSamplePlayer（取樣，都是 static）

| 函式 | 說明 |
|---|---|
| `init()` | 初始化 SampleOut，安靜 |
| `SamplePlayer::play<Clip>(repeat)` | 播 Flash 的取樣 |
| `StreamSamplePlayer::open<Clip>(repeat)` | 跟 `stream.py` 要取樣、先把緩衝填滿再開始；回傳 `STREAM_OK` 或錯誤 |
| `update()` | 播完就停（或從頭）；串流的還會在緩衝空出 CHUNK 時補資料。每次 loop 都要呼叫 |
| `stop()`、`isPlaying()`、`position()` | 停止、回傳有沒有在播、播了幾個取樣 |
| `setVolume(volume)`、`volume()` | 音量 0 ～ 255（預設 255 = 轉換時的大小）：每個取樣以 128 為中心縮小，128 = -6 dB；要比轉換時更大聲用 `convert.py --gain` |
| `StreamSamplePlayer::underruns()` | 回傳緩衝空掉的次數（那時取樣停在上一個值，會聽到喀的一聲） |
| `StreamSamplePlayer::error()`、`rate()` | 回傳錯誤、取樣率 |

| 串流的錯誤 | 原因 |
|---|---|
| `STREAM_OK`（0） | — |
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
| `StreamLink<Link>::message(text)` | 送一行字給 `stream.py` 印出來（測試用） |

- **一次一個聲音**：開始新的就取代舊的。音符和取樣都用 Timer1，不能同時播。
- **節奏不會飄**：下一個音從「上一個音應該結束的時間」算起，`update()` 晚一點呼叫，也不會讓整首變慢。

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `GAP_MS`（`Player`、`StreamPlayer`） | 20 | 音符的最後這麼多 ms 安靜，連續兩個相同的音才聽得出是兩個（短於 2 × GAP_MS 的音不切；節奏不切，它自己有停） |
| `CHUNK`（`StreamPlayer`） | 32 | 一次拉幾個 byte，佔同樣多的 RAM |
| `BUFFER`（`StreamSamplePlayer`） | 512 | 板子上存幾個 byte 的取樣，2 的次方、16 ～ 1024；要撐得過程式做別的事的時間 |
| `CHUNK`（`StreamSamplePlayer`） | 255 | 一次拉幾個 byte，最多 BUFFER / 2；大 = 來回次數少、比較快 |

## 串流

跟 image、video 一樣是「板子拉」：Mac 不會主動送超過 1 個 byte（UNO 的串口只有 2 個 byte 的緩衝，沒有中斷接收）。

| 方向 | 內容 |
|---|---|
| 板子 → Mac | `'A'`, CHUNK, 路徑長度, 路徑：要這個聲音 |
| Mac → 板子 | `'D'`：答案準備好了 |
| 板子 → Mac | `'R'`：要下一塊 |
| Mac → 板子 | [長度 1 ～ CHUNK][資料] |
| 答案 | `'H'`, kind, encoding, rate（u16）, count（u32）, rhythmHz（u16）, 資料大小（u32）, 資料；或 `'E'`, 代碼, 訊息長度, 訊息 |
| 板子 → Mac | `'M'`, 長度, 文字：`stream.py` 印成 `[Board] …` |

- **取樣**：中斷每個取樣從緩衝拿一個 byte（ADPCM 是半個），`update()` 看到空出 CHUNK 就拉一塊。拉的時候在等串口，但中斷照常播放。
- **音符**：一步才 2 ～ 4 bytes，用到才拉，每次約 1 ms。

**實測**（`src/week_5/test_2.cpp`，無源蜂鳴器在 D9）：

| 測試 | 結果 |
|---|---|
| Flash 的取樣，8 kHz ADPCM | 實際 8000 Hz；第一秒 7989 個取樣，2.54 秒的聲音播 2543 ms |
| Mac 的音符 | 14 步，6405 ms，跟 Flash 的一樣 |
| Mac 的取樣，16 kHz 8 bits，BUFFER 512 / CHUNK 255 | 7.46 秒的聲音播 7462 ～ 7465 ms；緩衝空掉 0 ～ 44 次（約 16 KB/秒，剛好在極限） |
| 同上，BUFFER 256 / CHUNK 64 | 空掉 3583 次：不夠 |

- **串流大約每秒 16 KB 就到頂**：要穩定就用每秒 12 KB 以內：11025 Hz 8 bits、或 22050 Hz ADPCM（11 KB/秒）。
- **踩過的坑**：`stream.py` 原本每 0.3 秒跑一次 `ps`（檢查有沒有在燒錄），那幾十 ms 不回應板子，速度只剩 13 KB/秒；現在放在另一個執行緒。

## 手寫旋律

不想從音檔轉時，也可以直接寫：

```cpp
#include <avr/pgmspace.h>
using namespace audio;

const uint16_t MY_SONG[] PROGMEM = {
    noteHz(E, 5), beatMs(120), noteHz(G, 5), beatMs(120, 1, 2), REST, 100, noteHz(C, 6), 600,
};
speaker.play(MY_SONG, sizeof(MY_SONG) / sizeof(MY_SONG[0]) / 2);
```

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `hz` | Sound 能發的範圍（`UnoTone`：16 ～ 20000） | 夾到範圍內（Sound 做）；0 = 安靜 |
| `ms`、`onMs`、`offMs` | 0 ～ 65535 | — |
| `beeps` 的 `count` | 1 ～ 255 | 0：安靜 |
| `Player` 的步數 | 1 ～ 65535（Flash 也有限制） | 編譯錯誤（`play<Melody>`）；`nullptr` 或 0：安靜 |
| 取樣率 | 2000 ～ 32000（`UnoPcmOut`：1954 ～ 32000） | `convert.py` 報錯；板子夾到範圍內 |
| 放錯播放器、`--target mac` 給 Flash 的播放器 | — | 編譯錯誤 |
| 音符給有源蜂鳴器 | — | 編譯警告 |
| `BUFFER`、`CHUNK` | 見參數 | 編譯錯誤 |
| `noteHz` 的八度 | 0 ～ 8 | 回傳 0（休止） |
| `update()` 的間隔 | 越常越好 | 音符：邊界晚，但節奏不飄；串流取樣：緩衝撐不住就 underrun |
| `notes` 指標 | 必須指向 `PROGMEM` 的陣列 | 不檢查：播出亂七八糟的音 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `Player` 約 32 bytes；`StreamPlayer` 約 30 + CHUNK；`SamplePlayer` 約 20；`StreamSamplePlayer` 約 30 + BUFFER（預設 512） |
| Flash | 音符 2 ～ 4 bytes / 步；取樣 4 ～ 16 KB / 秒；沒播的聲音不佔 Flash |
| 計時器 | 音符：Timer1（`UnoTone`）；取樣：Timer1（PWM）+ Timer2（取樣時鐘） |
| 中斷 | 取樣：每個取樣一次，8 bits 約 3 µs、ADPCM 約 8 µs（16 kHz ADPCM 約佔 13% CPU，估計） |
| 串口 | 串流用 D0 / D1（`UnoSerial`），1 Mbaud |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 和弦、兩個聲部 | 音符一次只有一個音 | 轉換時挑主旋律；要原本的聲音就用 `--sound pcm` |
| 有源蜂鳴器播音符 | 只聽得到節奏（編譯警告） | 用 `--sound active` 轉成節奏 |
| 很短的音（< 40 ms） | `GAP_MS` 讓它一半時間安靜 | `GAP_MS` 調小，或 `--min-note` 調大 |
| 取樣和 `UnoTone`、D3 / D9 / D10 / D11 的 PWM（LED）一起用 | 搶 Timer1 / Timer2 | 一次只用一個；LED 改接 D5 / D6 |
| 串流取樣時程式做很久的事（OLED 的 `display()` 64 ms） | 緩衝用完：underrun，喀喀聲 | `BUFFER` 加大（8 kHz 8 bits：64 ms 要 512 B），或降低取樣率、改 ADPCM |
| 同時開 image / video 的 `stream.py`、Arduino 的 `Serial` | 搶同一個串口 | 一次只開一個 |
| 播放中直接動腳位（`digitalWrite`、`analogWrite`） | 播放器不知道，狀態對不上 | 只透過播放器控制 |
