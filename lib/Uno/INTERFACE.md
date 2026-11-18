# Uno：介面

| 需要 | 得到 |
|---|---|
| Arduino 的 `millis()`、`micros()`（Arduino UNO，ATmega328P） | 其他函式庫要的介面的 UNO 版本：Bus（`UnoST7789`、`UnoI2C`）、Link（`UnoSerial`）、Wire（`UnoOneWire`）、Pin、Pwm、InterruptPin、AnalogPin（`UnoInputPin`、`UnoPwmPin`、`UnoInterruptPin`、`UnoAnalogPin`）、Tone（`UnoTone`）、Carrier（`UnoIrCarrier`）、SampleOut（`UnoPcmOut`）、背景執行（`UnoBackground`）；接錯腳在編譯時就擋下 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 晶片 | ATmega328P、16 MHz；用 Arduino 的 `millis()`、`micros()`（Timer0） |
| 計時器 | Timer0：`millis`、`UnoBackground`；Timer1：`UnoTone`、`UnoPcmOut`、D9 / D10 的 PWM；Timer2：`UnoPcmOut` 的取樣時鐘、`UnoIrCarrier`、D3 / D11 的 PWM |
| 中斷 | `UnoInterruptPin` 用腳位改變中斷，一組腳只能一個 |
| 檢查 | 腳位、鮑率、計時器衝突大多在編譯時檢查（`static_assert`、deprecated 警告） |

## 檔案

```
lib/Uno/
├── Uno.h               底層工具（腳位、SPI、UART、PWM、Flash、時間、編譯期檢查）+ UnoBoard：每個介面的基底
├── Uno_pin.h           腳位家族：UnoBasicPin → UnoInputPin（= UnoPin）、UnoOutputPin、UnoPwmPin
├── Uno_interrupt_pin.h UnoInterruptPin<PIN, INTERNAL_PULLUP>     InterruptPin：電位改變就呼叫函式（腳位改變中斷）
├── Uno_analog_pin.h    UnoAnalogPin<PIN, REFERENCE, MILLIVOLTS> AnalogPin：ADC 0 ～ 1023、mV；UnoChip：實際的 5V、晶片溫度
├── Uno_ST7789.h        UnoST7789<DC, RES, CS, BLK, MODE>         Bus：ST7789 螢幕（硬體 SPI）
├── Uno_I2C.h           UnoI2C<SCL, SDA, INTERNAL_PULLUP, KHZ>    Bus：I2C 裝置（軟體，任何腳）
├── Uno_serial.h        UnoSerial<BAUD>                           Link：USB 串口
├── Uno_one_wire.h      UnoOneWire<PIN, INTERNAL_PULLUP>          Wire：1-Wire 匯流排
├── Uno_tone.h          UnoTone<PIN, IDLE_HIGH>                   Tone：方波（Timer1）
├── Uno_pcm_out.h       UnoPcmOut<PIN, IDLE_HIGH>                 SampleOut：取樣（Timer1 PWM + Timer2 取樣時鐘）
├── Uno_ir_carrier.h    UnoIrCarrier<PIN, KHZ>                    Carrier：紅外線 LED 的 38 kHz（Timer2，D3 / D11）
└── Uno_background.h    UnoBackground                             背景：每 ~1 ms 自動執行 update()（Timer0 比較中斷）
```

## 提供的介面

腳位都是 Arduino 的編號：D0 ～ D13 = 0 ～ 13，A0 ～ A5 = 14 ～ 19。

### UnoBoard：每個介面都有的函式

| 函式 | 說明 |
|---|---|
| `nowMs()`、`nowUs()` | 開機後的 ms（`millis`）、µs（`micros`，每 4 µs 一格，中斷裡也能用） |
| `delayMs(ms)`、`delayUs(us)` | 忙碌等待（關中斷時也能用） |
| `readFlash8(p)`、`readFlash16(p)` | 讀 Flash（PROGMEM）裡的資料：字型、圖片、影片、聲音 |
| `stopInterrupts()`、`restoreInterrupts(saved)` | 關中斷、恢復：一次讀完或改完中斷也會用的資料 |

