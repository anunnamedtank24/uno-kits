#ifndef I2C_OLED_128X32_H
#define I2C_OLED_128X32_H

// a superset of I2C.h: including this file is enough
#include "../I2C.h"

namespace SSD1306 {
    // the 0.91" 128x32 I2C OLED module (GND, VCC, SCL, SDA), SSD1306 with charge pump
    // init: the datasheet's software initialization flow (Rev 1.1, application example),
    // with the COM pins of a 128x32 panel (sequential) and the pre-charge for the charge pump
    template <class Bus>
    class OLED128x32 : public Frame<Bus, 128, 32> {
    public:
        using Frame<Bus, 128, 32>::Frame;

        // composite: Bus init + the init sequence + a cleared panel, then display on
        // flip: false = pins at the top; true = turned 180° (pins at the bottom)
        // returns false if nothing answered at the address
        inline bool init(bool flip = false);
    };

    // ================= definitions =================

    template <class Bus>
    inline bool OLED128x32<Bus>::init(bool flip) {
        Bus::init();
        this->setDisplayOn(false);
        if (!this->ok()) {
            return false;
        }
        this->setMultiplexRatio(32);
        this->setDisplayOffset(0);
        this->setStartLine(0);
        this->setSegmentRemap(!flip);
        this->setComScanRemap(!flip);
        this->setComPins(false, false);
        this->setContrast(0x8F);
        this->setEntireDisplayOn(false);
        this->setInverse(false);
        this->setClock(1, 8);
        this->setPrecharge(1, 15);
        this->setVcomhDeselect(VCOMH_0_77);
        this->setChargePump(true);
        this->setAddressingMode(ADDRESSING_HORIZONTAL);
        this->deactivateScroll();
        // the display RAM is random after power on
        this->clear();
        this->displayAll();
        this->setDisplayOn(true);
        return this->ok();
    }
}

#endif
