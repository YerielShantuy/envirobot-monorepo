#pragma once

// Pins + values that describe physical hardware. Never runtime — changing
// these at runtime cannot change the actual wiring or divider resistors.

// ── Motor pins ──────────────────────────────────────────────────
// GPIO 21/22 are reserved for I2C (TCS34725 colour sensor) —
// never assign them to motors.
#define PIN_MOTOR_L_FWD 13
#define PIN_MOTOR_L_BWD 14
#define PIN_MOTOR_L_PWM 4
#define PIN_MOTOR_R_FWD 18
#define PIN_MOTOR_R_BWD 19
#define PIN_MOTOR_R_PWM 23

// ── I2C bus ──────────────────────────────────────────────────────
// TCS34725 colour sensor (0x29) is the only I2C device now.
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// ── Ultrasonic wall sensor (HC-SR04) ─────────────────────────────
// Reuses the two pins the dropped VL53L0X second I2C bus freed up.
// ECHO returns 5V — MUST pass through a divider to 3.3V before GPIO17
// (e.g. 1k series + 2k to GND → 0.66×). TRIG accepts the ESP32's 3.3V.
#define PIN_ULTRASONIC_TRIG 16
#define PIN_ULTRASONIC_ECHO 17
#define ULTRASONIC_TIMEOUT_US 25000 // ~4.3m cap; also bounds loop stall on no echo

// ── Servo ───────────────────────────────────────────────────────
#define PIN_SERVO 27 

// ── Sensors ─────────────────────────────────────────────────────
#define PIN_TURBIDITY 34  // ADC1 (input-only) — SEN0189 via voltage divider (see ratio below)
#define PIN_SOIL_SENSE 35 // ADC1 (input-only) — capacitive soil analogue
// NB: GPIO 34/35 (were 36/39) are ADC1 → safe with WiFi AP on. NEVER use
// ADC2 pins (0,2,4,12-15,25-27) for analog: ADC2 is unusable while WiFi runs.

// SEN0189 outputs up to ~4.5V at 5V supply — MUST pass through a
// voltage divider before the ESP32 ADC (3.3V max). Ratio is set by the
// physical resistors — changing it at runtime cannot change the hardware.
// (R_top + R_bottom) / R_bottom, e.g. 10k/10k → 2.0
#define TURBIDITY_DIVIDER_RATIO 2.0f
