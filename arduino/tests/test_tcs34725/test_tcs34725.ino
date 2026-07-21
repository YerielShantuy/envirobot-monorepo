// ── EnviroBot component test: TCS34725 colour / zone sensor ─────────
// Bus0 Wire (SDA 21 / SCL 22), address 0x29. Prints R/G/B/Clear plus the
// two ratios navigation uses, and the live WATER / SOIL verdict.
// Wave blue water, sand, and the soil disc under it — thresholds are
// arena-lighting dependent; calibrate the WATER_*/SOIL_* values on site.
#include <Wire.h>
#include <Adafruit_TCS34725.h>

#define WATER_CLEAR_MIN     800
#define WATER_RED_RATIO_MAX 0.40f
#define SOIL_RED_RATIO_MIN  0.45f
#define SOIL_BLUE_RATIO_MAX 0.20f

Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

void setup() {
  Serial.begin(115200);
  delay(300);
  Wire.begin(21, 22);
  if (!tcs.begin()) { Serial.println("[FAIL] TCS34725 not found on 0x29"); }
  else               { Serial.println("TCS34725 OK — show it water / sand / soil."); }
}

void loop() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  float rr = c ? (float)r / c : 0, br = c ? (float)b / c : 0;
  bool water = c > 100 && c > WATER_CLEAR_MIN && rr < WATER_RED_RATIO_MAX;
  bool soil  = c > 50  && rr > SOIL_RED_RATIO_MIN && br < SOIL_BLUE_RATIO_MAX;
  Serial.printf("R=%u G=%u B=%u C=%u  r/c=%.2f b/c=%.2f  %s\n",
                r, g, b, c, rr, br,
                water ? "WATER" : soil ? "SOIL" : "-");
  delay(400);
}
