# L298N

| 需要 | 得到 |
|---|---|
| EN 拔掉跳線帽：IN 用輸出腳（`Uno::UnoOutputPin<PIN>`）、EN 用 PWM 腳（`Uno::UnoPwmPin<PIN>`） | `L298N::L298NBoard`：兩個 H 橋 `Board::A`、`Board::B` |
| EN 接著跳線帽：IN 用 PWM 腳（`Uno::UnoPwmPin<PIN>`），或 `L298N::Ground`（接 GND） | `L298N::L298NBoardJumpers`：同上，速度用 IN 的 PWM |
| 上面的 `Board::A`、`Board::B` | `L298N::DCMotor`（速度、漸變、煞車、死區）、`Stepper`（步進馬達）、`TwoWheelCar`（兩輪車，動作排隊執行） |

「需要」是介面（duck typing），括號裡是這個專案現成的實作，換主板只要換它們。開發者看的細節（前提條件、介面需求、每個函式、參數、資源）在 [INTERFACE.md](INTERFACE.md)。

L298N 雙 H 橋馬達驅動，`namespace L298N`。可以接兩顆直流馬達，或一顆雙極步進馬達。

## 用法

```cpp
#include "Uno_pin.h"
#include "L298N.h"

constexpr uint8_t ENA_PIN = 5;      // PWM 腳
constexpr uint8_t IN1_PIN = A0;
constexpr uint8_t IN2_PIN = A1;
constexpr uint8_t IN3_PIN = A2;
constexpr uint8_t IN4_PIN = A3;
constexpr uint8_t ENB_PIN = 6;      // PWM 腳

using Board = L298N::L298NBoard<Uno::UnoPwmPin<ENA_PIN>,
                                Uno::UnoOutputPin<IN1_PIN>, Uno::UnoOutputPin<IN2_PIN>,
                                Uno::UnoOutputPin<IN3_PIN>, Uno::UnoOutputPin<IN4_PIN>,
                                Uno::UnoPwmPin<ENB_PIN>>;
L298N::DCMotor<Board::A> left;      // OUT1、OUT2
L298N::DCMotor<Board::B> right;     // OUT3、OUT4

void setup() {
    left.init();
    right.init();
    left.setSpeed(200);             // 正轉，經過漸變（預設 300 ms 從 0 到 255）
}

void loop() {
    left.update();                  // 每次 loop 都要呼叫，不會等
    right.update();
}
```

步進馬達：

```cpp
L298N::Stepper<Board::A, Board::B> motor;   // 線圈 A 接 OUT1、OUT2，線圈 B 接 OUT3、OUT4

motor.init();
motor.setSpeed(200);                // 步 / 秒
motor.move(400);                    // 往前 400 步；loop 裡呼叫 motor.update()
```

完整的例子：`src/week_7_L298N/tests/1_one_motor.cpp`（一顆）、`2_two_motors.cpp`（兩顆）、`3_voltage.cpp`（給馬達幾伏特）、`4_pwm.cpp`（PWM 是什麼）、`5_stepper.cpp`（步進）、`6_car.cpp`（兩輪車）、`7_background_remote.cpp`（遙控 + 背景執行）。

跳線帽不拔（EN 一直開）：

| 接法 | 用 | 能做的 |
|---|---|---|
| ENA、ENB 跳線帽接著，IN 接普通數位腳 | `L298NBoard<L298N::EnableJumper, IN1, IN2, IN3, IN4, L298N::EnableJumper>` | 只有全速、正反轉、煞車；**不能滑行**（`coast()` 會變成煞車） |
| ENA、ENB 跳線帽接著，IN1 ～ IN4 都接 PWM 腳 | `L298NBoardJumpers<IN1, IN2, IN3, IN4>`（4 個 `UnoPwmPin`） | 可以調速度（「驅動 / 煞車」交替的 PWM），**不能滑行** |

### IN 接 GND（`L298N::Ground`）

IN 不接 UNO、直接接 GND 時，在那個位置放 `L298N::Ground`：一直是 LOW，不佔腳位。

```cpp
// IN2、IN4 接 GND，只用兩隻 PWM 腳（跳線帽接著）
constexpr uint8_t IN1_PIN = 5;
constexpr uint8_t IN3_PIN = 3;
using Board = L298N::L298NBoardJumpers<Uno::UnoPwmPin<IN1_PIN>, L298N::Ground, Uno::UnoPwmPin<IN3_PIN>, L298N::Ground>;
```

