#pragma once
#include <Arduino.h>

void   initDataLogger();
void   logPathPoint();
// sector: 1-4 (0 = unknown). RC mode passes the operator-selected sector;
// autonomous mode passes an order-based estimate.
void   logSample(const char* type, float ntu, float pct, int sector);
void   finaliseRun();
String getRunJSON();
