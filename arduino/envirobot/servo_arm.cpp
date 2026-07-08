#include "servo_arm.h"
#include "config.h"
#include <ESP32Servo.h>

static Servo srv;

void initServo() {
  // Motors own LEDC timer 0 (channels 0/1) — steer the servo library
  // to the upper timers so PWM doesn't clash.
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  srv.setPeriodHertz(50);
  srv.attach(PIN_SERVO, 500, 2400);
  armNeutral();
}

void armNeutral() {
  srv.write(SERVO_NEUTRAL);
  delay(600);
}

void armDeployWater() {
  srv.write(SERVO_WATER);
  delay(SAMPLE_SETTLE_MS);
}

void armDeploySoil() {
  srv.write(SERVO_SOIL);
  delay(SAMPLE_SETTLE_MS);
}
