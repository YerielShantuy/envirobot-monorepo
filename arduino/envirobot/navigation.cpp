#include "navigation.h"
#include "config.h"
#include "position.h"
#include <Arduino.h>

// ── Motor helpers ────────────────────────────────────────────────
static float pwmToMmS(int pwm) {
  return config.mmPerSecAtDrive * (float)pwm / (float)config.driveSpeed;
}

// LEDC API changed in ESP32 Arduino core 3.x (channel-based → pin-based)
static void motorPwm(int l, int r) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_MOTOR_L_PWM, l); ledcWrite(PIN_MOTOR_R_PWM, r);
#else
  ledcWrite(0, l); ledcWrite(1, r);
#endif
}

void initMotors() {
  pinMode(PIN_MOTOR_L_FWD, OUTPUT); pinMode(PIN_MOTOR_L_BWD, OUTPUT);
  pinMode(PIN_MOTOR_R_FWD, OUTPUT); pinMode(PIN_MOTOR_R_BWD, OUTPUT);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_MOTOR_L_PWM, 5000, 8);
  ledcAttach(PIN_MOTOR_R_PWM, 5000, 8);
#else
  ledcSetup(0, 5000, 8); ledcAttachPin(PIN_MOTOR_L_PWM, 0);
  ledcSetup(1, 5000, 8); ledcAttachPin(PIN_MOTOR_R_PWM, 1);
#endif
  driveStop();
}

void driveForward(int speed) {
  int s = abs(speed);
  bool fwd = speed >= 0;
  digitalWrite(PIN_MOTOR_L_FWD, fwd); digitalWrite(PIN_MOTOR_L_BWD, !fwd);
  digitalWrite(PIN_MOTOR_R_FWD, fwd); digitalWrite(PIN_MOTOR_R_BWD, !fwd);
  motorPwm(s, s);
  float v = pwmToMmS(speed);
  setCommandedSpeed(v, v);
}

void driveStop() {
  digitalWrite(PIN_MOTOR_L_FWD, LOW); digitalWrite(PIN_MOTOR_L_BWD, LOW);
  digitalWrite(PIN_MOTOR_R_FWD, LOW); digitalWrite(PIN_MOTOR_R_BWD, LOW);
  motorPwm(0, 0);
  setCommandedSpeed(0, 0);
}

// Active short-brake: both direction inputs HIGH shorts the motor terminals
// (TB6612 / L298N / DRV8833), dumping momentum so FWD/BWD stop dead instead of
// coasting. Pulse it for BRAKE_MS then call driveStop() to de-energize —
// holding brake just heats the driver.
void driveBrake() {
  digitalWrite(PIN_MOTOR_L_FWD, HIGH); digitalWrite(PIN_MOTOR_L_BWD, HIGH);
  digitalWrite(PIN_MOTOR_R_FWD, HIGH); digitalWrite(PIN_MOTOR_R_BWD, HIGH);
  motorPwm(255, 255);
  setCommandedSpeed(0, 0);
}

void turnLeft(int speed) {
  digitalWrite(PIN_MOTOR_L_FWD, LOW);  digitalWrite(PIN_MOTOR_L_BWD, HIGH);
  digitalWrite(PIN_MOTOR_R_FWD, HIGH); digitalWrite(PIN_MOTOR_R_BWD, LOW);
  motorPwm(speed, speed);
  setCommandedSpeed(-pwmToMmS(speed), pwmToMmS(speed));
}

void turnRight(int speed) {
  digitalWrite(PIN_MOTOR_L_FWD, HIGH); digitalWrite(PIN_MOTOR_L_BWD, LOW);
  digitalWrite(PIN_MOTOR_R_FWD, LOW);  digitalWrite(PIN_MOTOR_R_BWD, HIGH);
  motorPwm(speed, speed);
  setCommandedSpeed(pwmToMmS(speed), -pwmToMmS(speed));
}
