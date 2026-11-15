# image/source

在電腦上跑的工具。需要 Python 3、`pip install pillow pyserial`。HEIC 可以另外裝 `pillow-heif`，沒裝的話 macOS 會用內建的 `sips`。都在 `lib/image` 裡執行：

| 檔案 | 作用 |
|---|---|
| `config.json` | 兩個目標（uno、mac）的參數範圍、預設值，以及畫圖時間的估計值 |
| `convert.py` | 圖片 → `output/*.h`，並把新產生的那個加進 `output/selected.h` |
| `check.py` | 檢查 `output/selected.h` |
| `stream.py` | 不用參數：自動找板子，板子要哪張圖就送哪張 |

```bash
cd lib/image
python -B source/convert.py assets/sunset.jpg output --target uno --fit 120x80 --scale 2 --palette 16
python -B source/check.py
python -B source/stream.py
```

`-B`：不產生 `__pycache__`。

## convert.py

### 檔案

| 副檔名 | 怎麼讀 |
|---|---|
| `.jpg`、`.jpeg`、`.png`、`.webp` | Pillow |
| `.heic`、`.heif` | `pillow-heif`（有裝的話）；沒有就用 macOS `sips` 先轉成暫存的 png，轉完刪掉 |

- **EXIF orientation**：照片會先轉正，跟相簿裡看到的方向一樣。
- **透明**：用 alpha 混合到 `--background` 上（預設 000000），半透明的邊緣也會混合，不是直接切掉。
- **其他副檔名**：報錯。

### 目標

`--target` 必填，可以給一個或兩個：

| | `--target uno` | `--target mac` | `--target uno mac` |
|---|---|---|---|
| 像素在哪 | `output/x.h`，燒進 Flash | `output/mac/x.h`，留在 Mac | `output/x.h` |
| `output/x.h` 裡有什麼 | 像素 + 路徑 | 只有尺寸、顏色、路徑 | 像素 + 路徑 |
| 顏色（`--palette`） | 2、16（預設）、256、565（警告） | 2、16、565（預設） | 2、16（預設）、565（警告） |
| 範圍 | uno 的 | mac 的 | 兩者都允許的（交集） |

### 尺寸（解析度）

所有尺寸都是指「螢幕上」的大小，也就是放大之後。跟 `lib/GIF/source/convert.py` 一樣：

| 參數 | 意思 |
|---|---|
| （都不給） | 圖片原本的大小（存的寬高都必須 ≤ 240，照片通常要給尺寸） |
| `--width W` 或 `--height H` | 只給一個：維持比例 |
| `--width W --height H` | 兩個都給：剛好這個大小（會拉伸） |
| `--resize F` / `--resize FXxFY` | 原尺寸的倍數，可以是小數，例如 `0.1` |
| `--fit WxH` | 維持比例，塞進 W×H 的框裡，盡量大 |
| `--fill WxH` | 維持比例，蓋滿 W×H，超出的部分從中間裁掉 |
| `--scale N` / `--scale NXxNY` | 板子把每個點重複畫 NX×NY 次（1 ～ 8）。資料只存「螢幕大小 ÷ scale」，也就是降低解析度換資料量 |
| `--resample` | 縮放方法：`nearest`（像素風）、`bilinear`、`bicubic`、`lanczos`（預設） |

- 螢幕上超過 240×240 只會警告：圖片會裁切，但多出來的資料是浪費的。
- 尺寸不是 scale 的整數倍時會四捨五入，並印警告說實際大小。

### 顏色

| 參數 | 意思 |
|---|---|
| `--palette 2` | 黑白，1 bit。灰階 ≥ `--threshold`（預設 128）是白；`--dither` 用 Floyd–Steinberg 抖色；`--invert` 黑白反過來 |
| `--palette 16` | 調色盤最多 16 色（median cut），4 bits |
| `--palette 256` | 調色盤最多 256 色，1 byte。只有 uno：調色盤直接從 Flash 讀 |
| `--palette 565` | 每個像素自己的 RGB565 顏色，2 bytes |
| `--colors N` | 調色盤只用 N 色（16 / 256） |
| `--dither` | 抖色（2 / 16 / 256）：漸層比較平滑，但 runs 變短 |
| `--background` | 透明的地方放在什麼顏色上，預設 000000 |

- 調色盤只留真的用到的顏色，重新編號成 0 ～ n − 1。

### 編碼和大小

| 參數 | 意思 |
|---|---|
| `--encoding auto` | raw 和 runs 都算，用比較小的（預設；一樣大時用 raw） |
| `--encoding raw` | 每個像素照原樣（1 / 4 / 8 / 16 bits） |
| `--encoding runs` | 連續同色的一段存成一筆 |
| `--max-bytes N` | 資料超過 N 就把尺寸乘 0.85 重做，直到放得下；小到 4 px 還放不下就報錯 |
| `--name` | 自訂檔名和 struct 名稱 |

