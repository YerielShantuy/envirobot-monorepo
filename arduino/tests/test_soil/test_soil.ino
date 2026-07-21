// ── EnviroBot component test: capacitive soil moisture ──────────────
// Pin GPIO35 (ADC1, input-only) — was 39. No divider needed (3.3V probe).
// Prints raw ADC + % moisture. Use this to CALIBRATE the two anchors:
//   probe in dry air / dry soil  -> note raw -> that is SOIL_DRY_VAL
//   probe fully in water/wet soil -> note raw -> that is SOIL_WET_VAL
// Then copy those two numbers into config.h.
#define PIN_SOIL_SENSE 35
#define SOIL_DRY_VAL   2850   // calibrate!
#define SOIL_WET_VAL   1200   // calibrate!
#define SAMPLES        10

void setup() {
  Serial.begin(115200);
  delay(300);
  analogReadResolution(12);
  Serial.println("Soil test — raw should FALL as it gets wetter (dry=high, wet=low).");
}

void loop() {
  long raw = 0;
  for (int i = 0; i < SAMPLES; i++) raw += analogRead(PIN_SOIL_SENSE);
  raw /= SAMPLES;
  float pct = (float)(SOIL_DRY_VAL - raw) / (float)(SOIL_DRY_VAL - SOIL_WET_VAL) * 100.0f;
  pct = constrain(pct, 0.0f, 100.0f);
  Serial.printf("raw=%ld  moisture=%.1f%%\n", raw, pct);
  delay(500);
}