例如 `Uno::UnoInputPin<2>::nowMs()`、`Uno::UnoTone<9>::readFlash16(p)` 都可以。

### UnoST7789：ST7789 螢幕的 Bus

```cpp
using Bus = Uno::UnoST7789<8, 10, Uno::NONE, 9>;    // SPI mode 3（預設）
```

| 參數 | 預設 | 說明 |
|---|---|---|
| `DC` | — | 必填 |
| `RES` | `Uno::NONE` | `Uno::NONE` = 只用軟體重置 |
| `CS` | `Uno::NONE` | `Uno::NONE` = 模組沒有 CS 腳（這時必須用 SPI mode 3） |
| `BLK` | `Uno::NONE` | `Uno::NONE` = 背光接 VCC；接 PWM 腳（3、5、6、9、10、11）可以調亮度 |
| `MODE` | 3 | SPI mode 0 ～ 3 |

| 函式 | 說明 |
|---|---|
| `init(blk)`、`setBacklight(blk)` | 設定腳位和 SPI；設定背光 0 ～ 255 |
| `cmdMode`、`dataMode`、`select`、`deselect`、`rstHigh`、`rstLow` | 切換 DC、CS、RES |
| `write8`、`write16` | 送出後馬上回來，CPU 趁傳送時準備下一個 byte；切換 DC、CS 前會先等送完 |
| `readMode`、`writeMode`、`readDummy`、`read8` | 空函式（模組只能寫） |
| `delayMs`、`nowMs`、`readFlash8/16`、`FlashString` | 給上層計時、讀 Flash |

- **固定接線**：SCL 接 D13、SDA 接 D11（硬體 SPI，8 MHz）。

### UnoI2C：I2C 的 Bus

```cpp
using Bus = Uno::UnoI2C<13, 11>;               // SCL = D13、SDA = D11，開內部上拉，目標 400 kHz
using Bus = Uno::UnoI2C<13, 11, false, 100>;   // 只靠外接上拉，100 kHz
```

| 參數 | 預設 | 說明 |
|---|---|---|
| `SCL`、`SDA` | — | 任何兩隻不同的腳（硬體 I2C 只在 A4 / A5，這個用軟體做） |
| `INTERNAL_PULLUP` | `true` | 放開線時順便開內部約 35 kΩ 上拉（模組板上通常已經有 4.7 ～ 10 kΩ） |
| `KHZ` | 400 | 1 ～ 400：每半個時脈至少等多久（LOW ≥ 1.3 µs、HIGH ≥ 0.6 µs） |

| 函式 | 說明 |
|---|---|
| `init()` | 放開兩條線；SDA 被拉住時（板子在傳輸途中重置）先送最多 9 個時脈再送 STOP |
| `start(address, read)` | 送 START + 7 位元位址 + 讀寫位元，回傳有沒有 ACK |
| `write8(byte)` | 送一個 byte（高位元先），回傳有沒有 ACK |
| `read8(ack)` | 讀一個 byte；`ack = false` 表示最後一個 |
| `stop()` | 送 STOP |
| `delayMs`、`nowMs`、`readFlash8/16`、`FlashString` | 給上層計時、讀 Flash |

- **開汲極**：要輸出 LOW 時才把腳設成輸出，HIGH 是放開讓上拉電阻拉高。
- **等 SCL 真的變 HIGH**：放開 SCL 後一直讀到 HIGH 才繼續（最多約 200 次），所以上拉太弱、或裝置拉住 SCL（clock stretching）只會變慢。
- **實際速度**：`KHZ = 400` 時約 160 kHz。延遲只佔每個位元的 43 個週期，呼叫腳位函式另外要約 56 個週期。

### UnoSerial：USB 串口的 Link

```cpp
using Link = Uno::UnoSerial<1000000>;   // 1 Mbaud，D0 / D1
```

