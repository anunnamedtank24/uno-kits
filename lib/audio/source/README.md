# audio/source

在電腦上跑的工具。需要 Python 3、`pip install numpy pyserial`，以及 `brew install ffmpeg`（解碼音檔；沒有時 macOS 的 `afconvert` 也能讀大部分格式）。都在 `lib/audio` 裡執行。

## 檔案

| 檔案 | 作用 |
|---|---|
| `convert.py` | 音檔 → `output/*.h`（`--target stream` 的資料在同一個檔案的 `#if 0` 裡），並把新產生的那個加進 `output/selected.h` |
| `check.py` | 檢查 `output/` 的每個檔案：目前的格式、原始檔沒變（`convert.py` 每次轉完會自動跑） |
| `stream_handler.py` | [lib/stream](../../stream/README.md) 的 `stream.py` 用它讀聲音、回答板子的 `'A'`（`StreamPlayer`、`StreamSamplePlayer`）；不單獨執行 |

## 用法

```bash
cd lib/audio
python source/convert.py assets/twinkle_star.wav output
python source/convert.py ~/Music/song.mp3 output --start 12.5 --duration 8 --preview song.wav
python source/convert.py assets/sos.webm output --sound active
python source/convert.py assets/hello.aiff output --sound pcm --quality low --preview hello.wav
python source/convert.py assets/ode_to_joy.flac output --target stream --sound pcm --quality high
python source/check.py
cd ../.. && python lib/stream/source/stream.py     # 串流：圖片、動畫、聲音都是它
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
- **試聽**：`--preview x.wav` 把結果合成成無源蜂鳴器的方波（含 `GAP_MS` 20 ms 的間隔），在電腦上就能聽，不用燒錄。

### 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `--target flash / stream` | `flash` | `flash`：資料燒進 Flash；`stream`：資料留在電腦（`output/x.h` 的 `#if 0` 裡），`stream.py` 串流；兩個都寫：Flash 有資料，也能串流 |
| `--sound passive / active / pcm` | `passive` | 誰播：無源蜂鳴器（音符）、有源蜂鳴器（節奏）、取樣（真正的波形） |
| `--quality low / medium / high` | `medium` | 預設組合，見 [audio 的格式和品質](../README.md#格式和品質)；下面的單一參數可以蓋過它 |
| `--name` | `{檔名}_{target}_{sound}_{quality}`；取樣：`{檔名}_{target}_pcm_{rate}_{encoding}`（`assets/` 外面的檔案前面加上資料夾；撞名，例如只差 `--transpose`，加 `_2`、`_3`，不蓋掉別的；見 [output/README.md](../output/README.md#名稱和來源)） | 檔名和 struct 名稱 |
| `--preview F.wav` | — | 存一個試聽用的 wav：音符、節奏是方波；取樣是板子實際播的值 |
| `--start S`、`--duration S` | 0、到結尾 | 取哪一段（秒） |
| `--speed F` | 1 | > 1 變快、< 1 變慢，音高不變；0.1 ～ 10（取樣要 ffmpeg） |
| `--min-note MS`、`--min-rest MS` | 看 `--quality` | 更短的音併進鄰居、更短的休止給前一個音（音符、節奏） |
| `--quantize semitone / raw` | 看 `--quality` | 對齊半音，或量到的頻率（音符；`--quality low` 只能 semitone） |
| `--transpose N` | 0 | 移調幾個半音（音符） |
| `--range LOW HIGH` | 200 4000 | Hz；範圍外的音移八度進來，至少一個八度寬（音符） |
| `--hz` | 量到的音高中位數，沒有就 2000 | 節奏在無源蜂鳴器上播的音高（節奏） |
| `--rate HZ` | 看 `--quality` | 取樣率 2000 ～ 32000；轉換時用板子實際做得到的頻率（`Uno::UnoPcmOut` 的整數 tick） |
| `--encoding u8 / u4 / adpcm` | 看 `--quality` | 每個取樣 8 bits、4 bits、IMA ADPCM 4 bits |
| `--gain DB` | 0 | 最大的取樣拉到滿格之後再加幾 dB；> 0 比較大聲，但峰值會被切掉 |
| `--silence DB` | 35 | 比最大聲小這麼多就是休止（音符、節奏） |
| `--voicing F` | 0.2 | YIN 的門檻，0.05 ～ 0.5（音符、節奏） |
| `--onset DB` | 6 | 音量突然升高多少 dB 算新的音；0 = 不用 |

- 不屬於這個 `--sound` 的參數（例如 `--rate` 配 `--sound passive`）是錯誤。

### 取樣的流程

1. **解碼**：ffmpeg 直接轉成單聲道、板子實際的取樣率（8000 → 8000 Hz、11025 → 11050 Hz、22050 → 21978 Hz）；`--speed` 用 ffmpeg 的 atempo（音高不變）。
2. **音量**：最大的取樣拉到 0.98，再加 `--gain`。
3. **編碼**：u8、u4，或 IMA ADPCM。ADPCM 的編碼器邊編邊用板子的解碼法解回來，所以誤差不會累積；最後再整段解一次確認跟板子一樣。
4. **報告**：訊號 / 誤差（dB）、每秒幾個 byte、切掉的比例。

### 錯誤和警告

| 情況 | 處理 |
|---|---|
| 檔案不存在、副檔名不支援、參數超出範圍 | 錯誤，不產生檔案 |
| 解碼失敗、少於 46 ms、一個音都找不到 | 錯誤，並提示可以調哪個參數 |
| `--target flash` 時超過 65535 步，或超過 UNO 的 Flash（32256 bytes） | 錯誤：剪短、降低 `--quality`，或 `--target stream` |
| `--target flash` 時音符、節奏超過 8192 bytes，取樣超過 20000 bytes | 警告：留給程式的 Flash 不多 |
| `--target stream` 的取樣每秒超過 12000 bytes | 警告：串流可能跟不上（實測 16000 在邊緣；`underruns()` 會算） |
| 取樣率高於 22050 / 低於 4000 | 警告：中斷佔很多 CPU / 聲音很悶 |
| 超過 1% 的取樣被切掉 | 警告：`--gain` 調小 |
| 有短於 40 ms 的音 | 警告：`GAP_MS` 20 會讓它一半時間是安靜的 |
| `--range` 低於 100 Hz 或高於 8 kHz | 警告 |
| 輸出的資料夾不是 `lib/audio/output` | 警告：`audio.h` 只 include `output/selected.h` |

## check.py

- `selected.h` 存在，而且有 include guard。
- 每個 `#include` 都是 `output/` 裡的檔案，只列一次。
- 每個檔案都是目前的格式（`// format: 2`），有 `struct melody::{檔名}`、`getPath()`；給 UNO 的（`--target flash`）還要有資料：`getNotes()`（音符、節奏）或 `getSamples()`（取樣）。
- 原始音檔（記錄的路徑和 sha256）沒有變過；找不到原始檔時只警告。
- `output/` 裡沒被選的 .h 只警告。

## stream_handler.py

[lib/stream](../../stream/README.md) 的 `stream.py`（找板子、燒錄時放開串口、印出板子的 `'M'` 訊息）收到 `'A'` 時交給它：

- **板子說要哪個檔**：路徑是 struct 的 `getPath()`（相對於 `lib/audio`），只開 `lib/audio` 裡的 .h；`--target flash` 和 `--target stream`（資料在 `#if 0` 裡）的都可以串流。
- **檔案改了會重讀**（看修改時間），不用重開。
- **印出**：每個聲音的格式、大小，傳完的時間和速度。
- 協定見 [audio 的串流](../INTERFACE.md#串流)。
