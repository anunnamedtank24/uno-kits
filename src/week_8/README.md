# week_8

循跡模組 HW-006（TCRT5000，[lib/TCRT5000](../../lib/TCRT5000/README.md)）的測試。

全部的檔案現在都是 `.bak`，要跑哪個就留下哪個：

```bash
python -B src/hide.py week_8/test_1     # 只留下這個（拿掉 .bak），其他改成 .bak
pio run -t upload
pio device monitor                      # 1 Mbaud
```

| 檔案 | 內容 | 需要的硬體 |
|---|---|---|
| `test_1.cpp` | 反射 / 暗印到串口，UNO 的 L 燈跟著亮 | HW-006 |

## 接線

| 零件 | 腳 | UNO |
|---|---|---|
| HW-006 | VCC | 5V |
| | GND | GND |
| | OUT | **D4** |
| USB | D0、D1 | 串口 |

| 腳 | 為什麼選它 |
|---|---|
| D4 | 只是讀高低，任何腳都可以；D4 沒有被其他週的零件用 |
| D13（L 燈） | 板子上的燈，不用接線；跟 OLED 的 SCL 是同一隻腳，所以這個測試不能接 OLED |

# test_1.cpp

```
HW-006 on D4: white / near = reflecting, black / far = dark
reflecting  (OUT LOW,  change 0)
dark        (OUT HIGH, change 1)    was reflecting 1250 ms
still dark        (OUT HIGH, change 1)
```

1. 模組前端朝下，離白紙 1 ～ 2 cm：應該是 `reflecting`，板子上的訊號燈和 UNO 的 L 燈都亮。
2. 移到黑色膠帶或墨水上（或拿遠）：變成 `dark`，兩個燈都滅。
3. 不會變：轉藍色的可變電阻，直到白紙亮、黑線滅。
4. **剛好相反**（白紙是 `dark`）：這塊板子反射時 OUT 是 HIGH，程式改用 `TCRT5000::Reflector<Uno::UnoPin<4>, false>`。
5. 每 2 秒印一行 `still …`，就算沒變化也看得到現在的電位。
