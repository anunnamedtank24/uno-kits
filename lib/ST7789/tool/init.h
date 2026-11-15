#ifndef ST7789_INIT_H
#define ST7789_INIT_H

#include "../basic/datasheet.h"

namespace ST7789 {
    // composite: Bus init + reset + a minimal init sequence for any ST7789 panel
    // optional: a module may have its own sequence (e.g. M130T::init),
    // or the user may call the commands one by one instead
    // works on anything built on Datasheet (Window, M130T, graphics::Shapes<...>)
    template <class Bus>
    inline void initDefault(Datasheet<Bus> &screen, uint8_t blk = 255) {
        Bus::init(blk);

        screen.hwReset();
        screen.swReset();
        screen.slpOut();
        screen.colmod(COLMOD_16BIT);
        screen.madctl(0x00);
        screen.invOn(); // most IPS ST7789 panels need inversion on
        screen.norOn();
        Bus::delayMs(10);
        screen.dispOn();
        Bus::delayMs(10);
    }
}

#endif
