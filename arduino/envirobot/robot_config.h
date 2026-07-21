#pragma once
#include <Arduino.h>

// Runtime-tunable values — compiled-in defaults here equal the old
// #define values. Overridden by /config.json at boot (config_store.cpp).
// Clamped on every load and every POST — never trust a raw value into RAM.
struct RobotConfiguration {
  // ── Calibration outputs (wizard-owned) ──────────────────────────
  int   soilDryVal        = 2850;
  int   soilWetVal        = 1200;
  int   turbidityZeroRaw  = 0;      // clear-water ADC baseline, 0 = uncalibrated
  float mmPerSecAtDrive   = 150.0f;
  float wheelBaseMm       = 150.0f;
  int   waterClearMin     = 800;
  float waterRedRatioMax  = 0.40f;
  float soilRedRatioMin   = 0.45f;
  float soilBlueRatioMax  = 0.20f;

  // ── Tunable by hand ─────────────────────────────────────────────
  int      driveSpeed        = 255;
  int      turnSpeed         = 255;
  int      wallStopMm        = 120;
  uint32_t sampleSettleMs    = 2000;
  uint32_t sampleWindowMs    = 3000;
  int      servoNeutral      = 90;
  int      servoWater        = 0;
  int      servoSoil         = 180;
  float    headingDeadbandRad = 0.20f;
};
extern RobotConfiguration config;
