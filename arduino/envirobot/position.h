#pragma once
#include <Arduino.h>

struct Position {
  float x_mm;
  float y_mm;
  float heading_rad;
};

void initPosition();
void updatePosition();
Position getPosition();
void resetPosition();
bool headingValid();   // false if MPU6050 DMP failed — autonomous mode must refuse to drive

// Motion model input — drive code reports commanded wheel speeds
// (mm/s, signed). No encoders on this robot: position is estimated
// from commanded motion + IMU heading.
void setCommandedSpeed(float left_mm_s, float right_mm_s);
