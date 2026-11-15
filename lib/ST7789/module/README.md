# ST7789/module

一個檔案對應一種螢幕模組。模組是 `ST7789.h` 的父集：使用者只要 include 模組，就有 ST7789 的全部。

| 檔案 | 模組 |
|---|---|
| `M130T.h` | 1.3 吋 IPS 240×240，沒有 CS（SPI mode 3） |

## 加一個新模組

模組要決定三件事：面板大小、在控制器 240×320 記憶體裡的偏移量，以及初始化流程。照 `M130T.h` 的樣子：

```cpp
#ifndef ST7789_XXX_H
#define ST7789_XXX_H

#include "../ST7789.h"   // 父集：include 這個檔案就夠了

namespace ST7789 {
    template <class Bus>
    class XXX : public Window<Bus> {
    public:
        inline XXX();
        inline void init(uint8_t blk = 255);   // Bus init + 重置 + 模組的初始化流程
    };

    // ================= definitions =================

    template <class Bus>
    inline XXX<Bus>::XXX()
        : Window<Bus>(240, 280) {   // 面板大小
    }

    template <class Bus>
    inline void XXX<Bus>::init(uint8_t blk) {
        Bus::init(blk);
        this->hwReset();
        this->swReset();
        this->slpOut();
        this->setOffset(0, 20);     // 面板不在記憶體左上角時
        this->colmod(COLMOD_16BIT);
        // ... 廠商給的電壓、gamma 設定
        this->dispOn();
    }
}

#endif
```

- **檔名**：用模組的型號，維持原本的大小寫（例如 `M130T.h`）。
- **數值來源**：模組的規格書或廠商範例程式；沒有的話先用 `tool/init.h` 的 `initDefault()` 試試看。
- **IPS 面板**：通常要 `invOn()`，否則顏色會反相。
- **驗證**：跑 `src/week_3/test_1.cpp` 的 `tests[0]`、`tests[1]`，看顏色順序和邊框有沒有貼齊四邊。
