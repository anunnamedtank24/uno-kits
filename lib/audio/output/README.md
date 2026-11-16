# audio/output

`source/convert.py` 產生的聲音，`namespace melody`。除了 `selected.h` 可以手動改，其他檔案都不要手動編輯，要改就重新轉換。

## 檔案

```
output/
├── selected.h    程式裡能用的聲音，audio.h 會 include 它
├── *.h           產生的聲音（--target mac 的只有長度和路徑）
└── mac/          --target mac 的資料，只給 stream.py 讀，不會被 include
```

## selected.h

- `convert.py` 每次只把剛產生的那個加進來，其他的保持原樣。
- 不用的聲音刪掉那行 `#include` 就好；就算留著，程式沒播它也不佔 Flash。
- `python -B source/check.py` 檢查每個 include 都是目前格式、原始音檔沒變過。

## 目前的聲音

| 檔案 | 來源 | 格式 | 內容 | 資料 |
|---|---|---|---|---|
| `twinkle_star_uno_passive_medium.h` | twinkle_star.wav | 音符 Hz + ms | 小星星第一句（法國民謠，公有領域），14 個音，C5 ～ A5，6.4 s | 56 B |
| `twinkle_star_uno_passive_low.h` | twinkle_star.wav | 音符 compact | 同上，一半大小 | 28 B |
| `twinkle_star_uno_active_medium.h` | twinkle_star.wav | 節奏 | 同上的節奏（有源蜂鳴器），音之間留 30 ms | 54 B |
| `twinkle_star_mac_passive_medium.h` | twinkle_star.wav | 音符 Hz + ms，Mac | 串流用（資料在 `mac/`） | 56 B |
| `ode_to_joy_uno_passive_medium.h` | ode_to_joy.flac | 音符 Hz + ms | 歡樂頌第一句（貝多芬，公有領域），14 個音，6.9 s | 56 B |
| `ode_to_joy_mac_pcm_16000_u8.h` | ode_to_joy.flac | 取樣 16 kHz 8 bits，Mac | 同一段的真正波形，7.5 s | 119 KB（在 `mac/`） |
| `scale_uno_passive_medium.h` | scale.aiff | 音符 Hz + ms | C 大調音階 C5 → C6 → C5，檢查音準用 | 60 B |
| `scale_uno_passive_high.h` | scale.aiff | 音符，量到的頻率 | 同上，頻率不對齊半音（例如 1048 Hz） | 60 B |
| `startup_uno_passive_medium.h` | startup.mp3 | 音符 | 開機：C E G C 往上，0.5 s | 16 B |
| `success_uno_passive_medium.h` | success.m4a | 音符 | 成功：往上四度，0.3 s | 8 B |
| `error_uno_passive_medium.h` | error.ogg | 音符 | 錯誤：兩個往下的低音，0.6 s | 8 B |
| `sos_uno_active_medium.h` | sos.webm | 節奏 | 摩斯碼 SOS（... --- ...），3.3 s | 34 B |
| `hello_uno_pcm_8000_adpcm.h` | hello.aiff | 取樣 8 kHz ADPCM | macOS `say` 的人聲 "Hello! I am an Arduino."，2.5 s | 10 KB |

`assets/` 的音檔：前七個是用 numpy 合成的（基音加泛音、起音和衰減、一點雜音、立體聲），再用 ffmpeg 存成七種格式；hello.aiff 是 `say -o assets/hello.aiff "Hello! I am an Arduino."`。

## 產生的 struct

```cpp
namespace melody {
    struct ode_to_joy_uno_passive_medium {
        static constexpr uint8_t target;       // audio::TARGET_UNO（1）、TARGET_MAC（2），或兩個
        static constexpr uint8_t kind;         // audio::KIND_NOTES、KIND_RHYTHM、KIND_PCM
        static constexpr uint8_t encoding;     // audio::NOTES_HZ_MS … PCM_ADPCM（lib/audio/format.h）
        static constexpr uint32_t count;       // 步數（音符、節奏）或取樣數
        static constexpr uint32_t durationMs;
        static constexpr uint16_t rate;        // 取樣率（只有取樣）
        static constexpr uint16_t rhythmHz;    // 節奏在無源蜂鳴器上的音高
        static constexpr uint32_t dataSize;
        static inline const char *getPath();          // stream.py 找資料的路徑，相對於 lib/audio
        static inline const uint16_t *getNotes();     // 音符、節奏（--target uno）
        static inline const uint8_t *getSamples();    // 取樣（--target uno）
    };
}
```

**檔案開頭**：`// format: 2`（格式版本）、`// source: … sha256 …`（原始音檔和它的 hash）、`// settings:`（所有設定和結果：target、sound、quality、編碼、剪的範圍、偵測參數、取樣率、訊號 / 誤差、資料量）。

## 格式（format 2）

每個編碼的位元組見 [lib/audio/format.h](../format.h) 的開頭：

| 編碼 | 每步 / 取樣 | 內容 |
|---|---|---|
| `NOTES_HZ_MS`（0） | 2 個 uint16 | Hz（0 = 休止）、ms；跟 `note.h` 手寫旋律的格式一樣 |
| `NOTES_COMPACT`（1） | 1 個 uint16 | 高 8 位元 MIDI 音（0 = 休止、12 ～ 119），低 8 位元 10 ms 的格數（1 ～ 255） |
| `RHYTHM_MS`（2） | 1 個 uint16 | 第 15 位元 = 響，第 0 ～ 14 位元 = ms（1 ～ 32767） |
| `PCM_U8`（3） | 1 byte | 0 ～ 255，128 = 安靜 |
| `PCM_U4`（4） | 半個 byte | 低 4 位元先；0 ～ 15 → 0 ～ 255（× 17） |
| `PCM_ADPCM`（5） | 半個 byte | IMA ADPCM，低 4 位元先，從 0、index 0 開始 |

- **太長的一步**拆成好幾步（同一個音再來一次）：Hz + ms 每步最多 65535 ms，compact 2550 ms，節奏 32767 ms。
- **format 1**（只有 `{hz, ms}`、名稱是 `*_semitone`）已經不用了：`check.py` 會要求重新轉換。
