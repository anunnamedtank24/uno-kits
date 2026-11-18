#ifndef ANALOG_KY_037_H
#define ANALOG_KY_037_H

#include "../sound_level.h"

namespace analog {
    // KY-037 (big microphone) / KY-038 (small microphone) sound modules: a microphone and a comparator (LM393)
    // 4 pins: AO -> an analog pin (Uno::UnoAnalogPin<A0 ~ A5>), G -> GND, + -> 5V,
    //         DO -> a digital pin (louder than the potentiometer's point: read it with lib/switch, Switch3Pins)
    // turn the potentiometer until middle() is near 512 (and the module's L2 LED just goes off)
    template <class AnalogPin, uint16_t WINDOW_MS = 50>
    using KY037 = SoundLevel<AnalogPin, WINDOW_MS>;

    template <class AnalogPin, uint16_t WINDOW_MS = 50>
    using KY038 = SoundLevel<AnalogPin, WINDOW_MS>;
}

#endif
