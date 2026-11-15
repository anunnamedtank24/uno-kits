# week_5

紅外線遙控器（[lib/NEC](../../lib/NEC/README.md)）的測試，以及三個用遙控器操作的作業：紅綠燈模組和一顆 LED（[lib/LED](../../lib/LED/README.md)）、OLED 上的方塊、五線譜和蜂鳴器。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python -B src/hide.py week_5/test_1     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `test_1.cpp` | 把遙控器送的碼印到串口，找出你的遙控器是哪一種 | 紅外線接收器 + 遙控器 |
| `assignment_1.cpp` | 作業一：數字鍵調紅、綠、藍（紅綠燈模組的紅和綠 + 一顆藍色 LED） | 接收器 + 紅綠燈模組 + LED |
| `assignment_2.cpp` | 作業二：方向鍵讓 OLED 上的小正方形移動 1 像素 | 接收器 + OLED |
| `assignment_3.cpp` | 作業三：在五線譜上寫二分音符，按 OK 播放 | 接收器 + OLED + 無源蜂鳴器 |

## 接線

| 零件 | 腳 | UNO | 哪些程式 |
|---|---|---|---|
| 紅外線接收器（HX1838 / VS1838B / KY-022） | OUT（S） | **D2** | 全部 |
| | GND（-） | GND | |
| | VCC（+） | 5V | |
| 紅綠燈模組（R、Y、G、GND） | R | **D3**（模組上有電阻，直接接） | `assignment_1` |
| | G | **D5** | |
| | Y | 不接 | |
| | GND | GND | |
| 藍色 LED（2 隻腳） | 長腳（+） | 220 Ω → **D6** | `assignment_1` |
| | 短腳（-） | GND | |
| OLED（128×64 I2C） | SCL | **D13** | `assignment_2`、`assignment_3` |
| | SDA | **D11** | |
| | VCC、GND | 5V、GND | |
| 無源蜂鳴器 | + | 100 Ω → **D9** | `assignment_3` |
| | - | GND | |
| USB | D0、D1 | 串口 | `test_1`、`assignment_1`（印出狀態） |

用到的腳：**D2、D3、D5、D6、D9、D11、D13**（加上串口的 D0、D1）。全部接著也不會衝突：

| 腳 | 為什麼選它 |
|---|---|
| D2 | 任何腳都能用腳位改變中斷；D2 跟其他零件不同組（D0 ～ D7 這組只有它用中斷） |
| D3、D5、D6 | PWM 腳裡不用 Timer1 的（D9、D10 留給蜂鳴器），D11 被 OLED 用了 |
| D9 | 無源蜂鳴器在 D9 由 Timer1 直接翻轉，不佔 CPU，`display()` 時聲音也不會斷 |
| D13、D11 | 跟 week_4 一樣的 OLED 接法 |