| 函式 | 說明 |
|---|---|
| `init()` | 設定 UART（8N1、倍速模式） |
| `available()` | 回傳有沒有收到 byte |
| `read8(byte, timeoutMs)` | 等一個 byte，最多 `timeoutMs`；回傳有沒有收到 |
| `write8(byte)` | 送一個 byte（等到能送為止） |
| `clearInput()` | 丟掉已經收到的 byte |
| `printText(text)` | 送一串文字（給 `pio device monitor` 看） |
| `printNumber(value, width = 0)` | 送十進位的整數（任何整數型別，負數有 `-`）；width > 0 時前面補空白靠右對齊 |
| `printDecimal(value, decimals, width = 0)` | 送定點數：value ÷ 10^decimals，小數點後 decimals 位（0 ～ 9），例如 `printDecimal(2350, 2)` → `23.50`、`printDecimal(-5, 1)` → `-0.5` |
| `printHex(value, digits = 2)` | 送 `0x` 和最低 digits 位的十六進位，例如 `printHex(0x1F, 4)` → `0x001F` |
| `printPin(pin)` | 送腳位的名稱：0 ～ 13 → `D0` ～ `D13`，14 ～ 19 → `A0` ～ `A5`；程式印出設定的腳位，換腳後印的也跟著變 |

- **print 不是 Link 的一部分**：串流（`audio`、`image`、`video`）只用上面五個；print 是給程式印文字用的，每個程式不用再各寫一份。
- **輪詢，不用中斷**：所以不能同時用 Arduino 的 `Serial`。
- **鮑率**：16 MHz 下 1000000 和 2000000 剛好整除；115200 有 2.1% 誤差，會出警告。

### UnoOneWire：1-Wire 的 Wire

```cpp
using SensorWire = Uno::UnoOneWire<2>;          // DQ 接 D2，順便開內部上拉
using SensorWire = Uno::UnoOneWire<2, false>;   // 只靠外接的 4.7 kΩ 上拉
```

| 函式 | 說明 |
|---|---|
| `init()` | 放開線 |
| `reset()` | 送 reset，回傳有沒有裝置回應；線是 LOW 時（沒有上拉、短路）直接回傳 false |
| `writeBit`、`readBit`、`write8`、`read8` | 送、讀一個位元或 byte（低位元先） |
| `delayMs`、`nowMs` | 計時 |

- **時序**：reset 480 µs、寫 1 拉低 6 µs、寫 0 拉低 60 µs、讀在拉低後 13 µs 取樣，每個位元約 70 µs。
- **關中斷**：每個位元的時序裡關中斷約 70 µs，避免 `millis()` 的中斷把時序拉長。

### UnoBasicPin：每隻腳的共同部分

| 常數 | 說明 |
|---|---|
| `NUMBER` | Arduino 的腳位編號 |
| `CAN_PWM`、`CAN_ANALOG` | 能不能 PWM（3、5、6、9、10、11）、能不能類比（A0 ～ A5） |
| `IS_SERIAL`、`IS_SPI`、`IS_LED` | 是不是 USB 串口（D0、D1）、硬體 SPI（D11 ～ D13）、板上的 L 燈（D13） |

### UnoInputPin（= UnoPin）、UnoOutputPin：一隻腳的 Pin

```cpp
using SwitchPin = Uno::UnoInputPin<2>;          // 開關模組的 OUT 接 D2（模組自己輸出高低，不開上拉）
using ButtonPin = Uno::UnoInputPin<2, true>;    // 裸按鈕接 D2 和 GND：開內部上拉，按下是 LOW
using BuzzerPin = Uno::UnoOutputPin<8>;         // 有源蜂鳴器接 D8
```

| 函式 | 說明 |
|---|---|
| `UnoInputPin::init()` | 設成輸入（`INTERNAL_PULLUP` 時開內部上拉） |
| `UnoInputPin::read()` | 回傳電位，true = HIGH |
| `UnoOutputPin::init()` | 先拉 LOW 再設成輸出（不會閃一下 HIGH） |
| `UnoOutputPin::write(high)` | 設定電位 |
| `UnoOutputPin::toggle()` | HIGH ↔ LOW |
| `UnoOutputPin::isHigh()` | 現在輸出的電位 |

