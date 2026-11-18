# L298N：介面

| 需要 | 得到 |
|---|---|
| EN 拔掉跳線帽：IN 用輸出腳（`Uno::UnoOutputPin<PIN>`）、EN 用 PWM 腳（`Uno::UnoPwmPin<PIN>`） | `L298N::L298NBoard`：兩個 H 橋 `Board::A`、`Board::B` |
| EN 接著跳線帽：IN 用 PWM 腳（`Uno::UnoPwmPin<PIN>`），或 `L298N::Ground`（接 GND） | `L298N::L298NBoardJumpers`：同上，速度用 IN 的 PWM |
| 上面的 `Board::A`、`Board::B` | `L298N::DCMotor`（速度、漸變、煞車、死區）、`Stepper`（步進馬達）、`TwoWheelCar`（兩輪車，動作排隊執行） |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [README.md](README.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| 腳位 | IN：輸出 `Pin`（或 `Ground`：接 GND）；EN：`Pwm`（或 `EnableJumper`：跳線帽接著） |
| 電源 | 馬達電源的 GND 一定要跟 UNO 的 GND 接在一起 |
| 呼叫 | `update()` 每次 loop 都要呼叫，或交給 `Uno::UnoBackground`；每個函式短暫關中斷，背景和 loop 不會互相干擾 |
| PWM | EN 用 Timer0 的腳（D5、D6）不影響 `millis`；D9 / D10（Timer1）不能跟 `UnoTone`、`UnoPcmOut` 一起用 |

## 檔案

```
lib/L298N/
├── L298N.h             入口：全部（程式只 include 它）
├── bridge.h            一個 H 橋：Bridge<In1, In2, Enable>、InputPwmBridge<In1, In2>、EnableJumper、Ground
├── dc_motor.h          DCMotor<Bridge>：直流馬達，速度 -255 ～ 255、漸變、死區、反向、煞車 / 滑行
├── stepper.h           Stepper<BridgeA, BridgeB>：雙極步進馬達，WAVE / FULL / HALF、步數 / 秒、位置
├── module/             實際買到的零件
│   └── L298N_board.h   L298NBoard<Ena, In1, In2, In3, In4, Enb>、L298NBoardJumpers<In1 ～ In4>：紅色模組
└── device/             用它做成的東西（一個 class = 一種接線方式，參數可以調）
    └── two_wheel_car.h TwoWheelCar<Board, IN12_SIDE, IN12_POSITIVE, IN34_POSITIVE>：兩輪車，直走、原地旋轉、單輪旋轉、速差，有時間、會排隊
```

- **`module/` 和 `device/` 的差別**：`module/` 是買到的板子（腳位、跳線）；`device/` 是把板子和馬達接成一台東西的方式（哪個 IN 接哪個輪子、正的時候往哪轉），以及那台東西的動作。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `In1`、`In2`（`Bridge`） | `init()`、`write(bool)`、`nowMs()`、`nowUs()` | `Uno::UnoOutputPin`、`L298N::Ground`（接 GND） |
| `Enable`（`Bridge`） | `init()`、`write(duty)` | `Uno::UnoPwmPin`、`L298N::EnableJumper` |
| `In1`、`In2`（`InputPwmBridge`） | `init()`、`write(duty)`、`nowMs()`、`nowUs()` | `Uno::UnoPwmPin`、`L298N::Ground`（接 GND） |
| `Bridge`（`DCMotor`、`Stepper`） | `init()`、`drive(duty)`、`brake(duty)`、`coast()`、`nowMs()`、`nowUs()`、`stopInterrupts()`、`restoreInterrupts()` | `L298N::Bridge`、`InputPwmBridge`；板子的 `Board::A`、`Board::B` |

## 函式

### Bridge：一個 H 橋（低階）

| 函式 | 說明 |
|---|---|
| `init()` | 設定腳位，滑行 |
| `drive(duty)` | -255 ～ 255：> 0 正轉（OUT1 是 +）、< 0 反轉、0 滑行。先設 IN 再設 EN，不會有一瞬間轉錯方向 |
| `brake(duty)` | 兩端短路煞車；duty 0 ～ 255 是煞多用力（PWM 煞車），預設 255 |
| `coast()` | 輸出斷開，馬達自己慢慢停；EN 一直開的接法會變成煞車 |
| `nowMs()`、`nowUs()` | 時間，從 In1 來 |

### DCMotor：直流馬達

| 函式 | 說明 |
|---|---|
| `init()` | 滑行，速度 0 |
| `setSpeed(speed)` | 目標速度 -255 ～ 255，`update()` 會照漸變慢慢改過去；正轉換反轉會先經過 0 |
| `setSpeedNow(speed)` | 同上，但馬上到（不漸變） |
| `update()` | 每次 loop 都要呼叫；還在漸變時回傳 true |
| `brake(duty)` | 馬上煞車並保持；目標變 0。duty 預設 255（最用力） |
| `coast()` | 馬上滑行；目標變 0 |
| `speed()`、`targetSpeed()` | 現在的速度、目標速度 |
| `isBraking()` | 煞車中（直到再設速度） |
| `setRampMs(ms)`、`rampMs()` | 從 0 到 255 要幾 ms，預設 300；0 = 不漸變。保護齒輪和電源：突然起動或反轉時電流很大，可能讓 UNO 重開機 |
| `setMinDuty(duty)`、`minDuty()` | 死區：速度 1 ～ 255 對應到工作週期 duty ～ 255（0 = 工作週期就是速度）。馬達在低工作週期只會嗡嗡叫不會轉，用 `src/week_7_L298N/tests/4_pwm.cpp` 找這個值 |
| `setReversed(bool)`、`isReversed()` | 馬達的線接反了：交換正反轉（不用重接線） |
| `dutyFor(speed)` | 這個速度實際給的工作週期（算上死區和反向） |

### Stepper：雙極步進馬達

| 函式 | 說明 |
|---|---|
| `init()` | 線圈不通電、位置 0、FULL、100 步 / 秒 |
| `setMode(mode)`、`mode()` | `WAVE`（一次一個線圈，力最小、電流最小）、`FULL`（兩個線圈，力最大，預設）、`HALF`（兩種交替，步數加倍、比較順） |
| `setSpeed(stepsPerSecond)`、`speed()` | 1 ～ 2000 步 / 秒。太快馬達會「失步」：在原地嗡嗡叫不轉 |
| `move(steps)` | 相對移動：> 0 往前、< 0 往後 |
| `moveTo(position)` | 移到絕對位置 |
| `run(direction)` | 一直轉：1 往前、-1 往後、0 停 |
| `stop()` | 停在現在的位置 |
| `update()` | 每次 loop 都要呼叫；剛走一步時回傳 true |
| `isMoving()` | 還有步要走（或 `run` 中），以及最後一步之後的一個間隔（讓轉子到位） |
| `position()`、`setPosition(p)`、`target()` | 現在的步數（往前 +）、把現在的位置叫做 p、目標 |
| `setRunDuty(duty)` | 走的時候線圈的工作週期，預設 255。用來降低電流（見 [README 的步進馬達要注意](README.md#步進馬達要注意)） |
| `setHoldDuty(duty)` | 停的時候線圈的工作週期，預設 0 = 不通電（不熱，但軸可以被轉動）；> 0 可以出力固定位置，但會熱 |
| `release()` | 兩個線圈馬上斷電 |

| 模式 | 線圈的順序（A、B） | 1.8° 的馬達一圈 |
|---|---|---|
| `WAVE` | A+ → B+ → A- → B- | 200 步 |
| `FULL` | A+B+ → A-B+ → A-B- → A+B- | 200 步 |
| `HALF` | A+ → A+B+ → B+ → A-B+ → A- → A-B- → B- → A+B- | 400 步 |

### TwoWheelCar：兩輪車（device/two_wheel_car.h）

一個 class 代表一種接線方式，參數是每對 IN 的作用：

| 參數 | 值 | 意思 |
|---|---|---|
| `Board` | `L298NBoard<…>` 或 `L298NBoardJumpers<…>` | 板子和腳位；Board::A = IN1、IN2 → OUT1、OUT2，Board::B = IN3、IN4 → OUT3、OUT4 |
| `IN12_SIDE` | `L298N::LEFT`、`L298N::RIGHT` | IN1、IN2 接的是哪個輪子；IN3、IN4 就是另一個 |
| `IN12_POSITIVE` | `L298N::FORWARD`、`L298N::BACKWARD` | IN1 - IN2 為正（IN1 HIGH、IN2 LOW）時，那個輪子往前還是往後 |
| `IN34_POSITIVE` | `L298N::FORWARD`、`L298N::BACKWARD` | IN3 - IN4 為正時，另一個輪子往前還是往後 |
| `QUEUE` | 預設 8 | 最多幾個動作可以排隊（每個 9 bytes RAM） |

```cpp
// IN1 - IN2 為正：右輪向前；IN3 - IN4 為正：左輪向前
using Car = L298N::TwoWheelCar<Board, L298N::RIGHT, L298N::FORWARD, L298N::FORWARD>;
Car car;

car.init();
car.forward(180, 1500);        // 直走 1.5 秒
car.spin(180, 450);            // 接著原地右轉 0.45 秒（排隊，不用等）
// loop 裡呼叫 car.update()；或 car.wait() 等全部做完
```

| 函式 | 動作 | 左輪 | 右輪 |
|---|---|---|---|
| `forward(speed, ms)` | 直走 | +speed | +speed |
| `backward(speed, ms)` | 後退 | -speed | -speed |
| `spin(speed, ms)` | 原地旋轉：> 0 往右（順時針）、< 0 往左 | +speed | -speed |
| `pivotLeft(speed, ms)` | 單輪旋轉：左輪煞住不動，右輪轉，車往左轉 | 煞車 | speed |
| `pivotRight(speed, ms)` | 單輪旋轉：右輪煞住不動，左輪轉，車往右轉 | speed | 煞車 |
| `arc(speed, difference, ms)` | 兩輪速差：difference > 0 往右彎、< 0 往左彎 | speed + difference / 2 | speed - difference / 2 |
| `wheels(left, right, ms)` | 兩輪各自的速度 | left | right |
| `brake(ms)`、`coast(ms)` | 停住 / 滑行一段時間（EN 跳線接著時滑行會變成煞車） | | |

| 函式 | 說明 |
|---|---|
| `init()` | 兩輪煞住，佇列清空，漸變 200 ms |
| `stop()` | 清掉佇列，馬上煞車 |
| `update()` | 每次 loop 都要呼叫：漸變、時間到換下一個動作 |
| `isBusy()` | 有計時的動作在跑或有動作在排隊（沒有時間的動作不算） |
| `isRunning()` | 有任何動作在跑（包括沒有時間的） |
| `moveMs()` | 現在的動作做多久了 |
| `until(condition)` | 最後給的動作在 `condition()` 成立時提早結束 |
| `setFailsafeMs(ms)` | 沒有時間的動作，超過 ms 沒有新動作就自動煞車（0 = 不會） |
| `wait()` | 一直呼叫 `update()` 直到 `isBusy()` 是 false（會卡住） |
| `queued()` | 排隊中的動作數 |
| `leftSpeed()`、`rightSpeed()` | 現在兩輪的速度（+ = 往前） |
| `setRampMs(ms)` | 兩輪的漸變；**漸變時間算在動作的時間裡** |
| `setMinDuty(duty)` | 兩輪的死區 |
| `setTrim(left%, right%)` | 一顆馬達比較快（直走會偏）：把快的那邊調成 90 % 之類 |

- **時間**：每個動作的 `ms` 到了就換下一個；最後一個計時的動作結束後兩輪煞車。
- **沒有時間**（`car.forward(180)`）：一直做，直到下面任何一個發生，然後兩輪煞車（或換下一個動作）：

  | 什麼時候停 | 怎麼設 |
  |---|---|
  | 下一個動作進來 | 再呼叫任何動作函式 |
  | 馬上停 | `stop()` |
  | 條件成立 | `car.until([] { return ranger.distanceMm() < 200; });`（只看最後給的動作） |
  | 太久沒有新動作（失聯保護） | `setFailsafeMs(600)`：遙控車按住鍵一直送，放開就停 |

- **背景執行**：不想在 loop 裡一直呼叫 `update()`，就交給 `Uno::UnoBackground`，loop 裡可以 `delay`、印字，車子照樣跑：

  ```cpp
  #include "Uno_background.h"
  Uno::UnoBackground::add([] { car.update(); });
  car.forward(180);          // 自己一直開，直到 until、failsafe 或 stop
  ```

  每個函式都會短暫關中斷（幾 µs），所以背景和 loop 同時用不會衝突；`DCMotor`、`Stepper` 也一樣。例子：`src/week_7_L298N/tests/7_background_remote.cpp`（用鍵盤遙控、背景執行）。
- **`until` 的條件在關中斷時執行**：只讀數值（距離、感測器狀態、`car.moveMs()`），不要等、不要印字。
- **排隊**：動作函式只是放進佇列（滿了回傳 false），所以可以一次排好一串（例如正方形：4 ×（直走、右轉 90°）），程式不用等。
- **角度靠時間**：沒有編碼器，轉 90° 要幾 ms 跟車子、地面、電池有關，要在地上量。
- **車上的兩顆馬達是面對面裝的**：同樣是「正」，兩邊常常一個往前一個往後，所以常會有一對是 `BACKWARD`。先把車架高試一次 `forward`。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `drive`、`setSpeed` | -255 ～ 255 | 夾到範圍內 |
| `setMinDuty` | 0 ～ 254 | 大於 254 當成 254 |
| `setRampMs` | 0 ～ 65535 ms | — |
| `Stepper::setSpeed` | 1 ～ 2000 步 / 秒 | 夾到範圍內 |
| `Stepper` 的位置 | ±21 億步 | 之後繞回 |
| EN 的腳位 | PWM 腳（3、5、6、9、10、11） | 編譯錯誤（`UnoPwmPin`） |
| `update()` 的間隔 | 越常越好 | 步進馬達：比步的間隔久時會變慢（`delay` 會讓它停） |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `DCMotor` 約 14 bytes、`Stepper` 約 26 bytes；`Bridge` 沒有 |
| 硬體 | 每個通道 2 隻數位腳 + 1 隻 PWM 腳（或 0 隻，跳線帽接著）；不用中斷 |
| 時間 | 都不會等 |
