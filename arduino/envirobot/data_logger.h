#pragma once
#include <Arduino.h>

void   initDataLogger();
void   startRun();        // reset doc + start run clock: first RC drive/sample, or autonomous START
bool   runLive();         // true between startRun() and finaliseRun()
bool   isRunActive();     // same flag — config editor / cal wizard lockout
float  runElapsedS();     // seconds since startRun (0 when not live)
void   logPathPoint();
// sector: 1-4 (0 = unknown). RC mode passes the operator-selected sector;
// autonomous mode passes a dead-reckoned quadrant estimate (SECTOR_MAP).
void   logSample(const char* type, float ntu, float pct, int sector);
void   finaliseRun();
String getRunJSON();
String getSamplesJSON();   // samples only, no path array — live feed for tools/live_plot.py
