#ifndef ANALOG_PHOTORESISTOR_H
#define ANALOG_PHOTORESISTOR_H

#include "../input.h"

namespace analog {
    // a photoresistor (LDR, light dependent resistor): less resistance in more light
    //
    // a bare LDR needs a divider: 5V -- LDR -- (pin) -- 10 kΩ -- GND   brighter = higher reading
    // a module (KY-018 and the 3 / 4-pin "light sensor" boards) has the resistor on it; on many of them it is
    // the other way round (brighter = lower): BRIGHTER_IS_HIGHER = false (try it: cover it with a finger)
    // a module's DO pin (4-pin boards) is "darker than the potentiometer's point", on / off: read it with lib/switch
    //
    //   brightness(): 0 ~ 100 (dark ~ bright) over the range (setRange / startLearning for this room)
    //   setThreshold: e.g. a night light, on when it gets dark
    template <class AnalogPin, bool BRIGHTER_IS_HIGHER = true, uint8_t SMOOTHING = 3>
    class Photoresistor : public Input<AnalogPin, SMOOTHING> {
    public:
        inline void init() {
            Input<AnalogPin, SMOOTHING>::init();
            if (!BRIGHTER_IS_HIGHER) {
                this->setRange(AnalogPin::MAX, 0);
            }
        }

        // 0 = dark ~ 100 = bright, over the range
        inline uint8_t brightness() const {
            return this->percent();
        }
    };
}

#endif
