#pragma once
#include <Arduino.h>

void   initSensors();
float  readTurbidityNTU();
float  readSoilMoisturePct();
bool   isWaterZone();
bool   isSoilPatch();
float  readToFDistanceMM();