| 接 GND 的 | 馬達能 | 反方向時 |
|---|---|---|
| IN2（或 IN4） | 只能 IN1 - IN2 為正的方向 | 煞車 |
| IN1（或 IN3） | 只能 IN1 - IN2 為負的方向 | 煞車 |
| IN1 和 IN2 都接 | 不能轉 | 編譯錯誤 |

- **省腳位**：每顆馬達少一隻腳。只會往前的車（不倒車、不原地旋轉）兩顆馬達只要 2 隻 PWM 腳。
- **兩輪車**：`forward`、`pivotLeft` / `pivotRight`、往前的 `arc` 都可以；`backward`、`spin` 會讓要反轉的輪子煞車。
- `Board::A::BOTH_WAYS`：兩個方向都能轉時是 true（編譯期常數）。
- 時間（`nowMs`、`nowUs`）會自動改從沒接 GND 的那隻腳拿。

## 接線

| L298N | UNO | 說明 |
|---|---|---|
| ENA | **D5** | Timer0 PWM，976 Hz（跳線帽拔掉） |
| IN1 | **A0** | 當數位腳用（`UnoOutputPin<14>`） |
| IN2 | **A1** | `UnoOutputPin<15>` |
| IN3 | **A2** | `UnoOutputPin<16>` |
| IN4 | **A3** | `UnoOutputPin<17>` |
| ENB | **D6** | Timer0 PWM（跳線帽拔掉） |
| GND | GND | 跟電池的 - 接在一起 |
| +12V | — | 電池的 +（6 ～ 12 V） |
| +5V | 不接，或接 UNO 的 5V 供電 | 5V-EN 跳線接著 |

- **為什麼是 D5、D6**：D9、D10 留給蜂鳴器（Timer1）、D3、D11 是 Timer2（D11 被 OLED 用了）。D5、D6 的 Timer0 也管 `millis()`，但只是讀它，不會改頻率，所以沒問題。
- **為什麼 IN 用 A0 ～ A3**：類比腳也能當數位腳，這樣 D2（紅外線）、D4（HW-006）、D7、D8（HC-SR04）、D9（蜂鳴器）、D11、D13（OLED）都還空著，以後可以一起接。

## 名稱

| 名稱 | 是什麼 |
|---|---|
| **L298N** | 晶片：ST 的雙 H 橋（Multiwatt15 封裝，15 隻腳），放在板子的大散熱片上。用晶片命名，所以叫 `L298N/` |
| **H 橋** | 4 個開關圍著馬達排成 H 形，交叉導通就能讓電流往兩個方向流，馬達可以正轉、反轉 |
| **紅色 L298N 模組** | 實際買到的板子（常印 HW-095，不一定有），放在 `module/L298N_board.h` |
| 不是這個函式庫的 | 「L298N mini」小藍板其實是 MX1508 / L9110S 等別的晶片，腳位和用法不同 |

## 板子的腳位

```
        ┌──────────────────────────────────────┐
 OUT1 ──┤                                      ├── OUT4
 OUT2 ──┤             L298N 晶片               ├── OUT3
        │             （散熱片）                │
        │  [5V-EN 跳線]                         │
        │  +12V   GND   +5V      ENA IN1 IN2 IN3 IN4 ENB
        └───┬──────┬─────┬────────┬───┬───┬───┬───┬───┬──┘
          螺絲端子（電源）          排針（ENA、ENB 上有跳線帽）
```

（板子的排列各家不同，以上只是常見的樣子；**照板子上印的字接**。）

| 腳 | 種類 | 功能 | 接到 |
|---|---|---|---|
| **OUT1、OUT2** | 螺絲端子 | 通道 A 的輸出 | 馬達 A 的兩條線（步進：線圈 A） |
| **OUT3、OUT4** | 螺絲端子 | 通道 B 的輸出 | 馬達 B 的兩條線（步進：線圈 B） |
| **+12V**（VS） | 螺絲端子 | 馬達的電源，**5 ～ 35 V**（名字叫 12V，但不一定要 12 V） | 電池 / 變壓器的 +。**不要接 UNO 的 5V** |
| **GND** | 螺絲端子 | 共同的地 | 電池的 - **和 UNO 的 GND，兩個都要接** |
| **+5V** | 螺絲端子 | 5V-EN 跳線**接著**、VS ≤ 12 V：板上穩壓（78M05）**輸出** 5 V | 可以接 UNO 的 5V 腳供電（最多約 0.5 A；UNO 同時接 USB 也沒關係） |
| | | 5V-EN 跳線**拔掉**（VS > 12 V 時一定要拔）：5 V **輸入** | 從 UNO 的 5V 接過來，晶片的邏輯電路要 5 V |
| **ENA** | 排針 | 通道 A 開 / 關，用 PWM 調速度 | 跳線帽接著：一直開（全速）；拔掉：接 PWM 腳，例如 **D5** |
| **IN1、IN2** | 排針 | 通道 A 的方向 | 數位腳，例如 **A0、A1** |
| **IN3、IN4** | 排針 | 通道 B 的方向 | 數位腳，例如 **A2、A3** |
| **ENB** | 排針 | 通道 B 開 / 關，用 PWM 調速度 | 跳線帽接著：一直開；拔掉：接 PWM 腳，例如 **D6** |

