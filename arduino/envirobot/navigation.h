#pragma once
#include <Arduino.h>
#include "config.h"

// Motor layer only. The autonomous FSM was deleted — the robot is
// remote-controlled, so every motion command originates from an operator
// tap on the RC page (wifi_server.cpp).

void initMotors();

void driveForward(int speed);   // negative = reverse
void driveStop();               // coast — both terminals open (rest state)
void driveBrake();              // active short-brake — dumps momentum; pulse then driveStop()
void turnLeft(int speed);
void turnRight(int speed);
