# color：介面

| 需要 | 得到 |
|---|---|
| （不需要） | 顏色格式：`color::RGB565`、`BGR565`、`RGB666`、`RGB888`、`BGR888`、`GBR888`、`RGB444`、`RGB332`、`BW<門檻>`，以及任何位元數、任何順序的 `color::Packed<R, G, B, 順序>` |
| （不需要） | 每個格式都有一樣的東西：常用顏色（`RED`、`BLACK`…）、`color(r, g, b)`、`red(c)` / `green(c)` / `blue(c)`、互相換算 |
| （不需要） | 每個格式都能當螢幕的 `ColorFormat`：`ST7789::M130T<Bus, color::RGB565>`，graphics、font、image、video 就都用它畫 |

給要寫新套件、新螢幕、新顏色格式的人。用法見 [README.md](README.md)。`namespace color`。

## 前提條件

| 項目 | 條件 |
|---|---|
| 依賴 | 沒有：不 include 任何函式庫，任何主板都能用 |
| 檔案的顏色 | `convert.py` 的圖片、動畫一律 RGB565；所以每個格式都要有 `fromRGB565()` |
| 計算 | 全部 `constexpr`、`static`；常數在編譯時算好，不佔 RAM |
| 32 bit 運算 | AVR 的 `int` 是 16 bit：中間值可能超過時都先轉 `uint32_t` |

## 檔案

```
lib/color/
├── color.h        入口
├── packed.h       Packed<R, G, B, ORDER>、Order、ValueOf、常用格式的 using、convert
└── BW.h           BW<THRESHOLD>：1 bit 黑白
```

## 提供的介面：ColorFormat

螢幕（`ST7789::Window`、`SSD1306::Frame`）拿它當 template 參數，graphics、font、image、video 從螢幕的 `ColorFormat` 拿顏色。任何有下面這些成員的 struct 都是一個 ColorFormat：

| 成員 | 說明 | 誰用 |
|---|---|---|
| `Value` | 一個顏色的型別（`uint8_t` / `uint16_t` / `uint32_t`） | 全部：參數、緩衝、調色盤 |
| `BITS` | 一個顏色幾 bit（1、8、12、16、18、24…） | 螢幕：選模式、檢查能不能用 |
| `BLACK`、`WHITE`、`RED`、`GREEN`、`BLUE`、`YELLOW`、`CYAN`、`MAGENTA`、`ORANGE`、`GRAY` | 常用顏色 | 程式 |
| `color(r, g, b)` | 0 ～ 255 → `Value` | 程式 |
| `red(c)`、`green(c)`、`blue(c)` | `Value` → 0 ～ 255 | `convert` |
| `fromRGB565(c)` | 檔案的顏色 → `Value` | image、video |
| `fromRGB888(rgb)`、`toRGB888(c)` | `0xRRGGBB` ↔ `Value` | 程式 |
| `R_BITS`、`G_BITS`、`B_BITS`、`ORDER`、`redField(c)`、`greenField(c)`、`blueField(c)` | 每色的位元數、順序、原始位元 | ST7789（12、18 bit 模式一色一色送）；`BW` 沒有 |

| 螢幕 | 接受的 ColorFormat | 不接受時 |
|---|---|---|
| `ST7789::Window`（`M130T`） | `BITS` 12（R4 G4 B4）、16（R5 G6 B5，順序 RGB 或 BGR）、18、24（每色 ≤ 8 bit，任何順序） | 編譯錯誤 |
| `SSD1306::Frame`（`OLED128x64`、`OLED128x32`） | `BITS` 1 | 編譯錯誤 |

## 函式

| 函式 / 常數 | 在哪 | 說明 |
|---|---|---|
| `Packed<R, G, B, ORDER>` | `packed.h` | 每色 R、G、B bit（1 ～ 8），`ORDER` 決定排列，第一個在最高位 |
| `Order` | `packed.h` | `RGB`、`RBG`、`GRB`、`GBR`、`BRG`、`BGR`（數值 0 ～ 5） |
| `ValueOf<BITS>::Type` | `packed.h` | 放得下 BITS bit 的最小無號型別 |
| `convert<To, From>(c)` | `packed.h` | 經過 8 bit 的紅綠藍換算 |
| `expand`（private） | `packed.h` | n bit → 8 bit：`field × 255 ÷ (2ⁿ − 1)`，最大值換成 255 |
| `BW<THRESHOLD>::color(r, g, b)` | `BW.h` | 亮度 `(77 R + 151 G + 28 B) ÷ 256` ≥ THRESHOLD 是 `ON`（1） |
| `BW::OFF`、`ON` | `BW.h` | 0、1 |

`Packed<5, 6, 5, RGB>::fromRGB565` 原樣回傳（編譯器直接拿掉），其他格式經過 8 bit 換算。

## 寫一個新的格式

1. 有固定位元的 RGB：直接 `using MyFormat = color::Packed<…>;`。
2. 其他（灰階、調色盤索引…）：寫一個 struct，至少有 `Value`、`BITS`、常數、`color()`、`red/green/blue()`、`fromRGB565()`、`fromRGB888()`、`toRGB888()`。
3. 要給 ST7789 用：再有 `R_BITS`、`G_BITS`、`B_BITS`、`ORDER`、`redField/greenField/blueField()`，而且 `BITS` 是 12、16、18 或 24。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| `color(r, g, b)` | 各 0 ～ 255 | —（低位被丟掉：例如 RGB565 的紅、藍留 5 bit，綠留 6 bit） |
| `red/green/blue(c)`、`fromRGB565(c)` | 任意值 | —（多的高位被忽略） |
| `Packed` 每個顏色的位元 | 1 ～ 8 | 編譯錯誤 |
| `BW` 的門檻 | 0 ～ 255；0 = 全部亮 | 編譯錯誤（`uint8_t`） |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | 0（全是 `constexpr`） |
| Flash | 只有用到的函式；`fromRGB565` 給 RGB565 是 0 |
| 時間 | 原樣：0；換算：每色一次乘除（AVR 沒有除法器，約幾十個週期） |
