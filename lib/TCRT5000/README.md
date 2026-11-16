# TCRT5000

TCRT5000 反射式紅外線感測器（裝在有比較器的小板子上），`namespace TCRT5000`。只依賴一個 Pin（例如 `Uno::UnoPin<PIN>`），換主板只要換 Pin。

## 名稱

| 名稱 | 是什麼 |
|---|---|
| **TCRT5000** | 感測元件：一顆紅外線 LED 和一顆光電晶體並排、朝同一個方向（板子前端那個黑色、兩個圓眼的零件） |
| **LM393** | 板子上的比較器：把光電晶體收到的強弱變成 OUT 的 HIGH / LOW |
| **HW-006** | 板子的型號（零件），放在 `module/HW_006.h`；循跡模組 |

照「`lib/` 用晶片命名」的規則，決定程式怎麼寫的是 TCRT5000（什麼算反射），所以叫 `TCRT5000/`。以後其他 TCRT5000 的板子（KY-033、有 A0 類比輸出的四腳板）放 `module/`。

## 檔案

```
lib/TCRT5000/
├── TCRT5000.h        入口：全部（程式只 include 它）
├── reflector.h       Reflector<Pin, ACTIVE_LOW, FILTER_MS>：反射 / 暗、濾掉抖動、變化次數、時間（模組共用的核心）
└── module/           實際買到的零件
    └── HW_006.h      HW006<Pin>：VCC、GND、OUT 的循跡模組
```

## 原理

| 前面是什麼 | 紅外線 | OUT | 板子上的訊號燈 | `isReflecting()` |
|---|---|---|---|---|
| 白紙、亮的表面，約 1 ～ 25 mm 內 | 反射回來很多 | **LOW** | 亮 | true |
| 黑色（膠帶、墨水）、太遠、什麼都沒有 | 幾乎沒回來 | **HIGH** | 暗 | false（`isDark()`） |

- **藍色的可變電阻**：調「多少反射才算」。轉到放白紙時燈亮、放黑線時燈滅。
- **循跡**：黑線吸收紅外線，所以在線上時 `isDark()` 是 true。
- **怕太陽**：陽光裡有很多紅外線，室外或窗邊會一直是「反射」。
- 這是「有沒有」的感測器，不能量距離。

## 接線

| HW-006 | UNO |
|---|---|
| VCC | 5V（3.3 ～ 5 V 都可以） |
| GND | GND |
| OUT（有些板子印 D0） | 任何數位腳，例如 **D4**；不用上拉（板子自己輸出） |

## 用法

```cpp
#include "Uno_pin.h"
#include "TCRT5000.h"

TCRT5000::HW006<Uno::UnoPin<4>> sensor;   // OUT 接 D4

void setup() {
    sensor.init();
}

void loop() {
    sensor.update();                       // 每次 loop 都要呼叫，不會等
    if (sensor.becameDark()) {
        // 剛碰到黑線（或離開白色的表面）
    }
}
```

完整的例子：`src/week_8/test_1.cpp`（狀態印到串口，UNO 的 L 燈跟著亮）。

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Pin` | `init()`、`read()`（true = HIGH）、`nowMs()` | `Uno::UnoPin` |

## 函式

| 函式 | 說明 |
|---|---|
| `init()` | 設定腳位，讀一次目前的狀態 |
| `update()` | 讀腳位、濾掉抖動；狀態剛改變時回傳 true |
| `isReflecting()` | 前面有白色 / 近的東西 |
| `isDark()` | 相反：黑色、遠、沒東西（在黑線上） |
| `readLevel()` | 腳位現在的電位（沒濾），true = HIGH，用來確認 `ACTIVE_LOW` |
| `becameReflecting()`、`becameDark()` | 剛變成反射 / 暗時回傳 true 一次（呼叫後清掉） |
| `changeCount()` | `init()` 之後變了幾次 |
| `stateMs()` | 目前的狀態維持多久了 |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Pin` | — | `Uno::UnoPin<PIN>`（不開上拉） |
| `ACTIVE_LOW`（`Reflector`） | `true` | 反射時 OUT 是 LOW（HW-006 和大部分板子）；相反的板子用 `Reflector<Pin, false>`（`HW006` 固定是 `true`） |
| `FILTER_MS` | 2 | 新的電位要維持這麼久才算數。在可變電阻的臨界點附近，比較器會快速來回跳；0 = 不濾，變了馬上算 |

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `Pin` 的腳位 | 0 ～ 19；D0 / D1 編譯警告（USB 串口） | 編譯錯誤 |
| `FILTER_MS` | 0 ～ 255 ms | — |
| `update()` 的間隔 | 越常越好；循跡的車子越快越要常呼叫 | 比 `FILTER_MS` 久時，很快掠過的黑線可能會漏掉 |
| `changeCount()` | 0 ～ 65535 | 停在 65535 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | 約 16 bytes |
| 硬體 | 一隻腳；不用中斷 |