`UnoPin` 是 `UnoInputPin` 的舊名，兩個一樣；舊程式照樣能編譯，文件和程式都寫 `UnoInputPin`。

### UnoPwmPin：一隻 PWM 腳的 Pwm

```cpp
using RedPin = Uno::UnoPwmPin<3>;          // LED 接 D3
```

| 函式 | 說明 |
|---|---|
| `init()` | 設成輸出、LOW |
| `write(duty)` | 設定工作週期（`pwmSet`）：0 = 一直 LOW、255 = 一直 HIGH、中間約 976 Hz 的 PWM |
| `duty()` | 上一次寫的工作週期 |

- **只能用 PWM 腳**：3、5、6、9、10、11，其他腳編譯錯誤。
- **計時器**：D5、D6 是 Timer0（跟 `millis` 共用，只開關輸出，LED、L298N 的 EN 沒問題）；D9、D10 是 Timer1（不能跟 `UnoTone` 一起用）；D3、D11 是 Timer2。

### UnoInterruptPin：腳位改變中斷的 InterruptPin

```cpp
using IrPin = Uno::UnoInterruptPin<2>;     // 紅外線接收器的 OUT 接 D2
IrPin::init(&onChange);                    // 每次 D2 的電位改變，就在中斷裡呼叫 onChange()
```

| 函式 | 說明 |
|---|---|
| `init(onChange)` | 設成輸入（`INTERNAL_PULLUP` 時開內部上拉），開腳位改變中斷 |
| `init()` | 只設成輸入（跟 `UnoInputPin` 一樣） |
| `read()` | 回傳電位，true = HIGH（繼承自 `UnoInputPin`） |

- **腳位改變中斷（PCINT）**：每隻腳都能用，但腳位分三組共用三個中斷：D0 ～ D7、D8 ～ D13、A0 ～ A5。**一組只能有一個 `UnoInterruptPin`**，第二個會蓋掉第一個的函式。
- **為什麼不用輪詢**：紅外線的訊號最短 0.56 ms，OLED 的 `display()` 一次要 64 ms，輪詢一定會漏。
- **中斷定義在標頭檔**：跟 `Uno_tone.h` 一樣，只能在一個 .cpp 裡 include。

### UnoAnalogPin：類比輸入的 AnalogPin

```cpp
using Knob = Uno::UnoAnalogPin<14>;                              // A0，參考電壓 = 5V
using Small = Uno::UnoAnalogPin<15, Uno::REFERENCE_INTERNAL>;    // A1，參考電壓 = 晶片的 1.1 V
```

| 函式 | 說明 |
|---|---|
| `init()` | 設成輸入（不上拉），開 ADC |
| `readAnalog()` | 0 ～ 1023（0 V ～ 參考電壓），約 112 µs |
| `readAnalogAverage(count)` | `count` 次的平均（1 ～ 64），雜訊比較小 |
| `readMillivolts()` | 換成 mV（用 `REFERENCE_MILLIVOLTS`） |
| `referenceMillivolts()` | 參考電壓的 mV |
| `read()` | 數位讀取（繼承自 `UnoInputPin`） |
| `MAX` | 1023 |

| `REFERENCE` | 1023 代表 | 用在 |
|---|---|---|
| `REFERENCE_VCC`（預設） | 5V 腳的電壓（USB 約 4.6 ～ 5.2 V） | 用 5V 供電的感測器：比例不受 5V 高低影響 |
| `REFERENCE_INTERNAL` | 晶片內部的 1.1 V（± 10 %，但很穩） | 小電壓、電池電壓（先分壓） |
| `REFERENCE_EXTERNAL` | AREF 腳接的電壓 | 外接的參考電壓；**AREF 有接東西時不能用其他兩種**（會短路） |

- **只能用 A0 ～ A5**（14 ～ 19），其他腳編譯錯誤。A4、A5 也是 I2C 的 SDA、SCL（這個專案的 I2C 是軟體的，可以接任何腳）。
- **換參考電壓**：讀不同參考電壓的腳時，會先等 400 µs 再丟掉一次讀數，第一次的值才準。
- **不同時讀**：ADC 只有一個，一次讀一隻腳。

