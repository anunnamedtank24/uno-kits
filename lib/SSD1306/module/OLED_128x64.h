#ifndef SSD1306_OLED_128X64_H
#define SSD1306_OLED_128X64_H

#include "../basic/datasheet.h"
#include "../basic/frame.h"

namespace SSD1306 {
    // the common 0.96" 128x64 I2C OLED module (GND, VCC, SCL, SDA), SSD1306 with charge pump
    // init: the datasheet's software initialization flow (Rev 1.1, application example),
    // with the COM pins of a 128x64 panel (alternative) and the pre-charge for the charge pump
    template <class Bus>
    class OLED128x64 : public Frame<Bus, 128, 64> {
    public:
        using Frame<Bus, 128, 64>::Frame;

        // composite: Bus init + the init sequence + a cleared panel, then display on
        // flip: false = pins at the top; true = turned 180° (pins at the bottom)
        // returns false if nothing answered at the address
        inline bool init(bool flip = false);
    };

    // ================= definitions =================

    template <class Bus>
    inline bool OLED128x64<Bus>::init(bool flip) {
        Bus::init();
        this->setDisplayOn(false);
        if (!this->ok()) {
            return false;
        }
        this->setMultiplexRatio(64);
        this->setDisplayOffset(0);
        this->setStartLine(0);
        this->setSegmentRemap(!flip);
        this->setComScanRemap(!flip);
        this->setComPins(true, false);
        this->setContrast(0xCF);
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
