// ── EnviroBot component test: I2C scanner ───────────────────────────
// Run this FIRST. Confirms the I2C device answers before you test it
// individually. Expected address:
//   Wire (SDA 21 / SCL 22): 0x29 TCS34725 (only I2C device)
// Wall detection is the HC-SR04 ultrasonic on GPIO 16/17 — not I2C, so
// it does not show up here (use test_ultrasonic for that).
#include <Wire.h>

static void scan(TwoWire& bus, const char* name) {
  Serial.printf("\n[%s] scanning...\n", name);
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    bus.beginTransmission(a);
    if (bus.endTransmission() == 0) {
      Serial.printf("  found 0x%02X\n", a);
      found++;
    }
  }
  Serial.printf("[%s] %d device(s)\n", name, found);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Wire.begin(21, 22);
}

void loop() {
  scan(Wire, "Wire 21/22");
  delay(3000);
}