- **預設檔名**：`{檔名}_{uno|mac|uno_mac}_{寬}x{高}_palette_{2|16|256|565}.h`，struct 同名。
- **自我檢查**：編碼完會用 Python 版的解碼器解回來比對，不一致就報錯，不會產生檔案。
- **錯誤**：超過硬限制（格式或板子做不到的）時，不產生檔案。
- **警告**：超出 `config.json` 的建議範圍。

### 流程

1. **讀檔**：依副檔名讀進來，轉正（EXIF），變成 RGBA。
2. **尺寸**：
   - 算出螢幕上的大小；`--fill` 先從中間裁掉多出的部分。
   - 除以 `--scale` 得到要存的大小，用 `--resample` 縮放，再混合到背景色上。
3. **顏色**：
   - 565：每個像素直接轉 RGB565。
   - 2：轉灰階，再用門檻或抖色變成黑白。
   - 16 / 256：median cut 做調色盤，可以加抖色。
4. **編碼**：raw 和 runs，依 `--encoding` 選。
5. **估計時間**：每段（`SEGMENT` = 32 個螢幕像素）約 900 週期，每個螢幕像素約 60 週期，每 byte 讀取 uno 6 週期、mac 176 週期（1 Mbaud），數值在 `config.json`。

產生的檔案格式見 [output/README.md](../output/README.md)。

## check.py

跟 `lib/GIF/source/check.py` 一樣的規則，只是 namespace 是 `picture`、看的是 `getPixels()`：

| 類型 | 項目 |
|---|---|
| 錯誤 | `selected.h` 不存在或沒有 include guard |
| 錯誤 | include 的檔案不存在、不在 `output/` 裡、include 自己、重複 include |
| 錯誤 | 檔案的 `// format:` 不是目前的 `FORMAT_VERSION`（1） |
| 錯誤 | 檔案裡沒有 `struct picture::{檔名}` |
| 錯誤 | 原始圖片跟產生時不一樣了（比對 sha256） |
| 錯誤 | mac 描述檔指到的 `output/mac/` 像素檔不存在、格式不對或過時 |
| 警告 | 原始圖片找不到 |
| 警告 | `output/` 裡有沒被選到的 .h |

## stream.py

- **不用參數**：自動找 `/dev/cu.usbmodem*` 等串口，沒插板子就等，拔掉重插也會自己接回。
- **選用參數**：`--port`、`--baud`（預設讀 `config.json`）。
- **安全**：只開 `lib/image` 裡面的 `.h`。
- **燒錄時自動放開串口**：跟 GIF 的一樣，每 0.3 秒檢查有沒有 avrdude 或 `pio run -t upload` 在跑。
- **打開串口會讓板子重置**：建議先開 `stream.py`，再燒錄（詳見 [GIF/source/README.md](../../GIF/source/README.md#streampy)）。
- **跟 GIF 的 `stream.py` 二選一**：兩個都要用同一個串口。
- **檢查檔案**：讀進來時會把像素解碼一次，壞掉的檔案直接回答錯誤，不會送到一半。
- **每張都印出來**：送了幾個 byte、花幾秒。

### 串流協定

跟 GIF 一樣的原則：**Mac 主動送的資料永遠不超過 1 byte**（UNO 的 UART 只能暫存 2 個 byte）。

| 方向 | 內容 |
|---|---|
| 板子 → Mac | `'I'`、CHUNK、路徑長度、路徑 |
| Mac → 板子 | `'D'`（回答準備好了） |
| 板子 → Mac | `'R'`（要下一塊），每塊都要一次 |
| Mac → 板子 | `[長度 1 ～ CHUNK][資料]` |

回答（被一塊一塊拉走）：

| 回答 | 內容 |
|---|---|
| `'H'` 圖片 | 寬、高、scaleX、scaleY、顏色模式、編碼、色數、資料長度（uint32）、調色盤（色數 × RGB565，只有 2 / 16 色）、像素 |
| `'E'` 錯誤 | 代碼（1 找不到、2 讀不懂、3 256 色不能串流）、訊息長度、訊息 |

板子邊收邊畫，所以只需要 CHUNK 大小的緩衝。

| 情況 | 板子的處理 |
|---|---|
| 3 秒沒等到 `'D'` | `ERROR_NO_ANSWER` |
| 一塊 200 ms 沒收完 | 剩下的像素畫成黑色，`ERROR_BROKEN` |
| 收到不合理的標頭 | `ERROR_BROKEN` |