### UnoChip：晶片本身

| 函式 | 說明 |
|---|---|
| `UnoChip::readVccMillivolts()` | 5V 腳實際的電壓（用內部 1.1 V 反推，± 10 %）：校正 `REFERENCE_MILLIVOLTS`、看電池剩多少 |
| `UnoChip::readTemperature()` | 晶片溫度，0.1 °C（± 10 °C，而且比室溫高），不能當溫度計 |
| `BANDGAP_MILLIVOLTS` | 內部參考電壓（1100），量 AREF 腳可以校正 |

### UnoBackground：背景執行

```cpp
#include "Uno_background.h"
Uno::UnoBackground::add([] { car.update(); });     // 從現在起每 ~1 ms 自動呼叫
```

| 函式 | 說明 |
|---|---|
| `add(task)` | 加一個函式（不能有參數；沒有擷取的 lambda 可以），最多 `MAX_TASKS`（6）個 |
| `remove(task)`、`clear()` | 拿掉一個、全部 |
| `pause()`、`resume()`、`isPaused()` | 暫停、繼續 |
| `count()` | 有幾個函式 |
| `skipped()` | 上一輪還沒做完、所以跳過的次數（函式太慢） |

- **用 Timer0 的比較 A 中斷**：Timer0 每 1.024 ms 一圈，不管 OCR0A 是多少都會觸發一次，所以 D6 的 PWM 不受影響；Arduino 核心沒有用到它。
- **中斷裡會再開中斷**：背景函式執行時 `millis()`、串口、腳位中斷都照常。
- **函式要短**：`update()` 這類不會等的函式（幾十 µs）最適合；不要在裡面 `delay`、印字。
- **要搭配有保護的類別**：`L298N::DCMotor`、`Stepper`、`TwoWheelCar` 的每個函式都會短暫關中斷，背景和 `loop()` 同時用也不會衝突。其他類別放背景時，`loop()` 改它的資料前要自己 `stopInterrupts()`。
- **中斷定義在標頭檔**：只能在一個 .cpp 裡 include。

### UnoTone：方波的 Tone

```cpp
using Tone = Uno::UnoTone<9>;              // start(440) 送 440 Hz，stop() 停止並拉 LOW
using Tone = Uno::UnoTone<9, true>;        // 安靜時 HIGH：三隻腳的無源蜂鳴器模組（PNP，LOW 才通）
```

| 參數 | 預設 | 說明 |
|---|---|---|
| `PIN` | — | 任何腳；D9 / D10 不佔 CPU |
| `IDLE_HIGH` | `false` | 安靜時（`init`、`stop` 後）腳位的電位；也是常數 `IDLE_LEVEL_HIGH`，給 `buzzer` 的模組檢查 |

| 函式 | 說明 |
|---|---|
| `init()` | 停止計時器，腳位設成輸出、安靜的電位（預設 LOW） |
| `start(hz)` | 開始送方波，或在播放中直接改頻率（不會停頓，滑音用）；0 = `stop()` |
| `stop()` | 停止計時器，腳位回到安靜的電位 |
| `realHz()` | 回傳實際播出的頻率（tick 是整數），停止時是 0 |
| `setVolume(volume)`、`volume()` | 設定、回傳音量 0 ～ 255（0 = 安靜），播放中也能改 |
| `realDuty()` | 回傳實際的工作週期（1 / 65536 個週期，32768 = 50%） |
| `nowMs`、`readFlash16` | 給上層計時、讀 Flash |

| 項目 | 說明 |
|---|---|
| 計時器 | Timer1 的 fast PWM（ICR1 當 TOP）：頻率 = 16 MHz ÷（預除 ×（ICR1 + 1）），OCR1A / OCR1B 決定「開」多久 |
| 預除 | 245 Hz 以上用 1（4 kHz 時誤差 < 0.02%），31 Hz 以上用 8，以下用 64（最低 16 Hz） |
| D9、D10 | 計時器直接控制腳位（OC1A / OC1B），完全不佔 CPU |
| 其他腳 | 每個週期兩個中斷（開、關），每次約 2 µs，20 kHz 時約佔 16% 的 CPU；最短的「開」約 2 µs |
| 工作週期 | `setVolume(0 ～ 255)`：255 = 50%（最大聲），其他 = asin(音量 / 255) / π，頻率不變 |
| 實測 | 50 Hz ～ 15 kHz，D9 和 D7 從腳位讀回來的頻率都跟 `realHz()` 一致（`src/week_4_SSD1306_switch_buzzer_audio/tests/4_passive_buzzer.cpp`） |

