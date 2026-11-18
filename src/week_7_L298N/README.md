# week_7_L298N

L298N 馬達驅動（[lib/L298N](../../lib/L298N/README.md)）的測試：一顆、兩顆直流馬達、控制電壓、PWM、步進馬達。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python src/hide.py week_7_L298N/tests/1_one_motor     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
pio device monitor                      # 1 Mbaud
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `tests/1_one_motor.cpp` | **一顆**馬達：漸變、滑行和煞車的差別、反轉、速度階梯 | L298N + 直流馬達（OUT1、OUT2） |
| `tests/2_two_motors.cpp` | **兩顆**馬達（兩輪車）：各自確認、直走、後退、轉彎、原地旋轉 | L298N + 2 顆直流馬達 |
| `tests/3_voltage.cpp` | **控制電壓**：在串口打 `a 4.5`，馬達 A 就拿到約 4.5 V | L298N + 1 ～ 2 顆直流馬達 |
| `tests/4_pwm.cpp` | **PWM**：佔空比由小到大（從哪裡開始轉）、由大到小（在哪裡停）、起動的「踢一下」、PWM 煞車 | L298N + 直流馬達 |
| `tests/5_stepper.cpp` | **步進馬達**：FULL、HALF、WAVE 各轉一圈來回，速度階梯找上限 | L298N + 雙極步進馬達（4 條線） |
| `tests/6_car.cpp` | **兩輪車**（`TwoWheelCar`）：直走、後退、原地旋轉、單輪旋轉、弧線、正方形、8 字、之字 | L298N（**EN 跳線帽接著**）+ 2 顆直流馬達 |
| `tests/7_background_remote.cpp` | **背景遙控**：車在背景跑（`UnoBackground`），用鍵盤操作；沒有時間的動作、`until`、失聯自動停 | 同 `tests/6_car` |

## 接線

