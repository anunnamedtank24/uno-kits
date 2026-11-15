# audio

旋律和播放器，`namespace audio`（播放器、音高）和 `namespace melody`（產生的旋律）。旋律由 `source/convert.py` 從音檔（wav、flac、mp3、m4a、ogg、webm…）轉成 `{hz, ms}` 的陣列，存在 Flash；播放器透過一個 Sound 發聲，例如 [buzzer](../buzzer/README.md) 的有源或無源蜂鳴器。不依賴任何硬體。

## 檔案

```
lib/audio/
├── audio.h       入口：note.h + player.h + output/selected.h
├── note.h        noteHz(A, 4) = 440、Pitch、REST、beatMs：音高和旋律格式
├── player.h      Player<Sound, GAP_MS>：tone / beep / beeps / play / slide，不會等
├── assets/       範例音檔（7 種格式）
├── source/       convert.py（音檔 → .h）、check.py，見 source/README.md
└── output/       產生的旋律和 selected.h，見 output/README.md
```

## 用法

```bash
cd lib/audio
python -B source/convert.py assets/ode_to_joy.flac output --preview ode.wav
```

```cpp
#include "Uno_tone.h"
#include "module/passive_buzzer.h"
#include "audio.h"

using Speaker = buzzer::PassiveBuzzer<Uno::UnoTone<9>>;   // 無源蜂鳴器 + 接 D9
audio::Player<Speaker> speaker;

void setup() {
    speaker.init();
    speaker.play<melody::ode_to_joy_semitone>();          // output/ 的旋律
}

void loop() {
    speaker.update();                                     // 每次 loop 都要呼叫，不會等
}
```

完整的例子：`src/week_4/test_3.cpp`（有源）、`src/week_4/test_4.cpp`（無源）。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Sound` | `init()`、`start(hz)`（hz > 0；再呼叫一次就換頻率，只有一個音高的東西就是「開」）、`stop()`、`nowMs()`、`readFlash16(address)` | `buzzer::ActiveBuzzer`、`buzzer::PassiveBuzzer` |
| `Melody`（`play<Melody>`） | `static constexpr uint16_t count`、`static const uint16_t *getNotes()` | `melody::*`（`output/selected.h`） |

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 初始化 Sound，安靜 |
| `update()` | 時間到了就換下一個音或停止，不會等；每次 loop 都要呼叫 |
| `tone(hz)` | 一直響，直到 `stop()` 或下一個聲音 |
| `beep(hz, ms)` | 響一次 |
| `beeps(hz, count, onMs, offMs)` | 響 count 次，間隔 offMs（最後一次後面不再等） |
| `play(notes, count, repeat)` | 播 Flash 裡的 `{hz, ms, ...}`；repeat：播完從頭 |
| `play<Melody>(repeat)` | 播 `output/` 的旋律，例如 `play<melody::scale_semitone>()` |
| `slide(fromHz, toHz, ms)` | 滑音（以 Hz 直線變化）：警笛、雷射 |
| `stop()` | 立刻安靜 |
| `isPlaying()` | 回傳還有沒有東西在播（`tone()` 和旋律裡的休止符也算） |
| `currentHz()` | 回傳現在的頻率，0 = 安靜 |
| `noteIndex()` | 回傳旋律（或 beeps）現在播到第幾個（從 0 開始） |
| `noteHz(pitch, octave)` | 回傳音高的頻率：十二平均律、A4 = 440 Hz，`CS` 是 C#；C4 是中央 C（262 Hz） |
| `beatMs(bpm, beats, divide)` | 回傳音符長度：`beatMs(120, 1, 2)` = 每分鐘 120 拍時的半拍 = 250 ms |

- **一次一個聲音**：開始新的就取代舊的。
- **節奏不會飄**：下一個音從「上一個音應該結束的時間」算起，`update()` 晚一點呼叫，也不會讓整首變慢。
- `noteHz`、`beatMs` 都是 `constexpr`，寫在旋律裡不佔執行時間。

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Sound` | — | 發聲的 struct（見介面需求） |
| `GAP_MS` | 20 | 旋律每個音的最後這麼多 ms 安靜，連續兩個相同的音才聽得出是兩個（短於 2 × GAP_MS 的音不切） |

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
| 旋律的音數 | 1 ～ 65535（實際上受 Flash 限制，一個音 4 bytes） | `nullptr` 或 0：安靜 |
| `slide` 的 hz、ms | 都 > 0 | 安靜 |
| `noteHz` 的八度 | 0 ～ 8 | 回傳 0（休止） |
| `update()` 的間隔 | 越常越好 | 太久時音的邊界跟著晚（例如 OLED 的 `display()` 64 ms），但節奏不飄 |
| `notes` 指標 | 必須指向 `PROGMEM` 的陣列 | 不檢查：播出亂七八糟的音 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `Player` 約 28 bytes（狀態、時間、旋律指標） |
| Flash | 旋律每個音 4 bytes；沒播的旋律不佔 Flash |
| 時間 | `update()` 只比較時間，幾微秒 |
| 硬體 | Sound 的（見 [buzzer](../buzzer/README.md#資源)） |

## 注意事項

| 情況 | 結果 | 做法 |
|---|---|---|
| 和弦、兩個聲部 | 一個 Sound 一次只發一個音 | 轉換時挑主旋律；兩個聲部要兩個 Sound（無源只能一個，見 buzzer） |
| 有源蜂鳴器播旋律 | 只聽得到節奏，音高都是它自己的 | 用無源；或用節奏型的旋律（`melody::sos_semitone`） |
| 很短的音（< 40 ms） | `GAP_MS` 讓它一半時間安靜 | `GAP_MS` 調小，或轉換時 `--min-note` 調大 |
| 播放中直接動 Sound 的腳位（`digitalWrite`、`analogWrite`） | `Player` 不知道，狀態對不上 | 只透過 `Player` 控制 |