- **一定要共地**：IN 和 EN 的 HIGH / LOW 是相對 GND 量的，沒有共地，L298N 看不懂 UNO 的訊號。
- **跳線帽**：ENA、ENB 上的跳線帽把 EN 接到 5 V。要調速度就拔掉，接 PWM 腳；拔下來的帽子收好。
- **電流檢測（SENSE A、SENSE B）**：晶片有，但這塊板子直接接到 GND，所以量不到電流。
- **保護二極體**：板上有 8 顆，馬達斷電時的反衝電壓不會打壞晶片。
- **邏輯電位**：IN、EN 的 HIGH 只要 2.3 V 以上，3.3 V 和 5 V 的板子都能用。

## 真值表

| EN | IN1 | IN2 | OUT1 | OUT2 | 馬達 | 函式 |
|---|---|---|---|---|---|---|
| HIGH | HIGH | LOW | + | - | **正轉** | `drive(> 0)` |
| HIGH | LOW | HIGH | - | + | **反轉** | `drive(< 0)` |
| HIGH | LOW | LOW | - | - | **煞車**：兩端短路，馬達自己的發電把自己擋住，停得快 | `brake()` |
| HIGH | HIGH | HIGH | + | + | 煞車（一樣） | — |
| LOW | 任意 | 任意 | 斷開 | 斷開 | **滑行**：什麼都不接，慢慢停 | `coast()`、`drive(0)` |
| PWM | HIGH | LOW | | | 正轉，速度 = 工作週期 | `drive(duty)` |

通道 B 一樣：ENB、IN3、IN4、OUT3、OUT4。

## 規格和極限

| 項目 | 數值 | 說明 |
|---|---|---|
| 馬達電源 VS | 5 ～ 35 V（板子的電容常是 35 V 的，建議 ≤ 24 V） | 低於約 7 V 時，扣掉壓降後剩不多 |
| 每個通道的電流 | 持續 **2 A**（有散熱片），瞬間 3 A | 馬達卡住（堵轉）時電流最大，要看馬達的堵轉電流 |
| 總功率 | 25 W（晶片） | 很燙就是超過了 |
| **壓降** | 約 1.5 V（0.3 A）～ 2.5 V（1 A），兩個電晶體加起來 | 12 V 電源，馬達最多只拿到約 10 V；變成熱 |
| PWM 頻率 | 晶片可到約 40 kHz；UNO 的 PWM 腳約 976 Hz（D5、D6）/ 490 Hz | 976 Hz 聽得到，馬達會有嗡嗡聲，正常 |
| 邏輯電源 | 5 V，約 36 mA | 板上穩壓或 UNO 的 5V |
| 沒有的功能 | 不會限流、不會量電流、沒有過熱以外的保護 | 步進馬達特別要注意，見下面 |

## 步進馬達要注意

- **怎麼分線圈**：用三用電錶量電阻，有幾 Ω 的兩條是同一個線圈，不通的是不同線圈。線圈 A 接 OUT1、OUT2，線圈 B 接 OUT3、OUT4。接反只會反轉或抖動，不會壞，交換 OUT3、OUT4 就好。
- **L298N 不限流**：線圈的電流 = 電源電壓 ÷ 線圈電阻。常見的 NEMA 17（1.5 Ω、額定 1.7 A）接 12 V 會想吃 8 A，L298N 和馬達都會過熱。
  - 用額定電壓接近電源的馬達（例如 12 V、約 30 Ω 的）。
  - 或用 `setRunDuty` 降低平均電流：約 duty ÷ 255 × 電壓 ÷ 電阻。
  - 真的要用低電阻的步進馬達，換成有限流的驅動（A4988、DRV8825）。
- **28BYJ-48**（5 線的小步進馬達）：不接紅色的中間線，剩下 4 條當雙極馬達接到 L298N 也可以，一圈約 2048 步（FULL），速度上限約 500 步 / 秒。
- **沒有加速**：`setSpeed` 是一開始就用這個速度。要跑快，先用慢的速度起動，再一步步調快（`src/week_7_L298N/tests/5_stepper.cpp` 的速度階梯就是這樣）。

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