8 隻腳（SDA、SCK、MOSI、MISO、IRQ、GND、RST、3.3V）的那塊板子是 **RC522 RFID 讀卡模組**，不是紅外線接收器，這週用不到（見 [lib/NEC](../../lib/NEC/README.md#名稱)）。紅外線接收器只有 3 隻腳，前面一顆黑色圓頂：晶片本身圓頂面對自己、腳朝下時是 OUT、GND、VCC；焊在小板子上的（KY-022 等）照板子印的字接，順序常常不同。

# test_1.cpp

每收到一個碼就印一行：

```
infrared receiver on D2: press keys on the remote
address 0x00  command 0x45  press     numbers top: 1      arrows top: *
address 0x00  command 0x45  repeat 1  numbers top: 1      arrows top: *
```

- **先跑這個**：17 鍵遙控器有兩種排法、指令不同（見 [lib/NEC](../../lib/NEC/README.md#遙控器)）。按幾個鍵，`numbers top` 和 `arrows top` 哪一欄跟你按的一樣，就把三個作業開頭的 `using Remote = …` 改成那一個（現在是 `NEC::Remote17KeyNumbersTop`，另一種是 `NEC::Remote17KeyArrowsTop`）。
- **兩欄都不對**：把印出來的 `command` 抄下來，照 `lib/NEC/module/remote_17_key.h` 加一個遙控器。
- **按住**：先一行 `press`，接著每 108 ms 一行 `repeat`。
- **收不到時的診斷**：
  - 開機印 `D2 idle: HIGH (ok)` 或 `LOW`。沒收到光時應該是 HIGH；LOW 表示接收器沒電、腳接錯，或 OUT 沒接在 D2。
  - 每一串脈衝印成 `pulses 67: L9010 H4470 L580 …`（L = 有光，H = 沒光，單位 µs）。好的 NEC 按鍵開頭是 L 約 9000、H 約 4500。
  - 有變化卻解不出來時，每 2 秒印 `D2 changed N times, no NEC code`。按一下 NEC 的鍵約變化 68 次，只有個位數就是雜訊。
  - D2 開了內部上拉：接收器沒接好時，D2 是安靜的 HIGH，不會讀到懸空的雜訊。
- 串口監看：`pio device monitor`（1 Mbaud）。

# assignment_1.cpp：遙控器調紅、綠、藍

| 鍵 | 紅 R | 綠 G | 藍 B |
|---|---|---|---|
| -= 1 | 1 | 4 | 7 |
| = 0 | 2 | 5 | 8 |
| += 1 | 3 | 6 | 9 |

`0`：R、G、B 全部歸 0。

- **一格多大**：R、G、B 是 0 ～ `LEVELS`（10）的整數，所以 `+= 1` 按 10 次從關到最亮。用 0 ～ 255 的話一次 `+= 1` 根本看不出來。
- **看起來均勻**：每一格先換成眼睛的亮度（`LED::levelToBrightness`），再換成 PWM 的工作週期（`LED::toDuty`，平方）。直接用工作週期的話，前幾格變化很大、後面幾乎不變。
- **按住**：1、3、4、6、7、9 按住約 0.3 秒後每秒走約 9 格；2、5、8、0 按住不重複。
- 每次改變也會印到串口：`R 3  G 0  B 10`。

# assignment_2.cpp：方向鍵移動方塊

- 6×6 的實心正方形，一開始在中間。
- 上、下、左、右各移動 1 像素，碰到邊就停；`OK` 回到中間。
- **按住**：約 0.3 秒後每秒移動約 9 像素（NEC 的重複碼每 108 ms 一個）。
- **畫法**：擦掉舊的、畫新的，`display()` 只送改過的位元組（約 1 ms），不用整張重送。

# assignment_3.cpp：五線譜

高音譜記號、五線譜和 7 個二分音符；三角形指著正在寫的那一個。

| 鍵 | 動作 |
|---|---|
| 上 / 下 | 升高 / 降低一階（C D E F G A B），升降記號拿掉 |
| `*` | 降半音，音符前面畫 ♭；本來是 ♯ 的變回本位 |
| `#` | 升半音，音符前面畫 ♯；本來是 ♭ 的變回本位 |
| `OK` | 換下一個；在最後一個按 OK 就一起播放，播完三角形回到第一個，可以再改 |

- **音域**：「中央 C5 到 B4」我理解成**中央 C（C4，262 Hz）到 B4（494 Hz）**，也就是 C4 開始的一個八度。科學音高記號的中央 C 是 C4；C5 比 B4 高，不會是「到 B4」的起點。中央 C 在五線譜下面加一條短線，B4 在第三線。
- **一開始**：7 個都是中央 C。
- **♭、♯ 的極端**：C♭4 播 B3（247 Hz），B♯4 播 C5（523 Hz）。
- **播放**：`BPM` 120，二分音符 = 2 拍 = 1 秒；每個音最後 60 ms 安靜，兩個一樣的音才聽得出是兩個。播放時三角形跟著走，按鍵不理。
- **為什麼 7 個**：每個音符要留升降記號的位置（5 px）和符桿，16 px 一個剛好放 7 個；8 個時記號會貼到前一個音符的符桿。改 `NOTE_COUNT` 時放不下會編譯錯誤。
- **高音譜記號**：一張 15×40 的黑白圖（`picture::treble_clef_uno_15x40_palette_2`），用 macOS 字型 Apple Symbols 的 𝄞 畫的，漩渦對齊 G4 線（見 [lib/image/output](../../lib/image/output/README.md#目前的圖片)）。先畫它再畫五條線，線才會穿過去。
- **在電腦上看過畫面**：把畫五線譜的程式碼拿到電腦上編譯、畫進假的 128×64 螢幕，確認線、記號、音符、短線的位置。

## 資源

| 程式 | Flash | RAM |
|---|---|---|
| `test_1` | 2.6 KB | 390 B（按鍵名稱的字串） |
| `assignment_1` | 2.7 KB | 150 B |
| `assignment_2` | 3.6 KB | 1164 B（OLED 的畫面 1 KB） |
| `assignment_3` | 8.3 KB | 1277 B |

## 實測

- `test_1`：第一顆接收器接錯過、之後一直把 OUT 拉在 LOW（燈一直亮，D2 讀到 LOW）；換一顆之後就收得到了。
- 遙控器、LED、方塊、五線譜**還沒有在板子上用遙控器試過**（我沒辦法按遙控器）。解碼在電腦上測過：256 個指令、±15% 的時序誤差、按住、雜訊、斷掉的一半都解對（見 [lib/NEC](../../lib/NEC/README.md#nec-協定)）。
