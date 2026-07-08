#pragma once
#include <Arduino.h>

enum FSMState {
  FSM_INIT,
  FSM_SWEEP,        // wander: IMU heading hold + ToF wall avoidance
  FSM_POND_SAMPLE,
  FSM_POND_BACKOFF, // reverse away so wheels never cross the water slot
  FSM_MEASURE_SOIL,
  FSM_COMPLETE
};

void initMotors();
void initNavigation();
void runFSM();

void driveForward(int speed);   // negative = reverse
void driveStop();
void turnLeft(int speed);
void turnRight(int speed);
