# buzzer

| 需要 | 得到 |
|---|---|
| 有源：一個 `Pin`：`init()`、`write(high)`…（`Uno::UnoOutputPin<PIN>`） | `buzzer::ActiveBuzzer2Pins`、`ActiveBuzzer3Pins`：開、關（音高固定） |
| 無源：一個 `Tone`：`init()`、`start(hz)`、`stop()`…（`Uno::UnoTone<PIN>`） | `buzzer::PassiveBuzzer2Pins`、`PassiveBuzzer3Pins`：任意頻率、音量 |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

兩隻腳（+、-）的蜂鳴器，`namespace buzzer`。有源和無源兩種（差別見[有源和無源](#有源和無源)），都只負責「發聲」：每個模組都是 [audio](../audio/README.md) 的 `audio::Player` 要的 Sound，由它播嗶聲和旋律。

## 用法

```cpp
#include "Uno_pin.h"
#include "Uno_tone.h"
#include "buzzer.h"
#include "audio.h"

constexpr uint8_t BEEPER_PIN = 8;                                         // 有源的 +
constexpr uint8_t SPEAKER_PIN = 9;                                        // 無源的 +
using Beeper = buzzer::ActiveBuzzer2Pins<Uno::UnoOutputPin<BEEPER_PIN>>;  // 有源、兩隻腳
using Speaker = buzzer::PassiveBuzzer2Pins<Uno::UnoTone<SPEAKER_PIN>>;    // 無源、兩隻腳
// 三隻腳模組：buzzer::ActiveBuzzer3Pins<Uno::UnoOutputPin<BEEPER_PIN>>、buzzer::PassiveBuzzer3Pins<Uno::UnoTone<SPEAKER_PIN, true>>
audio::Player<Beeper> beeper;
audio::Player<Speaker> speaker;

void setup() {
    beeper.init();
    speaker.init();
    beeper.beeps(Beeper::HZ, 3, 100, 100);                   // 嗶三聲（有源不管 hz，只要 > 0）
    speaker.play<melody::ode_to_joy_flash_passive_medium>();             // lib/audio/output 的旋律
}

void loop() {
    beeper.update();                                         // 每次 loop 都要呼叫，不會等
    speaker.update();
}
```

完整的例子：`src/week_4_SSD1306_switch_buzzer_audio/tests/3_active_buzzer.cpp`（有源）、`src/week_4_SSD1306_switch_buzzer_audio/tests/4_passive_buzzer.cpp`（無源，會從腳位量實際頻率）。

## 接線

| 蜂鳴器 | UNO |
|---|---|
| +（長腳，或標 + / S 的那邊） | 一隻數位腳：有源例如 D8；無源最好 D9 或 D10 |
| - | GND |

三隻腳模組：**I/O → 數位腳、VCC → 5V、GND → GND**，不用電阻。

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

## 兩隻腳和三隻腳

| 零件 | 長相 | 誰推蜂鳴器 | 用 |
|---|---|---|---|
| 兩隻腳的蜂鳴器 | 黑色圓柱，+、- 兩隻腳 | 腳位直接推 | `ActiveBuzzer2Pins`、`PassiveBuzzer2Pins` |
| 三隻腳、中間沒接（KY-012 有源、KY-006 無源） | 小板子，標 S、-（中間那隻沒接東西） | 腳位直接推：S 就是 + | 同上（當成兩隻腳） |
| 三隻腳模組：**VCC、I/O、GND** | 小板子，背面有一顆電晶體（S8550 / 9012） | 電晶體推，電流從 VCC 來 | `ActiveBuzzer3Pins`、`PassiveBuzzer3Pins` |

- **三隻腳模組比較大聲**：蜂鳴器的電流從 VCC 來，腳位只開關電晶體，不用擔心腳位的電流上限，也不用串電阻。
- **三隻腳模組是 LOW 才響**（PNP 電晶體，常寫「低電平觸發」）：
  - 有源：`ActiveBuzzer3Pins` 預設 `LOW_TRIGGER = true`。少數用 NPN 的模組是 HIGH 才響，改成 `false`。
  - 無源：安靜時腳位要停在 **HIGH**，不然電晶體一直開著、線圈一直通電。所以要用 `Uno::UnoTone<PIN, true>`（安靜時 HIGH）；用錯的話會編譯錯誤。
- **分不出來時**：板子上有 VCC 和 I/O 兩個字、背面有三隻腳的小黑元件（電晶體）的，就是三隻腳模組。

## 音量

`player.setVolume(volume)`：0 = 安靜 ～ 255 = 最大聲（`init()` 之後是 255），播放中也能改。

| 蜂鳴器 | 怎麼做 | 能調嗎 |
|---|---|---|
| 無源 | 方波的頻率不變，改變每個週期「開」的比例（工作週期） | ✅ 0 ～ 255 |
| 有源 | 它自己的振盪器決定音量 | ❌ 只有 0 = 靜音、其他 = 開（PWM 會讓它沙沙響） |
| 取樣（`audio::SamplePlayer`） | 每個取樣乘上音量 | ✅ 0 ～ 255 |

**音量和工作週期**：方波的基音大小跟 sin(π × 工作週期) 成正比，50% 時最大。`UnoTone` 用工作週期 = asin(音量 / 255) / π，所以音量就是基音的大小：

| 音量 | 工作週期 | 大小 |
|---|---|---|
| 255 | 50% | 0 dB（最大聲） |
| 128 | 16.7% | -6 dB |
| 64 | 8.1% | -12 dB |
| 32 | 4.0% | -18 dB |
| 16 | 2.0% | -24 dB |
| 1 | 0.13% | -48 dB |

- **實測**（`src/week_4_SSD1306_switch_buzzer_audio/tests/4_passive_buzzer.cpp` 的 test 7，2 kHz）：D9 上量到的工作週期跟設定的一樣（差 0.1% 以內），頻率不變。
- **耳朵聽到的**：-6 dB 大約是「小一點」，-10 dB 大約是「一半大聲」。蜂鳴器也會有一點泛音，所以很小聲時音色會變尖。
- **最小的一步**：工作週期是整數個 tick。20 kHz 時一個週期只有 800 tick，最小聲的幾格會變粗；D9 / D10 以外的腳最短約 2 µs（中斷本身）。

### 極限

| 項目 | 極限 | 原因 / 做法 |
|---|---|---|
| 最大聲 | 音量 255（工作週期 50%） | 方波的基音在 50% 最大，程式不可能更大聲 |
| 更大聲 | 硬體：電壓、電流、共振 | 用三隻腳的模組（電晶體推，電流從 VCC 來）；接近共振頻率（常見 2 ～ 4 kHz，最大聲）；12 V 的蜂鳴器 + 電晶體 |
| 腳位電流 | 建議 ≤ 20 mA，上限 40 mA | 電磁式串 100 Ω（見[接線](#接線)），所以直接接腳位的音量有上限 |
| 取樣的最大聲 | 取樣用滿 0 ～ 255 | `convert.py` 已經把最大的取樣拉到滿格；要更大聲用 `--gain`（會切掉峰值，聽起來更響但會破） |
| 有源 | 不能調 | 只能靜音；要小聲就串電阻（例如 100 ～ 330 Ω，會比較小聲，太大會不響） |

## 有源和無源

| | 有源（active） | 無源（passive） |
|---|---|---|
| 裡面 | 有振盪器 | 只有線圈（電磁式）或壓電片 |
| 通 DC | 響，固定音高（多半約 2.3 kHz） | 不響，只「喀」一聲 |
| 板子要做的 | 開、關 | 送方波，頻率就是音高 |
| 能播的 | 嗶聲、節奏 | 音階、旋律、滑音 |
| 分辨 | 接 5V 會一直響；底部通常封起來、比較高；+ 腳比較長 | 接 5V 不響；底部看得到電路板 |
| 用 | `ActiveBuzzer2Pins` / `ActiveBuzzer3Pins` + `Uno::UnoOutputPin` | `PassiveBuzzer2Pins` / `PassiveBuzzer3Pins` + `Uno::UnoTone` |

兩者只差「`start(hz)` 怎麼做」（有源：腳位 HIGH；無源：開計時器），播放的時序都在 `audio::Player`，所以放在同一個資料夾。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 腳位 | 0 ～ 19；D0 / D1 編譯警告 | 編譯錯誤 |
| 無源的 hz | 16 ～ 20000（`UnoTone`）；好聽的範圍約 200 ～ 5000，最大聲在共振頻率（常見 2 ～ 4 kHz） | 夾到範圍內 |
| 有源的 hz | 任何 > 0 都一樣 | — |
| 音量 | 無源 0 ～ 255、有源只能靜音（見[音量](#音量)） | 255 以上不可能（已經是 50%） |
| 同時發幾個音 | 一個 Sound 一個音 | — |

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
| 用 `PassiveBuzzer…` 推有源蜂鳴器 | 方波一直打斷它的振盪器，聲音很雜 | 有源用 `ActiveBuzzer…` |
| 用 `ActiveBuzzer…` 推無源蜂鳴器 | 只有開的瞬間「喀」一聲；電磁式長時間通 DC 會發熱 | 無源用 `PassiveBuzzer…` |
| 三隻腳模組當成兩隻腳用（`…2Pins`） | 有源：開關反過來，一開機就響；無源：安靜時腳位 LOW，電晶體一直開、線圈一直通電 | 用 `…3Pins`；無源配 `Uno::UnoTone<PIN, true>` |
| `Uno_tone.h` 被兩個 .cpp include | 中斷函式重複定義，連結錯誤 | 只在一個 .cpp 裡 include |
| 無源接 D9 / D10 以外的腳，同時用 `UnoOneWire`（DS18B20）或其他關中斷的程式 | 關中斷時方波暫停，聲音抖一下 | 無源接 D9 或 D10 |
| `update()` 很久才呼叫一次（例如 OLED 的 `display()` 64 ms） | 音的邊界晚，但節奏不飄；無源的方波照常（計時器在跑） | 正常；要準就少做長的事 |
| 板子進入睡眠（power-down） | Timer1 停止，無源不響 | 播放時不要睡 |
| 想調音量 | 無源用 `setVolume`；有源只能靜音 | 有源要小聲就串電阻 |
