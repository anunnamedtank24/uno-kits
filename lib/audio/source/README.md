# audio/source

在電腦上跑的工具。需要 Python 3、`pip install numpy`，以及 `brew install ffmpeg`（解碼音檔；沒有時 macOS 的 `afconvert` 也能讀大部分格式）。都在 `lib/audio` 裡執行，加 `-B` 不產生 `__pycache__`。

## 檔案

| 檔案 | 作用 |
|---|---|
| `convert.py` | 音檔 → `output/*.h`，並把新產生的那個加進 `output/selected.h` |
| `check.py` | 檢查 `output/selected.h` |

## 用法

```bash
cd lib/audio
python -B source/convert.py assets/twinkle_star.wav output
python -B source/convert.py ~/Music/song.mp3 output --start 12.5 --duration 8 --preview song.wav
python -B source/check.py
```

## convert.py

### 支援的檔案

| 格式 | 副檔名 | 怎麼解碼 |
|---|---|---|
| 無損 | `.wav` `.flac` `.aiff` `.aif` `.caf` | ffmpeg；沒有 ffmpeg 時用 macOS 的 `afconvert`；`.wav` 連兩個都沒有也能讀（整數 PCM） |
| 有損 | `.mp3` `.aac` `.m4a` `.m4r`（鈴聲）`.mp4` `.mov` | ffmpeg 或 `afconvert` |
| 有損 | `.ogg` `.oga` `.opus` `.webm` | 只有 ffmpeg |

- 立體聲會混成單聲道；影片檔只取聲音。
- 實測：同一段音階存成上面每一種格式，轉出來的 15 個音都一樣。

### 流程

蜂鳴器一次只能發一個音，所以只找「每一刻最明顯的那個音高」：

1. **解碼**：轉成單聲道 22050 Hz。
2. **音高**：每 10 ms 一格，用 YIN（de Cheveigné & Kawahara, 2002）找基頻，抓得到 43 Hz ～ 5 kHz；不夠明確的格子當作沒有音高。
3. **平滑**：每格取前後 5 格的中位數，去掉偶爾跳八度的錯。
4. **安靜**：比最大聲小 `--silence` dB 以上的格子是休止。
5. **切音**：音高（四捨五入到半音）改變就是新的音；同一個音高但音量在 40 ms 內突然升高 `--onset` dB，也是新的音（連續兩個相同的音）。
6. **整理**：
   - 短於 `--min-note` 的音，併進比較長的鄰居。
   - 短於 `--min-rest` 的休止，給前一個音。
   - 頭尾的休止去掉。
7. **音高**：`--transpose` 移調，超出 `--range` 的音以八度移進範圍。
8. **時間**：除以 `--speed`，每個音的長度用「四捨五入後的起點、終點」相減，所以整首的總長不會因為四捨五入而飄掉。

- **和弦、有伴奏的歌**：得到的是每一刻最強的那條線，不一定是主旋律。挑一段主旋律單獨或最大聲的地方（`--start`、`--duration`）。
- **試聽**：`--preview x.wav` 把結果合成成無源蜂鳴器的方波（含 `GAP_MS` 20 ms 的間隔），在 Mac 上就能聽，不用燒錄。

### 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `--name` | `{檔名}_{semitone\|raw}` | 檔名和 struct 名稱 |
| `--preview F.wav` | — | 另外存一個試聽用的方波 wav |
| `--start S` | 0 | 從第幾秒開始 |
| `--duration S` | 到結尾 | 取幾秒 |
| `--speed F` | 1 | > 1 變快、< 1 變慢，音高不變；0.1 ～ 10 |
| `--min-note MS` | 60 | 更短的音併進鄰居 |
| `--min-rest MS` | 40 | 更短的休止給前一個音 |
| `--quantize` | `semitone` | `semitone`：對齊到最近的半音，聽起來準；`raw`：量到的頻率（每個音的中位數） |
| `--transpose N` | 0 | 移調幾個半音，+ 升、- 降 |
| `--range LOW HIGH` | 200 4000 | Hz；範圍外的音移八度進來。小蜂鳴器低於 200 Hz 很小聲、高於 4 kHz 很刺耳。至少要一個八度寬 |
| `--silence DB` | 35 | 比最大聲小這麼多就是休止；音檔很安靜或有雜音時調整 |
| `--voicing F` | 0.2 | YIN 的門檻，0.05 ～ 0.5：小 = 只收很乾淨的音，大 = 雜一點的也收 |
| `--onset DB` | 6 | 音量突然升高多少 dB 算新的音；0 = 不用（同音高一律連成一個） |

### 錯誤和警告

| 情況 | 處理 |
|---|---|
| 檔案不存在、副檔名不支援、參數超出範圍 | 錯誤，不產生檔案 |
| 解碼失敗、少於 46 ms、一個音都找不到 | 錯誤，並提示可以調哪個參數 |
| 超過 65535 個音，或超過 UNO 的 Flash（32256 bytes） | 錯誤：用 `--start` / `--duration` 剪短 |
| 超過 8192 bytes | 警告：留給程式的 Flash 不多 |
| 有短於 40 ms 的音 | 警告：`GAP_MS` 20 會讓它一半時間是安靜的 |
| `--range` 低於 100 Hz 或高於 8 kHz | 警告 |
| 輸出的資料夾不是 `lib/audio/output` | 警告：`audio.h` 只 include `output/selected.h` |

## check.py

- `selected.h` 存在，而且有 include guard。
- 每個 `#include` 都是 `output/` 裡的檔案，只列一次。
- 每個檔案都是目前的格式（`// format: 1`），有 `struct melody::{檔名}` 和 `getNotes()`。
- 原始音檔（記錄的路徑和 sha256）沒有變過；找不到原始檔時只警告。
- `output/` 裡沒被選的 .h 只警告。
