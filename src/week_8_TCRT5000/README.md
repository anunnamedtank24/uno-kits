# week_8_TCRT5000

循跡模組 HW-006（TCRT5000，[lib/TCRT5000](../../lib/TCRT5000/README.md)）的測試。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python src/hide.py week_8_TCRT5000/tests/1_line_tracking     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
pio device monitor                      # 1 Mbaud
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `tests/1_line_tracking.cpp` | 反射 / 暗印到串口 | HW-006 |

## 接線

| 零件 | 腳 | UNO |
|---|---|---|
| HW-006 | VCC | 5V |
| | GND | GND |
| | OUT | **D4** |
| USB | D0、D1 | 串口 |

| 腳 | 為什麼選它 |
|---|---|
| D4 | 只是讀高低，任何腳都可以；D4 只有 week_1 的搖桿按鈕用，跟 L298N、HC-SR04、紅外線、OLED、蜂鳴器都不衝突 |

# tests/1_line_tracking.cpp：循跡模組 HW-006

```
HW-006 on D4: white / near = reflecting, black / far = dark
reflecting  (OUT LOW,  change 0)
dark        (OUT HIGH, change 1)    was reflecting 1250 ms
still dark        (OUT HIGH, change 1)
```

1. 模組前端朝下，離白紙 1 ～ 2 cm：應該是 `reflecting`，HW-006 上的訊號燈亮。
2. 移到黑色膠帶或墨水上（或拿遠）：變成 `dark`，訊號燈滅。
3. 不會變：轉藍色的可變電阻，直到白紙亮、黑線滅。
4. **剛好相反**（白紙是 `dark`）：這塊板子反射時 OUT 是 HIGH，設定改成 `ACTIVE_LOW = false`。
5. 每 2 秒印一行 `still …`，就算沒變化也看得到現在的電位。
