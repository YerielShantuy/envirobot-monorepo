/*
 * test_ultrasonic — HC-SR04 wall sensor bring-up
 * Wiring: TRIG=GPIO16, ECHO=GPIO17 (through a 5V→3.3V divider).
 * Prints distance in mm. Wave a hand / wall in front and watch it track.
 * "no echo" = out of range, or ECHO not wired / divider wrong.
 */
#define PIN_TRIG 16
#define PIN_ECHO 17
#define TIMEOUT_US 25000

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  Serial.println("HC-SR04 test — distance in mm");
}

void loop() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, TIMEOUT_US);
  if (us == 0) Serial.println("no echo (out of range / check ECHO wiring)");
  else         Serial.printf("%.0f mm\n", (float)us * 0.343f / 2.0f);
  delay(200);
}