### UnoIrCarrier：紅外線發射的 Carrier

```cpp
using Carrier = Uno::UnoIrCarrier<3>;      // 紅外線 LED 接 D3，38 kHz
```

| 函式 | 說明 |
|---|---|
| `init()` | 腳位 LOW，Timer2 以 2 × KHZ 執行（CTC），還沒接到腳位 |
| `on()`、`off()` | LED 以 KHZ 閃（mark）/ 熄滅（space） |
| `REAL_HZ` | 實際的頻率（38 kHz → 37.9 kHz） |

- **只能 D3（OC2B）或 D11（OC2A）**：方波由 Timer2 直接輸出，不佔 CPU。
- 用的時候 Timer2 是它的：不能同時用 D3 / D11 的 PWM、`UnoPcmOut`。

### UnoPcmOut：取樣的 SampleOut

```cpp
using PcmOut = Uno::UnoPcmOut<9>;          // 蜂鳴器或喇叭接 D9（或 D10）
using PcmOut = Uno::UnoPcmOut<9, true>;    // 三隻腳的無源模組（PNP，安靜時 HIGH）
```

| 函式 | 說明 |
|---|---|
| `init()` | 停止計時器，腳位設成輸出、安靜的電位 |
| `start(rate, next)` | 開始：每秒 rate 次在中斷裡呼叫 `next()`，回傳值（0 ～ 255）就是 PWM 的工作週期 |
| `stop()` | 停止兩個計時器，腳位回到安靜的電位 |
| `realRate()` | 回傳實際的取樣率，停止時是 0 |
| `stopInterrupts()`、`restoreInterrupts(saved)`、`nowMs`、`readFlash8/16` | 給播放器讀中斷寫的資料、讀 Flash |

| 項目 | 說明 |
|---|---|
| 聲音 | Timer1 fast PWM 8 bits、不預除：62.5 kHz，工作週期 = 取樣；遠超過聽得到的範圍，只剩平均值變成聲音 |
| 取樣時鐘 | Timer2 CTC：2 MHz / n（n ≤ 256，7813 Hz 以上）或 500 kHz / n；8000、16000 剛好，11025 → 11050、22050 → 21978（`convert.py` 用同樣的算法） |
| 範圍 | 1954 ～ 32000 Hz，超出夾到範圍內 |
| 腳位 | 只能 D9（OC1A）或 D10（OC1B），其他腳編譯錯誤 |
| 衝突 | Timer1 和 Timer2 都是它的：不能同時用 `UnoTone`、D3 / D9 / D10 / D11 的 PWM、`Servo`、Arduino `tone()` |

## 分級

每個介面都繼承 `UnoBoard`，腳位再一層一層繼承，大的型態一定能放在要小型態的地方：

```
UnoBoard                          時間、等待、Flash、關中斷（Uno.h）——每個介面都有
├── UnoBasicPin<PIN>              腳位編號和它能做什麼（編譯期常數）
│   ├── UnoInputPin<PIN, PULLUP>  Pin（輸入）：init、read                 = UnoPin（舊名，一樣能用）
│   │   ├── UnoInterruptPin       InterruptPin：init(onChange)、read
│   │   └── UnoAnalogPin          AnalogPin：init、readAnalog、readMillivolts…（read 也還能讀數位）
│   ├── UnoOutputPin<PIN>         Pin（輸出）：init、write、toggle、isHigh
│   └── UnoPwmPin<PIN>            Pwm：init、write(duty)、duty
├── UnoST7789、UnoI2C             Bus：螢幕、I2C
├── UnoSerial                     Link：USB 串口
├── UnoOneWire                    Wire：1-Wire
├── UnoTone                       Tone：方波
├── UnoPcmOut                     SampleOut：取樣
├── UnoIrCarrier                  Carrier：紅外線的 38 kHz
├── UnoChip                       晶片本身：實際的 5V、晶片溫度
└── UnoBackground                 背景執行
```

