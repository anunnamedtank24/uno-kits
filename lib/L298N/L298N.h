#ifndef L298N_H
#define L298N_H

// L298N dual H-bridge motor driver, entry point: include this file only
//   bridge.h     L298N::Bridge<In1, In2, Enable>: one H-bridge, any wiring (pin, PWM pin, Ground, Supply / EnableJumper);
//                InputPwmBridge<In1, In2> = Bridge<In1, In2, EnableJumper>
//   dc_motor.h   DCMotor<Bridge>: a DC motor, speed -255 ~ 255, ramp, dead zone, brake / coast
//   stepper.h    Stepper<BridgeA, BridgeB>: a bipolar stepper on both bridges, WAVE / FULL / HALF, steps / s
// module/  the parts you buy
//   L298N_board.h  L298NBoard<Ena, In1, In2, In3, In4, Enb, SUPPLY_MV, JUMPER_5V>, L298NBoardJumpers<In1 ~ In4, ...>,
//                  L298NBoardParallel<Enable, In1, In2, ...>: the red board, every wiring, the 5V-EN jumper
// device/  things built with it
//   two_wheel_car.h  TwoWheelCar<Board, IN12_SIDE, IN12_POSITIVE, IN34_POSITIVE>: one way of wiring a car,
//                    forward, spin, pivot, arc ... with times, queued
#include "bridge.h"
#include "dc_motor.h"
#include "stepper.h"
#include "module/L298N_board.h"
#include "device/two_wheel_car.h"

#endif
