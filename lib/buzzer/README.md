# buzzer

兩隻腳（+、-）的蜂鳴器，`namespace buzzer`。有源和無源兩種（差別見[有源和無源](#有源和無源)），都只負責「發聲」：各自提供一個 Sound，讓 [audio](../audio/README.md) 的 `audio::Player` 播嗶聲和旋律。只依賴一個 Pin 或 Tone（例如 `Uno::UnoOutputPin`、`Uno::UnoTone`），換主板只要換這兩個。

## 檔案

```
lib/buzzer/
├── buzzer.h               入口：兩個模組
└── module/
    ├── active_buzzer.h    ActiveBuzzer<Pin, ACTIVE_HIGH>：有源，只能開關
    └── passive_buzzer.h   PassiveBuzzer<Tone>：無源，任何音高
```

## 接線

| 蜂鳴器 | UNO |
|---|---|
| +（長腳，或標 + 的那邊） | 一隻數位腳：有源例如 D8；無源最好 D9 或 D10 |
| - | GND |

**直接接腳位**時，蜂鳴器只能從腳位拿電：

| 蜂鳴器 | 電流（5 V） | 接法 |
|---|---|---|
| 有源（5 V 型） | 約 20 ～ 30 mA | 可以直接接（短時間的嗶聲） |
| 無源，電磁式（線圈約 16 Ω） | 直接接會超過腳位的上限 | **串一顆 100 Ω 電阻**（約 30 mA，聲音小一點） |
| 無源，壓電式 | 幾 mA | 可以直接接 |

一隻腳建議 ≤ 20 mA、絕對上限 40 mA，整顆晶片 ≤ 200 mA。

**要更大聲、或蜂鳴器是 9 V / 12 V 的**：用 NPN 電晶體（例如 S8050、2N2222），不要直接接：

```
外部電源 + ── 蜂鳴器 + ── 蜂鳴器 - ── 電晶體 C
                │                │
                └── 二極體 ◀─────┘     （1N4148，電磁式才需要：反接在蜂鳴器兩端）
UNO 腳位 ── 1 kΩ ── 電晶體 B
外部電源 - ── 電晶體 E ── UNO GND（兩邊的 GND 一定要接在一起）
```

- 電晶體導通 = 響，所以 `ActiveBuzzer` 的 `ACTIVE_HIGH` 還是 `true`。
- 二極體吸收線圈斷電時的反向電壓，保護電晶體。

## 用法

```cpp
#include "Uno_pin.h"
#include "Uno_tone.h"
#include "buzzer.h"
#include "audio.h"

using Beeper = buzzer::ActiveBuzzer<Uno::UnoOutputPin<8>>;   // 有源 + 接 D8
using Speaker = buzzer::PassiveBuzzer<Uno::UnoTone<9>>;      // 無源 + 接 D9
audio::Player<Beeper> beeper;
audio::Player<Speaker> speaker;

void setup() {
    beeper.init();
    speaker.init();
    beeper.beeps(Beeper::HZ, 3, 100, 100);                   // 嗶三聲（有源不管 hz，只要 > 0）
    speaker.play<melody::ode_to_joy_semitone>();             // lib/audio/output 的旋律
}

void loop() {
    beeper.update();                                         // 每次 loop 都要呼叫，不會等
    speaker.update();
}
```

完整的例子：`src/week_4/test_3.cpp`（有源）、`src/week_4/test_4.cpp`（無源，會從腳位量實際頻率）。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Pin`（有源） | `init()`、`write(high)`、`nowMs()`、`readFlash16(address)` | `Uno::UnoOutputPin` |
| `Tone`（無源） | `init()`、`start(hz)`（播放中呼叫就換頻率）、`stop()`（腳位拉 LOW）、`realHz()`、`nowMs()`、`readFlash16(address)` | `Uno::UnoTone` |

兩個模組本身都是 `audio::Player` 要的 Sound（`init`、`start`、`stop`、`nowMs`、`readFlash16`）。

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 設定腳位，安靜 |
| `start(hz)` | 有源：開（hz 只要 > 0）；無源：送 hz 的方波，或在播放中換頻率 |
| `stop()` | 安靜；無源的腳位拉 LOW |
| `nowMs()`、`readFlash16()` | 轉給 Pin / Tone（給 `audio::Player`） |
| `ActiveBuzzer::HZ` | 2300：有源的音高不知道，用常見的值（給 `beep(HZ, ms)`、`currentHz()`） |
| `PassiveBuzzer::realHz()` | 回傳實際播出的頻率（計時器只能整數 tick），安靜時是 0 |

平常不直接呼叫這些，而是透過 `audio::Player` 的 `tone`、`beep`、`beeps`、`play`、`slide`（見 [audio](../audio/README.md#函式)）。

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Pin`（有源） | — | `Uno::UnoOutputPin<PIN>` |
| `ACTIVE_HIGH`（有源） | `true` | + 接腳位、- 接 GND（或用電晶體）時是 `true`；+ 接 5V、- 接腳位時改成 `false` |
| `Tone`（無源） | — | `Uno::UnoTone<PIN>`；D9 / D10 不佔 CPU，其他腳用中斷 |

## 有源和無源

| | 有源（active） | 無源（passive） |
|---|---|---|
| 裡面 | 有振盪器 | 只有線圈（電磁式）或壓電片 |
| 通 DC | 響，固定音高（多半約 2.3 kHz） | 不響，只「喀」一聲 |
| 板子要做的 | 開、關 | 送方波，頻率就是音高 |
| 能播的 | 嗶聲、節奏 | 音階、旋律、滑音 |
| 分辨 | 接 5V 會一直響；底部通常封起來、比較高；+ 腳比較長 | 接 5V 不響；底部看得到電路板 |
| 用 | `module/active_buzzer.h` + `Uno::UnoOutputPin` | `module/passive_buzzer.h` + `Uno::UnoTone` |

兩者只差「`start(hz)` 怎麼做」（有源：腳位 HIGH；無源：開計時器），播放的時序都在 `audio::Player`，所以放在同一個資料夾。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 腳位 | 0 ～ 19；D0 / D1 編譯警告 | 編譯錯誤 |
| 無源的 hz | 16 ～ 20000（`UnoTone`）；好聽的範圍約 200 ～ 5000，最大聲在共振頻率（常見 2 ～ 4 kHz） | 夾到範圍內 |
| 有源的 hz | 任何 > 0 都一樣 | — |
| 音量 | 不能調：方波固定 50%（最大聲） | — |
| 同時發幾個音 | 一個 Sound 一個音 | — |

## 資源

| 項目 | 有源 | 無源 |
|---|---|---|
| RAM | 0（模組只有 static 函式；`audio::Player` 約 28 bytes） | 同左 |
| 腳位 | 一隻 | 一隻 |
| 計時器 | 不用 | **Timer1**（整個） |
| 中斷 | 不用 | D9 / D10：不用；其他腳：Timer1 比較中斷，每次翻轉約 2 µs（20 kHz 約佔 8% CPU） |
| 同時用幾個 | 幾個都可以（各自一隻腳、一個 `Player`） | **一個**（只有一個 Timer1） |

## 注意事項

### 接線不能做的事

| 情況 | 結果 | 做法 |
|---|---|---|
| 蜂鳴器的 + 同時接腳位**和**外部電源（5V、9V、電池） | 外部電源灌進腳位，燒壞腳位或整顆晶片；腳位 LOW 時等於把電源短路到 GND | 一個蜂鳴器只接一個來源；要外部電源就用電晶體（見接線） |
| 蜂鳴器接在腳位和 9 V / 12 V 之間 | 腳位 HIGH 時也承受超過 5 V 的電壓，會燒壞 | 用電晶體，GND 共接 |
| 外部電源的 GND 沒跟 UNO 的 GND 接在一起 | 電晶體不會動，或亂響 | GND 一定要共接 |
| 電磁式無源直接接、不串電阻 | 電流超過腳位上限，腳位慢慢壞掉 | 串 100 Ω |
| 電磁式用電晶體推、沒有二極體 | 斷電時的反向電壓打壞電晶體 | 蜂鳴器兩端反接 1N4148 |
| 有源的 +、- 接反 | 不響（有極性） | 長腳是 + |
| 3 V 的蜂鳴器直接接 5 V 腳位 | 比較大聲，但會熱、壽命變短 | 串電阻，或換 5 V 型 |
| 接在 D0 / D1 | 串口和燒錄會干擾、蜂鳴器會亂響 | 換別的腳 |
| 接在 D13 / D11（這個專案的 OLED、M130T 用的腳） | 跟螢幕搶腳位 | 換別的腳 |

### 程式不能做的事

| 情況 | 結果 | 做法 |
|---|---|---|
| 對蜂鳴器的腳位 `analogWrite` / `pwmSet`（PWM） | 有源：振盪器每個週期都被打斷，聲音沙沙的、也不能調音量；無源：只有固定的 490 / 980 Hz，不能播音階 | 不要用 PWM：有源用 `ActiveBuzzer`，無源用 `PassiveBuzzer` |
| 在 D9 / D10 用 `analogWrite` / `pwmSet`（例如 `UnoST7789` 的 BLK 接 D9），同時用無源 | 兩邊互改 Timer1，頻率和亮度都錯 | BLK 改接 D3、D5、D6、D11，或不調亮度 |
| 同時用 Arduino 的 `tone()` / `noTone()`、`Servo` | 搶計時器、搶腳位 | 只用這個函式庫 |
| 播放中直接 `digitalWrite` 蜂鳴器的腳位 | `audio::Player` 不知道，狀態對不上 | 只透過 `Player` 控制 |
| 兩個無源同時發聲 | 只有一個 Timer1：後 `start` 的會改掉前一個的頻率和腳位 | 一次只用一個無源；或一個無源 + 幾個有源 |
| 用 `PassiveBuzzer` 推有源蜂鳴器 | 方波一直打斷它的振盪器，聲音很雜 | 有源用 `ActiveBuzzer` |
| 用 `ActiveBuzzer` 推無源蜂鳴器 | 只有開的瞬間「喀」一聲；電磁式長時間通 DC 會發熱 | 無源用 `PassiveBuzzer` |
| `Uno_tone.h` 被兩個 .cpp include | 中斷函式重複定義，連結錯誤 | 只在一個 .cpp 裡 include |
| 無源接 D9 / D10 以外的腳，同時用 `UnoOneWire`（DS18B20）或其他關中斷的程式 | 關中斷時方波暫停，聲音抖一下 | 無源接 D9 或 D10 |
| `update()` 很久才呼叫一次（例如 OLED 的 `display()` 64 ms） | 音的邊界晚，但節奏不飄；無源的方波照常（計時器在跑） | 正常；要準就少做長的事 |
| 板子進入睡眠（power-down） | Timer1 停止，無源不響 | 播放時不要睡 |
| 想調音量 | 方波固定 50% | 串電阻或可變電阻 |
