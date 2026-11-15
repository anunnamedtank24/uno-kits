#include <Arduino.h>
#include "Uno_pin.h"
#include "Uno_interrupt_pin.h"
#include "Uno_serial.h"
#include "NEC.h"
#include "LED.h"

// UNO + infrared receiver + traffic light module (red, green) + one LED (blue): the remote sets R, G, B
//   receiver:       S -> D2, + -> 5V, - -> GND
//   traffic light:  R -> D3, G -> D5, GND -> GND, Y not connected (the module has its own resistors)
//   blue LED:       long leg (+) -> 220 Ω -> D6, short leg (-) -> GND
//
//   1: R -= 1   2: R = 0   3: R += 1
//   4: G -= 1   5: G = 0   6: G += 1
//   7: B -= 1   8: B = 0   9: B += 1
//   0: R, G, B = 0
// R, G, B are levels 0 ~ LEVELS, shown as brightness the eye sees evenly (LED::toDuty);
// holding 1, 3, 4, 6, 7, 9 keeps stepping (after ~0.3 s, ~9 steps a second)
// every change is also printed to the USB serial: pio device monitor
using Remote = NEC::Remote17KeyNumbersTop;   // or Remote17KeyArrowsTop: see src/week_5/test_1.cpp
constexpr uint8_t LEVELS = 10;                      // steps from off to full
constexpr uint8_t REPEAT_DELAY = 3;                 // repeat codes skipped before a held key repeats

using IrPin = Uno::UnoInterruptPin<2>;
using Receiver = NEC::HX1838<IrPin>;
using Link = Uno::UnoSerial<1000000>;
LED::TrafficLight4Pins<Uno::UnoPwmPin<3>, LED::NoPwm, Uno::UnoPwmPin<5>> trafficLight;   // yellow not used
LED::LED2Pins<Uno::UnoPwmPin<6>> blue;

uint8_t levels[3] = {0, 0, 0};                      // R, G, B

// ---------- text over the serial link ----------

void printText(const char *text) {
    while (*text) {
        Link::write8((uint8_t)*text);
        text++;
    }
}

void printNumber(uint8_t value) {
    if (value >= 10) {
        Link::write8((uint8_t)('0' + value / 10));
    }
    Link::write8((uint8_t)('0' + value % 10));
}

// ---------- color ----------

void show() {
    trafficLight.setRed(LED::levelToBrightness(levels[0], LEVELS));
    trafficLight.setGreen(LED::levelToBrightness(levels[1], LEVELS));
    blue.set(LED::levelToBrightness(levels[2], LEVELS));
    printText("R ");
    printNumber(levels[0]);
    printText("  G ");
    printNumber(levels[1]);
    printText("  B ");
    printNumber(levels[2]);
    printText("\r\n");
}

// key 1 ~ 9 -> color (0 = R, 1 = G, 2 = B) and action (0: -= 1, 1: = 0, 2: += 1); 0: all off
// returns true if something changed
bool apply(NEC::Key key, bool repeat) {
    int8_t digit = NEC::keyDigit(key);
    if (digit < 0) {
        return false;
    }
    if (digit == 0) {
        if (repeat) {
            return false;
        }
        levels[0] = levels[1] = levels[2] = 0;
        return true;
    }
    uint8_t color = (uint8_t)((digit - 1) / 3);
    uint8_t action = (uint8_t)((digit - 1) % 3);
    uint8_t &level = levels[color];
    if (action == 0 && level > 0) {
        level--;
        return true;
    }
    if (action == 1 && !repeat && level > 0) {
        level = 0;
        return true;
    }
    if (action == 2 && level < LEVELS) {
        level++;
        return true;
    }
    return false;
}

void setup() {
    Link::init();
    trafficLight.init();
    blue.init();
    Receiver::init();
    printText("\r\nred D3, green D5 (traffic light), blue D6 (LED), remote on D2\r\n");
    show();
}

void loop() {
    NEC::Code code;
    while (Receiver::read(code, REPEAT_DELAY)) {
        if (apply(Remote::toKey(code), code.repeats > 0)) {
            show();
        }
    }
}