`tests/1_one_motor` ～ `tests/5_stepper` 的接法一樣（只接一顆馬達的測試，通道 B 就空著）；`tests/6_car` 不同，見 [tests/6_car](#tests6_carcpp兩輪車)。

| 零件 | 腳 | UNO / 其他 |
|---|---|---|
| L298N | ENA（**拔掉跳線帽**） | **D5** |
| | IN1 | **A0** |
| | IN2 | **A1** |
| | IN3 | **A2** |
| | IN4 | **A3** |
| | ENB（**拔掉跳線帽**） | **D6** |
| | +12V | 電池 / 變壓器的 +（6 ～ 12 V，看馬達） |
| | GND | 電池的 - **和** UNO 的 GND |
| | +5V | 不接（5V-EN 跳線接著） |
| | OUT1、OUT2 | 馬達 A（步進：線圈 A） |
| | OUT3、OUT4 | 馬達 B（步進：線圈 B） |
| USB | D0、D1 | 串口 |

| 腳 | 為什麼選它 |
|---|---|
| D5、D6 | PWM 腳裡不跟蜂鳴器（D9、Timer1）、OLED（D11）衝突的 |
| A0 ～ A3 | 類比腳當數位腳用，D2、D4、D7 ～ D13 都留給其他週的零件 |

- **馬達不要用 UNO 的 5V 供電**：馬達起動電流很大，會讓 UNO 重開機或燒壞 USB。
- **第一次測試**：車子架高，輪子懸空。

# tests/1_one_motor.cpp：一顆馬達

```
1 forward 255: ramps up in 1 s
2 coast: rolls out slowly
3 forward 255 again
4 brake: stops at once (compare with 2)
5 backward 160
5 forward 160: through 0 by the ramp, no jolt
6 speed ladder: 40 (turning? if not: the dead zone, see tests/4_pwm)
...
```

- **2 和 4 比一比**：滑行是慢慢停，煞車是馬上停。
- **5**：從反轉直接設成正轉，漸變會先減到 0 再加上去，不會猛然反轉。
- **6**：速度 40、80 時可能只會嗡嗡叫，那是死區，用 `MIN_DUTY` 補（見 `tests/4_pwm`）。
- 轉的方向反了：交換馬達的兩條線，或 `REVERSED = true`。

# tests/2_two_motors.cpp：兩顆馬達

A 是左輪、B 是右輪。先看 1、2：只有一個輪子轉、而且往前，確認哪個是哪個。

| 步驟 | 左 | 右 | 車子 |
|---|---|---|---|
| 3 | 200 | 200 | 直走 |
| 4 | -200 | -200 | 後退 |
| 5 | 200 | 100 | 往右的弧線 |
| 6 | 100 | 200 | 往左的弧線 |
| 7 | 200 | 煞車 | 以右輪為中心轉 |
| 8 | 200 | -200 | 原地右轉 |
| 9 | -200 | 200 | 原地左轉 |
| 10 | 255 | 255 | 看兩顆馬達一不一樣快（通常有點不同，用速度微調） |

- 車上的兩顆馬達是面對面裝的，所以通常有一顆要反過來：`LEFT_REVERSED` 或 `RIGHT_REVERSED = true`。

# tests/3_voltage.cpp：控制電壓

在 `pio device monitor` 打一行再按 Enter：

| 輸入 | 做什麼 |
|---|---|
| `a 4.5` | 馬達 A 正轉，約 4.5 V |
| `a -3` | 馬達 A 反轉，約 3 V |
| `b 6` | 馬達 B，約 6 V |
| `x 5` | 兩顆都 5 V |
| `a 0` | 馬達 A 滑行 |
| `s` | 兩顆都煞車 |
| `?` | 印出設定 |

```
> a 4.5
A: duty 115 / 255 (45 %) -> about 4.51 V
```

- **L298N 不能真的降壓**：它把電源開開關關（PWM），馬達平均起來拿到 `佔空比 × (電源 - 壓降)`。
- 設定要改成你的：`SUPPLY_V`（電源，用電錶量；9 V 電池接上馬達後常常只剩約 7 V）、`DROP_V`（L298N 吃掉的，約 2 V）、`MOTOR_MAX_V`（馬達的額定電壓，超過的會被限制）。
- 用電錶的直流電壓量 OUT1、OUT2，大約就是印出來的值（PWM 會讓有些電錶不太準）。

# tests/4_pwm.cpp：PWM

只用馬達 A，四個部分一直重複：

1. **佔空比 0 → 255**（每 0.4 秒加 5）：記下**開始轉**的佔空比。
2. **255 → 0**：記下**停下來**的佔空比，會比 1 低（已經在轉的馬達比較容易繼續轉）。
3. **踢一下**：低佔空比（`LOW_DUTY`，設成比 1 的值低一點）直接起動可能只會嗡嗡叫；先 255 跑 100 ms 再降下來，就能起動而且繼續轉。
4. **PWM 煞車**：全速後分別滑行、`brake(64)`、`brake(128)`、`brake(255)`，停下來的距離越來越短。

- 1 的值就是 `DCMotor::setMinDuty` 該設的值（`tests/1_one_motor`、`tests/2_two_motors` 的 `MIN_DUTY`）。
- **嗡嗡聲**：UNO 的 PWM 是 976 Hz，在人耳聽得到的範圍，正常。

# tests/5_stepper.cpp：步進馬達

雙極步進馬達（4 條線）：用電錶找出兩個線圈（有幾 Ω 的兩條是一組），線圈 A 接 OUT1、OUT2，線圈 B 接 OUT3、OUT4。

```
FULL (2 coils, most torque): a turn forward, then back (200 steps at 100 steps/s)
  at 200
  at 0
HALF (smoother, twice the steps): ...
speed ladder (FULL): the last speed that turns is the limit
  50 steps/s = 15 rpm
  ...
```

- **一直抖、不轉**：一個線圈接反，交換 OUT3、OUT4。
- **速度階梯**：最後還在轉的速度就是這個電源下的上限；超過會失步（嗡嗡叫不轉）。
- **很燙**：降低 `RUN_DUTY`。L298N 不限流，低電阻的步進馬達（例如 1.5 Ω 的 NEMA 17）接 12 V 電流會太大，見 [lib/L298N](../../lib/L298N/README.md#步進馬達要注意)。
- `STEPS_PER_TURN`：1.8° 的馬達是 200；28BYJ-48 約 2048。

# tests/6_car.cpp：兩輪車

**接法跟其他測試不同**：只接 IN1 ～ IN4、OUT1 ～ OUT4、+12V、GND，**ENA、ENB 的跳線帽接著**。速度改由 IN 腳的 PWM 控制，所以 IN1 ～ IN4 都要接 PWM 腳。

| L298N | UNO / 其他 |
|---|---|
| IN1 | **D5** |
| IN2 | **D6** |
| IN3 | **D3** |
| IN4 | **D11** |
| ENA、ENB | 不接（跳線帽接著） |
| OUT1、OUT2 | 右馬達 |
| OUT3、OUT4 | 左馬達 |
| +12V | 電池的 + |
| GND | 電池的 - **和** UNO 的 GND |

- **為什麼是 D3、D5、D6、D11**：UNO 只有 6 隻 PWM 腳；D9、D10（Timer1）留給蜂鳴器。D11 跟 OLED 的 SDA 是同一隻，所以這個測試不能同時接 OLED。

接線方式寫成型態，一行就說完：

```cpp
//                                    IN1 / IN2     IN1 - IN2 > 0    IN3 - IN4 > 0
using Car = L298N::TwoWheelCar<Board, L298N::RIGHT, L298N::FORWARD, L298N::FORWARD>;
```

- **IN1 - IN2 為正**（IN1 HIGH、IN2 LOW）：右馬達向前。
- **IN3 - IN4 為正**：左馬達向前。
- 輪子方向相反：把那一對的 `FORWARD` 改成 `BACKWARD`。左右相反：`RIGHT` 改成 `LEFT`。
- **IN 接 GND**：例如 IN2、IN4 直接接 GND，只用兩隻 PWM 腳，設定改成 `IN2_PIN = GROUND`、`IN4_PIN = GROUND`。`tests/6_car` 目前的設定就是這樣（IN1 = D5、IN3 = D6）；照上表接四隻腳時，改回 `IN1_PIN = 5`、`IN2_PIN = 6`、`IN3_PIN = 3`、`IN4_PIN = 11`。輪子只能往前：後退和原地旋轉（動作 2、3、4、9）會變成煞車。

| 動作 | 內容 |
|---|---|
| 1、2 | 直走、後退 1.5 秒：**先架高確認兩輪都往前、都往後** |
| 3、4 | 原地右轉、左轉 90° |
| 5、6 | 單輪旋轉：右輪不動 / 左輪不動 |
| 7、8 | 弧線：左快右慢（往右）、右快左慢（往左），各半圈 |
| 9 | 正方形：4 ×（直走 1 秒、右轉 90°），一次排進佇列 |
| 10 | 8 字：往右一圈、往左一圈 |
| 11 | 之字：兩輪速度每 0.6 秒交換 |
| 12 | 直走後滑行（跳線帽接著，會變成煞車） |

設定裡的參數，要在地上量：

| 參數 | 預設 | 說明 |
|---|---|---|
| `SPEED` | 180 | 所有動作的速度 |
| `SPIN_90_MS` | 450 | 原地轉 90° 要幾 ms：看 3 轉了多少再調，正方形才會閉合 |
| `ARC_360_MS` | 3000 | 弧線轉一整圈要幾 ms：8 字才會閉合 |
| `ARC_DIFFERENCE` | 120 | 弧線兩輪的速差，越大彎越急 |
| `LEFT_TRIM`、`RIGHT_TRIM` | 100、100 | 直走會偏：把快的那邊調低（%） |
| `MIN_DUTY` | 0 | 馬達的死區（`tests/4_pwm`） |

# tests/7_background_remote.cpp：背景遙控

接法跟 `tests/6_car` 一樣（EN 跳線帽接著，IN1 ～ IN4 = D5、D6、D3、D11）。`pio device monitor` 打開後直接按鍵（不用 Enter）：

| 鍵 | 動作 |
|---|---|
| `w` / `s` | 前進 / 後退 |
| `a` / `d` | 原地左轉 / 右轉 |
| `q` / `e` | 往左 / 往右的弧線 |
| 空白、`x` | 停 |
| `f` | 前進，直到走了 2 秒（`until`） |
| `t` | 前進 1 秒、再右轉 0.5 秒（有時間、排隊） |

- **背景**：`Uno::UnoBackground::add([] { car.update(); })`，`loop()` 只讀鍵盤、印速度，還有 `delay(500)`，車子照樣加減速、換動作。
- **沒有時間的動作**：`w`、`a` 這些一直做。
- **失聯自動停**（`setFailsafeMs(600)`）：放開鍵 0.6 秒後自動煞車。按住鍵時終端機會一直重送，所以會繼續走，就像遙控器。
