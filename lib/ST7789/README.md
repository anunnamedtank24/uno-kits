# ST7789

ST7789 螢幕控制器的驅動，`namespace ST7789`。只依賴一個 Bus，Bus 例如 `Uno::UnoST7789`，所以換主板只要換 Bus。

## 入口

```cpp
#include "module/M130T.h"    // 有對應的模組：只 include 模組，它包含 ST7789.h 的全部
#include "ST7789.h"          // 沒有對應的模組：basic/ + tool/，自己設定尺寸和初始化
```

## 檔案

| 資料夾 | 檔案 | 類別 | 內容 |
|---|---|---|---|
| `basic/` | `datasheet.h` | `ST7789::Datasheet<Bus>` | datasheet 第 9 章所有指令（Table 1、Table 2），照 datasheet 的順序排；外加模組尺寸和偏移量 |
| `basic/` | `window.h` | `ST7789::Window<Bus>` | `setWindow`、`fillWindow`、`writeWindow`，超出螢幕的部分會裁掉 |
| `module/` | `M130T.h` | `ST7789::M130T<Bus>` | 1.3 吋 240×240、沒有 CS 的模組：尺寸 + `init()` 初始化流程 |
| `tool/` | `init.h` | `ST7789::initDefault()` | 適用任何面板的最精簡初始化 |

繼承順序：`Datasheet` → `Window` → `M130T`，再往上是 `graphics::Shapes`、`graphics::Text`。

## Bus 介面

`datasheet.h` 開頭有完整清單。Bus 是一個只有 static 函式的 struct：

- `init`、`setBacklight`、`delayMs`、`nowMs`、`readFlash8/16`
- `cmdMode`、`dataMode`（DC 腳）、`select`、`deselect`（CS 腳）
- `HAS_RES`、`rstHigh`、`rstLow`（RES 腳）
- `write8`、`write16`：可以在 byte 送完之前就返回，但 DC、CS 切換前必須等它送完
- `readMode`、`writeMode`、`readDummy`、`read8`：模組只能寫的話就留空

## 用法

```cpp
using Bus = Uno::UnoST7789<8, 10, Uno::NONE, 9>;
ST7789::M130T<Bus> screen;
screen.init();                               // Bus init + 重置 + M130T 的初始化流程
screen.fillWindow(0, 0, 240, 240, 0xF800);   // 整個螢幕填紅色
```

## 備註

- **NVM 指令**：會燒錄面板的一次性記憶體（OTP），寫了就改不回來，宣告旁有警告。
- **讀取指令**：都有寫，但 UNO 的 Bus 沒有讀取線，所以會回傳 0。
- **座標**：`setWindow` 會加上模組的偏移量（`setOffset`）。M130T 的 240×240 面板位在控制器 240×320 記憶體的左上角。
- **畫面撕裂**：M130T 沒有拉出 TE 腳，沒辦法跟面板刷新同步，大面積變化時可能看得到。
