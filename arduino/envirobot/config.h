#pragma once

// ── Motor pins ──────────────────────────────────────────────────
// GPIO 21/22 are reserved for I2C (MPU6050, TCS34725, VL53L0X) —
// never assign them to motors.
#define PIN_MOTOR_L_FWD  13
#define PIN_MOTOR_L_BWD  14
#define PIN_MOTOR_L_PWM  4
#define PIN_MOTOR_R_FWD  18
#define PIN_MOTOR_R_BWD  19
#define PIN_MOTOR_R_PWM  23

// ── I2C buses ────────────────────────────────────────────────────
// Bus 0: MPU6050 (0x68) + TCS34725 (0x29)
#define PIN_I2C_SDA  21
#define PIN_I2C_SCL  22
// Bus 1: VL53L0X — ALSO 0x29, clashes with TCS34725, so it gets its
// own bus (replaces the TCA9548A mux from the original BOM)
#define PIN_I2C2_SDA 16
#define PIN_I2C2_SCL 17

// ── Servo ───────────────────────────────────────────────────────
#define PIN_SERVO        26
#define SERVO_NEUTRAL    90   // both arms up (travel)
#define SERVO_WATER       0   // Arm A down — turbidity probe
#define SERVO_SOIL      180   // Arm B down — soil probe

// ── Sensors ─────────────────────────────────────────────────────
#define PIN_TURBIDITY    36   // ADC1 — SEN0189 via voltage divider (see ratio below)
#define PIN_SOIL_SENSE   39   // ADC1 — capacitive soil analogue

// SEN0189 outputs up to ~4.5V at 5V supply — MUST pass through a
// voltage divider before the ESP32 ADC (3.3V max). Set the ratio to
// (R_top + R_bottom) / R_bottom, e.g. 10k/10k → 2.0
#define TURBIDITY_DIVIDER_RATIO  2.0f

// ── WiFi AP ─────────────────────────────────────────────────────
#define WIFI_SSID  "RobotAP"
#define WIFI_PASS  "enviro123"

// ── Motion model (no encoders — speed-model position estimate) ──
// ponytail: calibrate MM_PER_SEC by driving 1m on arena surface and timing it
#define MM_PER_SEC_AT_DRIVE   150.0f  // forward speed at DRIVE_SPEED PWM
#define POS_UPDATE_MS         20

// ── Sensor calibration ───────────────────────────────────────────
#define SOIL_DRY_VAL        2850
#define SOIL_WET_VAL        1200
#define SAMPLE_SETTLE_MS    500    // ms after servo deploy before reading
#define SENSOR_SAMPLES      10     // ADC averages per reading

// ── Zone detection (TCS34725) — calibrate on the real arena ─────
#define WATER_CLEAR_MIN     800    // clear channel above this AND
#define WATER_RED_RATIO_MAX 0.40f  // low red ratio → blue water
#define SOIL_RED_RATIO_MIN  0.45f
#define SOIL_BLUE_RATIO_MAX 0.20f

// ── Autonomous driving ───────────────────────────────────────────
#define DRIVE_SPEED           180  // PWM 0-255
#define TURN_SPEED            140
#define WALL_STOP_MM          120  // VL53L0X: stop this far from wall
#define RUN_TIME_LIMIT_MS     (8UL * 60UL * 1000UL)  // hard 8-minute cap
#define SAMPLE_LOCKOUT_MS     4000 // min driving time between zone detections
#define POND_BACKOFF_MS       900  // reverse duration after water sample
