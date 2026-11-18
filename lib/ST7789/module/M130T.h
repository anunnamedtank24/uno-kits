#ifndef ST7789_M130T_H
#define ST7789_M130T_H

#include "../basic/datasheet.h"
#include "../basic/window.h"
#include "../tool/init.h"

namespace ST7789 {
    // M130T: 1.3" IPS 240x240 module, ST7789 driver, no CS pin (SPI_MODE3)
    // the 240x240 panel sits at the top-left of the 240x320 controller RAM
    //
    // init values are the common vendor values for 1.3" 240x240 ST7789 modules;
    // replace them with the values from the M130T documentation if they differ
    //
    // ColorFormat: the pixels' format, see Window (color::RGB565 is the fastest; RGB888 / RGB666 = 18 bit,
    // RGB444 = 12 bit); init() sets COLMOD and the color order for it
    template <class Bus, class ColorFormat>
    class M130T : public Window<Bus, ColorFormat> {
    public:
        inline M130T();

        // composite: Bus init + reset + the module's init sequence
        inline void init(uint8_t blk = 255);
    };

    // ================= definitions =================

    template <class Bus, class ColorFormat>
    inline M130T<Bus, ColorFormat>::M130T()
        : Window<Bus, ColorFormat>(240, 240) {
    }

    template <class Bus, class ColorFormat>
    inline void M130T<Bus, ColorFormat>::init(uint8_t blk) {
        Bus::init(blk);

        this->hwReset();
        this->swReset();
        this->slpOut();

        this->madctl(Window<Bus, ColorFormat>::MADCTL_ORDER);
        this->colmod(Window<Bus, ColorFormat>::COLMOD_VALUE);

        this->porCtrl(0x0C, 0x0C, 0x00, 0x33, 0x33);
        this->gCtrl(0x35);
        this->vcomS(0x19);
        this->lcmCtrl(0x2C);
        this->vdvVrhEn(true);
        this->vrhS(0x12);
        this->vdvS(0x20);
        this->frCtrl2(0x0F); // 60 Hz
        this->pwCtrl1(0xA1);

        const uint8_t pvGamma[14] = {
            0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F,
            0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23
        };
        this->pvGamCtrl(pvGamma);

        const uint8_t nvGamma[14] = {
            0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F,
            0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23
        };
        this->nvGamCtrl(nvGamma);

        this->invOn(); // IPS panel
        this->norOn();
        Bus::delayMs(10);
        this->dispOn();
        Bus::delayMs(10);
    }
}

#endif
