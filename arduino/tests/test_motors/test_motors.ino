// ── EnviroBot component test: DC drive motors (L + R via driver) ────
// Cycles: L fwd, L bwd, R fwd, R bwd, both fwd, both bwd, stop.
// Watch each wheel spins the RIGHT way. If a wheel runs backwards,
// swap that motor's two driver output wires (or FWD/BWD pins).
// PWM API is guarded for ESP32 core 2.x vs 3.x (matches navigation.cpp).
#define PIN_MOTOR_L_FWD 13
#define PIN_MOTOR_L_BWD 14
#define PIN_MOTOR_L_PWM 4
#define PIN_MOTOR_R_FWD 18
#define PIN_MOTOR_R_BWD 19
#define PIN_MOTOR_R_PWM 23
#define TEST_SPEED      180   // 0-255

static void pwm(int l, int r) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_MOTOR_L_PWM, l); ledcWrite(PIN_MOTOR_R_PWM, r);
#else
  ledcWrite(0, l); ledcWrite(1, r);
#endif
}

static void left(bool fwd)  { digitalWrite(PIN_MOTOR_L_FWD, fwd);  digitalWrite(PIN_MOTOR_L_BWD, !fwd); }
static void right(bool fwd) { digitalWrite(PIN_MOTOR_R_FWD, fwd);  digitalWrite(PIN_MOTOR_R_BWD, !fwd); }

static void stop() {
  pwm(0, 0);
  digitalWrite(PIN_MOTOR_L_FWD, LOW); digitalWrite(PIN_MOTOR_L_BWD, LOW);
  digitalWrite(PIN_MOTOR_R_FWD, LOW); digitalWrite(PIN_MOTOR_R_BWD, LOW);
}

static void step(const char* label, bool lOn, bool lFwd, bool rOn, bool rFwd) {
  Serial.println(label);
  if (lOn) left(lFwd);
  if (rOn) right(rFwd);
  pwm(lOn ? TEST_SPEED : 0, rOn ? TEST_SPEED : 0);
  delay(1500);
  stop();
  delay(800);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_MOTOR_L_FWD, OUTPUT); pinMode(PIN_MOTOR_L_BWD, OUTPUT);
  pinMode(PIN_MOTOR_R_FWD, OUTPUT); pinMode(PIN_MOTOR_R_BWD, OUTPUT);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_MOTOR_L_PWM, 5000, 8);
  ledcAttach(PIN_MOTOR_R_PWM, 5000, 8);
#else
  ledcSetup(0, 5000, 8); ledcAttachPin(PIN_MOTOR_L_PWM, 0);
  ledcSetup(1, 5000, 8); ledcAttachPin(PIN_MOTOR_R_PWM, 1);
#endif
  stop();
  Serial.println("Motor test — wheels should spin as labelled. Robot on blocks!");
}

void loop() {
  step("LEFT  forward",  true,  true,  false, false);
  step("LEFT  backward", true,  false, false, false);
  step("RIGHT forward",  false, false, true,  true);
  step("RIGHT backward", false, false, true,  false);
  step("BOTH  forward",  true,  true,  true,  true);
  step("BOTH  backward", true,  false, true,  false);
  Serial.println("--- cycle done ---\n");
  delay(1500);
}
