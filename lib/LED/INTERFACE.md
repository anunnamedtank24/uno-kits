# LED：介面

| 需要 | 得到 |
|---|---|
| 一個 `Pwm`：`init()`、`write(duty)`（`Uno::UnoPwmPin<PIN>`） | `LED::LED2Pins`、`RGBLED4Pins`、`TrafficLight4Pins`：照眼睛感覺的亮度 0 ～ 255；`LED::Effect`：閃爍、呼吸、漸變、心跳 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 腳位 | `Pwm`：UNO 的 PWM 腳 3、5、6、9、10、11；D9 / D10（Timer1）跟 `UnoTone`、`UnoPcmOut` 衝突，D3 / D11（Timer2）跟 `UnoPcmOut`、`UnoIrCarrier` 衝突 |
| 亮度 | 0 ～ 255 是眼睛看的亮度，工作週期 = 亮度² ÷ 255 |
| 效果 | `Effect::update()` 每次 loop 都要呼叫 |

## 檔案

```
lib/LED/
├── LED.h                       入口：全部（程式只 include 它）
├── brightness.h                toDuty（眼睛的亮度 → 工作週期）、levelToBrightness（第幾格 → 亮度）
├── single_LED.h                SingleLED<Pwm, ACTIVE_HIGH>：一個顏色（模組共用的核心）；NoPwm：沒接的顏色
├── module/                     實際買到的零件
│   ├── LED_2_pins.h            LED2Pins<Pwm>：一顆 LED（2 隻腳）
│   ├── RGB_LED_4_pins.h        RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>：RGB LED（4 隻腳）
│   └── traffic_light_4_pins.h  TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>：紅綠燈模組（R、Y、G、GND）
└── tool/                       建在它上面的常用功能
    └── effect.h                Effect<Led>：閃爍、呼吸、漸變、心跳（不會等）
```

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Pwm` | `init()`（腳位 LOW）、`write(duty)`（0 = 一直 LOW、255 = 一直 HIGH） | `Uno::UnoPwmPin` |

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 設定腳位，關掉 |
| `SingleLED::set(brightness)` | 設定亮度：0 = 關 ～ 255 = 最亮，照眼睛的感覺 |
| `SingleLED::setDuty(duty)` | 直接設定工作週期，不換算 |
| `SingleLED::brightness()` | 回傳目前的亮度 |
| `RGBLED4Pins::set(red, green, blue)`、`TrafficLight4Pins::set(red, yellow, green)` | 設定每個顏色的亮度 |
| `setRed`、`setGreen`、`setBlue`、`setYellow` | 設定一個顏色 |
| `red`、`green`、`blue`、`yellow` | 回傳一個顏色的亮度 |
| `NoPwm` | 什麼都不做的 Pwm：給沒接的顏色 |
| `toDuty(brightness)` | 回傳亮度對應的工作週期：brightness² / 255 |
| `levelToBrightness(level, levels)` | 回傳第 level 格（共 levels 格）的亮度，0 ～ 255 |

`Effect<Led>`（`tool/effect.h`）：一個顏色（有 `set(brightness)`、static `nowMs()` 的，例如 `SingleLED`、`LED2Pins`），不會等：

| 函式 | 說明 |
|---|---|
| `Effect(led)` | 建構：要控制的那一個顏色 |
| `steady(brightness)` | 一直亮，不變 |
| `blink(onMs, offMs, count = 0, brightness = 255)` | 亮 onMs、暗 offMs，count 次後關掉（0 = 一直閃） |
| `breathe(periodMs, low = 0, high = 255)` | 在 low 和 high 之間平順地亮暗，一次呼吸 periodMs |
| `fadeTo(brightness, ms)` | 從現在的亮度在 ms 內變到 brightness，之後停著 |
| `heartbeat(periodMs = 1200, brightness = 255)` | 快跳兩下、停一下，一輪 periodMs |
| `update()` | 越常呼叫越好：算現在的亮度 |
| `kind()` | 回傳現在的效果：`STEADY`、`BLINK`、`BREATHE`、`FADE`、`HEARTBEAT` |
| `isRunning()` | 有次數的 `blink`、`fadeTo` 結束後回傳 false |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `ACTIVE_HIGH`（核心 `SingleLED`） | `true` | 自己接線時用：5V → LED → 電阻 → 腳位時改成 `false`（`LED2Pins` 固定 `true`） |
| `COMMON_ANODE`（`RGBLED4Pins`） | `false` | 共陰是 `false`、共陽是 `true` |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 腳位 | 3、5、6、9、10、11 | 編譯錯誤 |
| 亮度、工作週期 | 0 ～ 255 | — |
| `levelToBrightness` 的 level | 0 ～ levels | 大於 levels 時是 255；levels = 0 時是 0 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `LED2Pins` 1 byte，`RGBLED4Pins`、`TrafficLight4Pins` 3 bytes（記住的亮度） |
| 計時器 | D5、D6：Timer0（跟 `millis` 共用，只開關輸出）；D9、D10：Timer1；D3、D11：Timer2 |
| CPU | 不用：計時器自己產生 PWM |
