#pragma once
#include <Arduino.h>

void   initSensors();
// rawOut (optional) receives the pre-curve median ADC value from the same
// sampling window — the diag page shows it so a clamped 0 NTU can be told
// apart from genuinely clear water (both read 0 through the curve).
float  readTurbidityNTU(float* rawOut = nullptr);
float  readSoilMoisturePct();
// HC-SR04 ping → distance in mm. Returns a large value (no obstacle) on
// timeout. This is the only obstacle sensor — collision detection only,
// never used for navigation or zone detection.
float  readObstacleDistanceMM();

// Calibration wizard captures (POST /cal/capture) — same settle→window→median
// path a real sample uses, raw pre-scaling values, does not write config.
float  captureRawSoil();        // PIN_SOIL_SENSE, used for both dry/wet steps
float  captureRawTurbidity();   // PIN_TURBIDITY, clear-water zero point
