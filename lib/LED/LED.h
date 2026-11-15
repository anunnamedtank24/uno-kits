#ifndef LED_H
#define LED_H

// LEDs on PWM pins, brightness as the eye sees it; entry point: include this file only
//   brightness.h   toDuty, levelToBrightness
//   single_LED.h   SingleLED<Pwm, ACTIVE_HIGH>: one channel (the core of every module); NoPwm: a color not wired
// module/  the parts you buy
//   LED_2_pins.h            LED2Pins<Pwm>                                          one bare LED
//   RGB_LED_4_pins.h        RGBLED4Pins<RedPwm, GreenPwm, BluePwm, COMMON_ANODE>   RGB LED (KY-016 ...)
//   traffic_light_4_pins.h  TrafficLight4Pins<RedPwm, YellowPwm, GreenPwm>         R, Y, G, GND module
#include "brightness.h"
#include "single_LED.h"
#include "module/LED_2_pins.h"
#include "module/RGB_LED_4_pins.h"
#include "module/traffic_light_4_pins.h"

#endif
