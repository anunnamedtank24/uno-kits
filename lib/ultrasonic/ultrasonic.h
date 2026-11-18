#ifndef ULTRASONIC_H
#define ULTRASONIC_H

// ultrasonic distance modules (trigger / echo), entry point: include this file only
// (no common chip: the boards use different ones, so the folder is named by the kind of part)
//   ranger.h  ultrasonic::Ranger<Trigger, Echo, MIN_MM, MAX_MM>: Trig pulse, Echo timed by its interrupt, mm
// module/  the parts you buy
//   HC_SR04.h  HCSR04<Trigger, Echo>  VCC, Trig, Echo, GND; 2 ~ 400 cm
// tool/  what is often built on it
//   median.h   Median<Ranger, N>: the median of the last N measurements (steadier)
#include "ranger.h"
#include "module/HC_SR04.h"
#include "tool/median.h"

#endif
