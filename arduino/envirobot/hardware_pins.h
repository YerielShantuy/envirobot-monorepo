#pragma once

// Pins + values that describe physical hardware. Never runtime — changing
// these at runtime cannot change the actual wiring or divider resistors.
// Board: ESP32 DevKit v1 (WROOM-32).

// ── Motor pins ──────────────────────────────────────────────────
#define PIN_MOTOR_L_FWD 13
#define PIN_MOTOR_L_BWD 14
#define PIN_MOTOR_L_PWM 4
#define PIN_MOTOR_R_FWD 18
#define PIN_MOTOR_R_BWD 19
#define PIN_MOTOR_R_PWM 23

// ── Ultrasonic collision sensor (HC-SR04) ────────────────────────
// The ONLY obstacle sensor on the robot (colour sensor removed — there is no
// I2C device left, so GPIO 21/22 are free). ECHO returns 5V — MUST pass
// through a divider to 3.3V before GPIO17 (e.g. 1k series + 2k to GND →
// 0.66×). TRIG accepts the ESP32's 3.3V directly.
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

// SEN0189 outputs up to ~4.5V at 5V supply — MUST pass through a voltage
// divider before the ESP32 ADC (3.3V max). As-built topology (see wiring
// diagram): SEN0189 signal → 220Ω → tap → 470Ω → GND, ADC reads the tap.
//   V_adc = V_sensor × 470 / (220 + 470) = V_sensor × 0.681
//   ratio = (R_top + R_bottom) / R_bottom = 690 / 470 = 1.468
// Headroom check: 4.5V × 0.681 = 3.07V at the pin — inside the 3.3V limit,
// and a 3.3V pin reading maps back to 4.84V so the whole sensor range fits.
// Changing this number at runtime cannot change the physical resistors.
#define TURBIDITY_DIVIDER_RATIO 1.4681f
