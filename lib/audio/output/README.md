# audio/output

`source/convert.py` 產生的旋律，`namespace melody`。除了 `selected.h` 可以手動改，其他檔案都不要手動編輯，要改就重新轉換。

## 檔案

```
output/
├── selected.h    程式裡能用的旋律，audio.h 會 include 它
└── *.h           產生的旋律
```

## selected.h

- `convert.py` 每次只把剛產生的那個加進來，其他的保持原樣。
- 不用的旋律刪掉那行 `#include` 就好；就算留著，程式沒播它也不佔 Flash。
- `python -B source/check.py` 檢查每個 include 都是目前格式、原始音檔沒變過。

## 目前的旋律

| 檔案 | 來源 | 內容 | 音 | 長度 | 資料 |
|---|---|---|---|---|---|
| `twinkle_star_semitone.h` | twinkle_star.wav | 小星星第一句（法國民謠，公有領域） | 14，C5 ～ A5 | 6.4 s | 56 B |
| `ode_to_joy_semitone.h` | ode_to_joy.flac | 歡樂頌第一句（貝多芬，公有領域） | 14，C5 ～ G5 | 6.9 s | 56 B |
| `scale_semitone.h` | scale.aiff | C 大調音階 C5 → C6 → C5，檢查音準用 | 15，C5 ～ C6 | 4.3 s | 60 B |
| `startup_semitone.h` | startup.mp3 | 開機：C E G C 往上 | 4 | 0.5 s | 16 B |
| `success_semitone.h` | success.m4a | 成功：往上四度 | 2 | 0.3 s | 8 B |
| `error_semitone.h` | error.ogg | 錯誤：兩個往下的低音 | 2 | 0.6 s | 8 B |
| `sos_semitone.h` | sos.webm | 摩斯碼 SOS（... --- ...），只有節奏，給有源蜂鳴器 | 9 + 8 休止，A5 | 3.3 s | 68 B |

`assets/` 的音檔是用 numpy 合成的（基音加泛音、起音和衰減、一點雜音、立體聲），再用 ffmpeg 存成七種格式，每種格式都測得到。轉出來的音高全部正確，長度誤差約 ±10 ms（每格 10 ms）；最後一個音會長一點，因為包含了尾音的衰減。

## 產生的 struct

```cpp
namespace melody {
    // player.play<melody::twinkle_star_semitone>()
    struct twinkle_star_semitone {
        static constexpr uint16_t count;        // 幾個音（休止也算一個）
        static constexpr uint32_t durationMs;   // 整首多長
        static inline const uint16_t *getNotes();   // Flash 裡的 { hz, ms, hz, ms, ... }，hz 0 = 休止
    };
}
```

**檔案開頭**：
- `// format: 1`：格式版本。
- `// source: … sha256 …`：原始音檔和它的 hash。
- `// settings:`：所有設定，包括原本的指令、剪的範圍、偵測參數、移了幾個八度、音數、音域、資料量。

## 格式（format 1）

`uint16_t` 的陣列，兩個一組：

| 位置 | 內容 |
|---|---|
| `[2i]` | 頻率 Hz，16 ～ 20000；0 = 休止 |
| `[2i + 1]` | 長度 ms，1 ～ 65535（更長的音拆成好幾個） |

跟 `note.h` 手寫旋律的格式一樣，`audio::Player::play` 兩種都能播。
