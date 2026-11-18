# week_2_LED

LED（[lib/LED](../../lib/LED/README.md)）的測試：亮度怎麼換算、每一種效果（閃爍、呼吸、漸變、心跳）。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python src/hide.py week_2_LED/tests/1_effects     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
pio device monitor                      # 1 Mbaud
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `tests/1_effects.cpp` | 一顆 LED：亮度分段、閃爍、呼吸、漸變、心跳，一項一項跑 | LED + 220 Ω |

其他用到 LED 的程式：`week_4_SSD1306_switch_buzzer_audio/tests/6_clicks`（按鈕手勢換效果）、`week_5_NEC/assignments/1_rgb_remote`（遙控器調紅、綠、藍）。

## 接線

| LED | UNO |
|---|---|
| 長腳（+） | 220 Ω → **D6** |
| 短腳（-） | GND |

- **一定要串電阻**：LED 自己不限流，直接接腳位會燒掉 LED 或腳位。
- **要 PWM 腳**（3、5、6、9、10、11）才能調亮度；換腳改 `LED_PIN`。

# tests/1_effects.cpp：LED 的每種效果

```
LED on D6
test 0: steady steps
  steady   0
  steady  31
  ...
test 1: blink
  done
test 2: breathe
test 3: fade
test 4: heartbeat
```

| 測試 | 應該看到 |
|---|---|
| 0 亮度分段 | 0 ～ 255 分 `STEPS` 段，每段看起來都比前一段亮一點、而且平均 |
| 1 閃爍 | 閃 `BLINK_TIMES` 下後熄滅 |
| 2 呼吸 | 平順地變亮變暗，一次 `BREATHE_MS` |
| 3 漸變 | `FADE_MS` 內從暗到最亮，再回到暗 |
| 4 心跳 | 兩下短跳、休息，重複 |

- **亮度是眼睛看的**：0 ～ 255 看起來均勻變亮，所以 128 看起來「一半亮」，PWM 的工作週期其實只有 25 %（duty = brightness² ÷ 255 = 64）。
- **效果不會等**：`effect.blink(...)` 馬上回來，要一直呼叫 `effect.update()`（這裡的 `run(ms)` 就是在做這件事）。
- **只跑某幾個**：`ONLY_TESTS`，例如 `(1UL << 2)` 只跑呼吸。