| 分類 | 介面 | UNO 的型態 | 誰用 |
|---|---|---|---|
| **共同**（每個都有） | — | `UnoBoard`：`nowMs`、`nowUs`、`delayMs`、`delayUs`、`readFlash8/16`、`stopInterrupts`、`restoreInterrupts` | 全部 |
| **腳位** | Pin（輸入） | `UnoInputPin`（= `UnoPin`）、`UnoInterruptPin`、`UnoAnalogPin` | switch、TCRT5000、ultrasonic Echo、joystick 按鈕 |
| | Pin（輸出） | `UnoOutputPin` | 有源蜂鳴器、L298N 的 IN、ultrasonic Trig |
| | Pwm | `UnoPwmPin` | LED、L298N 的 EN（或 IN） |
| | InterruptPin | `UnoInterruptPin` | NEC 接收器、ultrasonic Echo |
| | AnalogPin | `UnoAnalogPin` | analog（旋鈕、光敏、搖桿、熱敏、聲音）、TCRT5000 四腳板 |
| **匯流排** | Bus | `UnoST7789`（SPI 螢幕）、`UnoI2C` | ST7789、SSD1306 |
| | Link | `UnoSerial` | `stream::Puller`（lib/stream）：串流圖片、影片、聲音 |
| | Wire | `UnoOneWire` | DS18B20 |
| **計時器** | Tone | `UnoTone` | 無源蜂鳴器 |
| | SampleOut | `UnoPcmOut` | 取樣聲音 |
| | Carrier | `UnoIrCarrier` | 紅外線發射（NEC） |
| | 背景 | `UnoBackground` | 任何有 `update()` 的東西 |

- **函式庫只要最小的那個**：例如 `switch` 要 Pin（`init`、`read`），所以 `UnoInputPin`、`UnoInterruptPin`、`UnoAnalogPin` 都能用。
- **繼承不佔空間**：全部是 static 函式，沒有虛擬函式，編譯後跟直接寫暫存器一樣。

## 編譯期檢查

腳位都是 template 參數，編譯時就檢查：

| 類型 | 情況 |
|---|---|
| **編譯錯誤** | 腳位不存在；DC / RES / CS / BLK 用到 SPI 腳（D11、D12、D13）；沒有 CS 卻不是 mode 3；鮑率誤差超過 5%；`UnoI2C` 的兩隻腳相同或 `KHZ` 超出 |
| **編譯警告** | 兩個訊號用同一隻腳；用到 D0 / D1（USB 串口）；BLK 不是 PWM 腳；BLK 用 D5 / D6（Timer0，頻率固定）；鮑率誤差超過 2% |

警告的寫法：`static_assert(Uno::warn::Xxx<條件>::ok, "")`。條件成立時會用到一個標了 deprecated 的成員，編譯器就印出警告。

## Uno.h 的工具

