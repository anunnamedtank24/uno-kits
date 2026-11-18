# audio

| 需要 | 得到 |
|---|---|
| 播音符、節奏：`Sound`（[buzzer](../buzzer/README.md) 的模組） | `audio::Player`：嗶聲、旋律、滑音，播放時不會等 |
| 播取樣：`SampleOut`（`Uno::UnoPcmOut<9>`） | `audio::SamplePlayer`：真正的波形（說話、音樂） |
| 串流：再加一個 `Stream`（`stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>`） | `audio::StreamPlayer`、`StreamSamplePlayer`：聲音留在電腦，播放時才送過來 |
| 音檔（wav、mp3、m4a…） | `melody::*`：`MELODY("檔案")` 或 `source/convert.py` 轉出來的聲音 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

聲音和播放器，`namespace audio`（播放器、格式）和 `namespace melody`（產生的聲音）。聲音由 `source/convert.py` 從音檔（wav、flac、mp3、m4a、ogg、webm…）轉成三種交換格式之一，依**誰來播**（`--sound`）和**品質**（`--quality`）而不同，可以燒進 Flash（`--target flash`），也可以留在電腦、播放時從串口串流（`--target stream`）。

## 用法

```cpp
#include "Uno_tone.h"        // 音符、節奏（Timer1）
#include "Uno_pcm_out.h"     // 取樣（Timer1 + Timer2）
#include "Uno_serial.h"      // 串流
#include "stream.h"
#include "buzzer.h"
#include "audio.h"

constexpr uint8_t BUZZER_PIN = 9;                              // 無源蜂鳴器的 +（取樣只能 D9、D10）
constexpr uint32_t BAUD = 1000000;                             // 跟 stream.py 的 --baud 一樣
constexpr uint8_t NOTE_CHUNK = 32;                             // 音符：一次拉幾個 byte
constexpr uint8_t PCM_CHUNK = 255;                             // 取樣：越大越快（16 kHz 要 255）

using Speaker = buzzer::PassiveBuzzer2Pins<Uno::UnoTone<BUZZER_PIN>>;
using Link = Uno::UnoSerial<BAUD>;
using NoteLink = stream::Puller<Link, NOTE_CHUNK>;
using PcmLink = stream::Puller<Link, PCM_CHUNK>;
audio::Player<Speaker> speaker;
audio::StreamPlayer<Speaker, NoteLink> streamSpeaker;
using Samples = audio::SamplePlayer<Uno::UnoPcmOut<BUZZER_PIN>>;
using StreamSamples = audio::StreamSamplePlayer<Uno::UnoPcmOut<BUZZER_PIN>, PcmLink>;

void setup() {
    speaker.init();
    speaker.play<melody::ode_to_joy_flash_passive_medium>();             // Flash 的音符
    // streamSpeaker.open<melody::twinkle_star_stream_passive_medium>();  // 電腦的音符
    // Samples::play<melody::hello_flash_pcm_8000_adpcm>();              // Flash 的取樣
    // StreamSamples::open<melody::ode_to_joy_stream_pcm_16000_u8>();     // 電腦的取樣
}

void loop() {
    speaker.update();                                                  // 每次 loop 都要呼叫，不會等
}
```

```bash
cd lib/audio
python source/convert.py assets/hello.aiff output --sound pcm --quality low --preview hello.wav
python source/convert.py assets/song.mp3 output --target stream --sound pcm --quality high --duration 60
cd ../.. && python lib/stream/source/stream.py     # 串流時一直開著；燒錄時它會自己放開串口（lib/stream）
```

完整的例子：`src/week_5_NEC/tests/2_sound_formats.cpp`（每種格式從 Flash 和電腦各播一次，把實測結果送給 `stream.py` 印出來）、`src/week_4_SSD1306_switch_buzzer_audio/tests/3_active_buzzer.cpp`（有源）、`src/week_4_SSD1306_switch_buzzer_audio/tests/4_passive_buzzer.cpp`（無源）。

## 直接用檔案（推薦）

程式裡寫 `MELODY("路徑", "選項")`，編譯前自動轉換，不用先跑 `convert.py`，檔案也不用放在 `assets/`：

```cpp
#include "audio.h"
using Chime = MELODY("sounds/chime.m4a", "--sound passive --quality high");
speaker.play<Chime>();
```

