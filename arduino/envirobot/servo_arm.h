#pragma once
#include <Arduino.h>

void initServo();
void armNeutral();       // 90° — both arms up (travel)
void armDeployWater();   // 0°  — Arm A down, turbidity probe in water slot
void armDeploySoil();    // 180° — Arm B down, soil probe on patch