| 分類 | 內容 |
|---|---|
| 腳位功能表 | `PIN_FUNCTIONS[20]`，以及 `exists`、`has`、`optional`、`same`、`mask` |
| 數位腳 | `pinOutput<P>()`、`pinInput<P>(pullUp)`、`pinHigh<P>()`、`pinLow<P>()`、`pinRead<P>()`；`P` 是 `NONE` 時什麼都不做（讀到 false） |
| SPI / UART | `spiSet(mode)`：SPI 主模式，8 MHz；`uartSet(ubrr)`、`uartUbrr(baud)`：UART，8N1，倍速模式；`uartRealBaud(baud)`、`uartErrorPermille(baud)`：實際的鮑率、誤差（‰，編譯時檢查用） |
| PWM | `pwmSet<P>(duty)`：Timer1 / Timer2 用 fast PWM；Timer0（D5、D6）只開關輸出，不動設定（`millis` 要用） |
| Flash / 時間 | `readFlash8/16`、`delayMs`（忙等，不靠 Timer0）、`nowMs`（`millis`） |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 腳位（template） | 0 ～ 19，或 `Uno::NONE`（只有選填的腳） | 編譯錯誤；D0 / D1 編譯警告 |
| `UnoPwmPin` 的腳位 | 3、5、6、9、10、11 | 編譯錯誤 |
| `UnoAnalogPin` 的腳位 | 14 ～ 19（A0 ～ A5） | 編譯錯誤 |
| `UnoPcmOut` 的腳位 | 9、10 | 編譯錯誤 |
| `UnoIrCarrier` 的腳位 | 3、11 | 編譯錯誤 |
| `MODE` | 0 ～ 3 | 編譯錯誤 |
| `BAUD` | 2000 ～ 2000000，誤差 ≤ 5% | 編譯錯誤；誤差 > 2% 編譯警告 |
| `UnoI2C` 的 `KHZ` | 1 ～ 400 | 編譯錯誤 |
| `init(blk)`、`setBacklight(blk)` | 0 ～ 255；BLK 不是 PWM 腳時只有 0 = 關、其他 = 開 | — |
| `read8(byte, timeoutMs)` | 0 ～ 65535 ms | — |
| `UnoTone::start(hz)` | 16 ～ 20000，0 = 停止 | 夾到範圍內 |
| `UnoPcmOut::start(rate, next)` | 1954 ～ 32000 Hz | 夾到範圍內 |
| `readAnalogAverage(count)` | 1 ～ 64 | 夾到範圍內 |
| `UnoBackground::add` | 最多 6 個函式 | 回傳 false |
| `readFlash8/16(p)` | 必須指向 `PROGMEM` 的資料 | 不檢查：讀到錯的資料 |
| `delayMs(ms)` | 0 ～ 65535 | — |

## 資源

| 資源 | 誰用 | 影響 |
|---|---|---|
| D11（MOSI）、D13（SCK） | `UnoST7789` | 不能當一般腳位 |
| D12（MISO） | SPI 開著時強制當輸入 | 不能當輸出 |
| D10（SS） | `spiSet` 設成輸出（SS 是輸入又被拉低時，SPI 會變成從機） | 可以當 RES / CS / BLK，不能當輸入 |
| D0、D1 | `UnoSerial` | 不能同時用 Arduino 的 `Serial`；燒錄時也會用到 |
| `UnoI2C` 的兩隻腳 | 軟體 I2C | 例如 D13 / D11 時不能同時接 SPI 螢幕；D13 的板載 LED 會跟著閃 |
| Timer0 | Arduino 的 `millis()`（`nowMs()`） | BLK 接 D5 / D6 時只能用固定約 976 Hz 的 PWM |
| Timer1（D9、D10）、Timer2（D3、D11） | `pwmSet`（例如 BLK、`UnoPwmPin`） | 跟 `Servo`、Arduino `tone()` 等用同一個計時器的函式庫衝突 |
| Timer1 | `UnoTone` | 播放時整個 Timer1 都是它的 |
| Timer1 + Timer2 | `UnoPcmOut` | 播放取樣時兩個都是它的 |
| Timer2 | `UnoIrCarrier` | 紅外線發射時是它的（D3 / D11 不能 PWM） |
| Timer0 比較 A 中斷 | `UnoBackground` | 只用中斷、不改 Timer0，`millis()` 和 D6 的 PWM 照常 |
| ADC | `UnoAnalogPin`、`UnoChip` | 一次一隻腳，每次約 112 µs（忙碌等待） |
| 中斷 | `UnoTone` 不在 D9 / D10 時用 Timer1 比較中斷；`UnoInterruptPin` 用腳位改變中斷（三組，一組一個）；`UnoOneWire` 每個位元關中斷約 70 µs；`UnoBackground` 用 Timer0 比較 A 中斷 | 其他都是輪詢，`update()` 要自己一直呼叫，或交給 `UnoBackground` |
