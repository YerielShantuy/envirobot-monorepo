// ── EnviroBot component test: double-arm servo ──────────────────────
// One shaft, two arms. Sweeps 0 -> 90 -> 180 -> 90 so you can confirm:
//   0   = Arm A (turbidity/SEN0189) DOWN, Arm B UP
//   90  = NEUTRAL, both arms horizontal (travel)
//   180 = Arm B (soil) DOWN, Arm A UP
// If travel is short of full range, widen attach() min/max microseconds.
// Uses ESP32Servo. Timers 2/3 so it never clashes with motor LEDC (0/1).
#include <ESP32Servo.h>

#define PIN_SERVO     26
#define SERVO_NEUTRAL 90
#define SERVO_WATER   0
#define SERVO_SOIL    180

Servo srv;

static void go(const char* label, int deg) {
  Serial.printf("%s -> %d deg\n", label, deg);
  srv.write(deg);
  delay(1200);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  srv.setPeriodHertz(50);
  srv.attach(PIN_SERVO, 500, 2400);
  Serial.println("Servo test — check both arms hit end stops cleanly.");
}

void loop() {
  go("WATER  ", SERVO_WATER);
  go("NEUTRAL", SERVO_NEUTRAL);
  go("SOIL   ", SERVO_SOIL);
  go("NEUTRAL", SERVO_NEUTRAL);
  Serial.println("--- cycle done ---\n");
  delay(1000);
}
