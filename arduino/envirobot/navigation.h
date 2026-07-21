#pragma once
#include <Arduino.h>
#include "config.h"

void initMotors();

#if ENABLE_AUTONOMOUS
enum FSMState {
  FSM_INIT,
  FSM_SWEEP,        // wander: dead-reckoned heading hold + ultrasonic wall avoidance
  FSM_POND_SAMPLE,
  FSM_POND_BACKOFF, // reverse away so wheels never cross the water slot
  FSM_MEASURE_SOIL,
  FSM_COMPLETE
};
void initNavigation();
void runFSM();
void navigationStart();   // arms FSM_INIT → FSM_SWEEP ({"cmd":"START"} or BOOT button)
#endif

void driveForward(int speed);   // negative = reverse
void driveStop();               // coast — both terminals open (FSM/rest state)
void driveBrake();              // active short-brake — dumps momentum; pulse then driveStop()
void turnLeft(int speed);
void turnRight(int speed);
