#ifndef ANALOG_H
#define ANALOG_H

// analog sensors (a voltage that varies, read by the ADC), entry point: include this file only
// (no common chip: named by the kind of part, like switch, buzzer, LED)
//   input.h        analog::Input<AnalogPin, SMOOTHING>: smoothing, range, percent / map, threshold, change
//   joystick.h     Joystick<XPin, YPin, ButtonPin, ...>: 2 axes -100 ~ 100, dead zone, directions, button
//   thermistor.h   Thermistor<AnalogPin, NTC_ON_GND, R0, BETA, FIXED>: NTC temperature (Beta equation)
//   sound_level.h  SoundLevel<AnalogPin, WINDOW_MS>: loudness of a microphone module, claps
// module/  the parts you buy
//   potentiometer.h  Potentiometer<AnalogPin>                knob, slider, trimmer
//   photoresistor.h  Photoresistor<AnalogPin, BRIGHTER_IS_HIGHER>   LDR, KY-018, light sensor boards
//   KY_023.h         KY023<XPin, YPin, ButtonPin>            thumb joystick
//   KY_013.h         KY013<AnalogPin>                        NTC temperature module
//   KY_037.h         KY037<AnalogPin>, KY038<AnalogPin>      sound modules
#include "input.h"
#include "joystick.h"
#include "thermistor.h"
#include "sound_level.h"
#include "module/potentiometer.h"
#include "module/photoresistor.h"
#include "module/KY_023.h"
#include "module/KY_013.h"
#include "module/KY_037.h"

#endif
