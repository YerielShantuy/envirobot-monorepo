#pragma once
#include <Arduino.h>

void   initSensors();
float  readTurbidityNTU();
float  readSoilMoisturePct();
bool   isWaterZone();
bool   isSoilPatch();
// One TCS34725 read (~24ms blocking) classifying both zone types —
// use this in the drive loop instead of two separate is*() calls.
void   readZones(bool* water, bool* soil);
// HC-SR04 ping → distance in mm. Returns a large value (no wall) on timeout.
float  readWallDistanceMM();

// Calibration wizard captures (POST /cal/capture) — same settle→window→median
// path a real sample uses, raw pre-scaling values, does not write config.
float  captureRawSoil();        // PIN_SOIL_SENSE, used for both dry/wet steps
float  captureRawTurbidity();   // PIN_TURBIDITY, clear-water zero point
void   captureRawColor(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* c);
