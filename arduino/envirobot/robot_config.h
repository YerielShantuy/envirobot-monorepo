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

  // ── Tunable by hand ─────────────────────────────────────────────
  int      driveSpeed        = 255;
  // Collision guard master switch. ON by default — it is a safety feature —
  // but it is optional: turn it off when the ultrasonic gets in the way
  // (tight manoeuvres against a wall, sampling right up against an edge, or
  // a sensor that is misreading the arena). Distance is still measured and
  // reported with the guard off; only the motor cut and the FWD refusal stop.
  bool     collisionGuardOn  = true;
  // Collision threshold: forward drive is cut when the HC-SR04 reads closer
  // than this. Calibrate against the real arena wall + the robot's stopping
  // distance at driveSpeed (measure, don't guess — see CALIBRATION.md).
  int      obstacleStopMm    = 120;
  int      turnSpeed         = 255;
  uint32_t sampleSettleMs    = 2000;
  uint32_t sampleWindowMs    = 3000;
  int      servoNeutral      = 90;
  int      servoWater        = 0;
  int      servoSoil         = 180;
};
extern RobotConfiguration config;
