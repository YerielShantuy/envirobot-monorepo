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
void resetPose(float heading_rad);  // re-zero x/y at a known heading
bool headingValid();   // always true — heading is open-loop dead reckoned (no IMU)

// Motion model input — drive code reports commanded wheel speeds
// (mm/s, signed). No encoders and no IMU: position AND heading are
// estimated from commanded motion via the differential-drive model.
void setCommandedSpeed(float left_mm_s, float right_mm_s);