| 項目 | 說明 |
|---|---|
| 路徑 | 絕對路徑、`~/…`、相對於寫它的 .cpp；找不到時再試專案資料夾、`lib/audio/assets/` |
| 選項 | 一個字串，就是 `source/convert.py` 的選項：`--target`、`--sound`、`--quality`、`--start`、`--duration`…；全部省略時是轉換工具的預設（`--sound passive --quality medium`） |
| 轉到哪 | `lib/audio/output/`，跟手動跑 `convert.py` 的一樣（`tools/assets.py` 每次編譯前找：同一個檔案、同樣的選項已經轉過就直接用，沒有才轉） |
| 型態 | `MELODY(...)` 就是那個 `melody::` struct，跟手動轉換的一樣用 |
| 限制 | 兩個參數都要是字串常數（不能是變數） |

完整的說明：[lib/README.md](../README.md#字型圖片影片聲音直接用檔案)；例子：`src/week_4_SSD1306_switch_buzzer_audio/tests/5_files.cpp`（四種都有）。下面「用法」是手動轉換的方式，一樣能用。

## 接線

| 播什麼 | 接法 |
|---|---|
| 音符、節奏 | 蜂鳴器的接法，見 [buzzer](../buzzer/README.md#接線) |
| 取樣（`Uno::UnoPcmOut`） | **D9 或 D10** → 100 Ω → 無源蜂鳴器 + / 小喇叭；- 接 GND。三隻腳的無源模組（PNP）：I/O 接 D9、用 `Uno::UnoPcmOut<9, true>` |

- **取樣怎麼發聲**：Timer1 在 D9 / D10 送 62.5 kHz 的 PWM，工作週期就是取樣值；62.5 kHz 遠超過聽得到的範圍，蜂鳴器或喇叭只跟得上平均值，所以平均值變成聲音。
- **小喇叭（8 Ω）**：不能直接接腳位（電流太大），串 100 ～ 220 Ω，或用電晶體推；再串一顆 10 µF 電容可以擋掉直流（取樣的中間值是一半的電壓）。
- **蜂鳴器播取樣**：電磁式在 2 ～ 4 kHz 共振最大聲，低音很小、音色偏尖，但人聲聽得出來。

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
- **放錯播放器**：取樣給 `Player`、音符給 `SamplePlayer`、`--target stream` 的給 Flash 的播放器，都是**編譯錯誤**；音符給有源蜂鳴器（`Sound::HAS_PITCH` 是 false）是**編譯警告**（只聽得到節奏）。

## Flash 或串流

| | `--target flash` | `--target stream` | `--target flash stream` |
|---|---|---|---|
| 資料在哪 | `output/x.h`，燒進 Flash | `output/x.h` 的 `#if 0` 裡，留在電腦；板子只編譯前面的長度和路徑 | `output/x.h` |
| 播放 | `Player`、`SamplePlayer` | `StreamPlayer`、`StreamSamplePlayer` + `stream.py` | 兩種都可以 |
| 長度 | 受 Flash 限制：取樣 8 kHz ADPCM 約 6 秒、16 kHz 8 bits 約 1.5 秒 | 不限 | 受 Flash 限制 |

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
| 放錯播放器、`--target stream` 給 Flash 的播放器 | — | 編譯錯誤 |
| 音符給有源蜂鳴器 | — | 編譯警告 |
| `BUFFER`、`CHUNK` | 見參數 | 編譯錯誤 |
| `noteHz` 的八度 | 0 ～ 8 | 回傳 0（休止） |
| `update()` 的間隔 | 越常越好 | 音符：邊界晚，但節奏不飄；串流取樣：緩衝撐不住就 underrun |
| `notes` 指標 | 必須指向 `PROGMEM` 的陣列 | 不檢查：播出亂七八糟的音 |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 和弦、兩個聲部 | 音符一次只有一個音 | 轉換時挑主旋律；要原本的聲音就用 `--sound pcm` |
| 有源蜂鳴器播音符 | 只聽得到節奏（編譯警告） | 用 `--sound active` 轉成節奏 |
| 很短的音（< 40 ms） | `GAP_MS` 讓它一半時間安靜 | `GAP_MS` 調小，或 `--min-note` 調大 |
| 取樣和 `UnoTone`、D3 / D9 / D10 / D11 的 PWM（LED）一起用 | 搶 Timer1 / Timer2 | 一次只用一個；LED 改接 D5 / D6 |
| 串流取樣時程式做很久的事（OLED 的 `display()` 64 ms） | 緩衝用完：underrun，喀喀聲 | `BUFFER` 加大（8 kHz 8 bits：64 ms 要 512 B），或降低取樣率、改 ADPCM |
| 同時開 `pio device monitor`、Arduino 的 `Serial` | 搶同一個串口 | 串流時都不要開；`stream.py` 本身同時處理圖片、動畫、聲音 |
| 播放中直接動腳位（`digitalWrite`、`analogWrite`） | 播放器不知道，狀態對不上 | 只透過播放器控制 |
