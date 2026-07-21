// ── EnviroBot component test: turbidity (DFRobot SEN0189) ───────────
// Pin GPIO34 (ADC1, input-only) — was 36. MUST go through the hardware
// voltage divider: SEN0189 swings to ~4.5V, ESP32 ADC max is 3.3V.
// Prints raw ADC, divider-corrected sensor volts, and NTU.
// CALIBRATE before Testing Day: dunk in clear water (~0 NTU) then a
// muddy sample; the NTU curve below is only trustworthy ~2.5-4.2V.
#define PIN_TURBIDITY          34
#define TURBIDITY_DIVIDER_RATIO 2.0f   // (R_top+R_bottom)/R_bottom
#define SAMPLES                10

void setup() {
  Serial.begin(115200);
  delay(300);
  analogReadResolution(12);            // 0-4095
  Serial.println("Turbidity test — expect volts to DROP as water gets murkier.");
}

void loop() {
  long raw = 0;
  for (int i = 0; i < SAMPLES; i++) raw += analogRead(PIN_TURBIDITY);
  float adc  = raw / (float)SAMPLES;
  float vAdc = adc * (3.3f / 4095.0f);
  float v    = vAdc * TURBIDITY_DIVIDER_RATIO;             // true sensor output
  float ntu  = -1120.4f * v * v + 5742.3f * v - 4353.8f;
  ntu = constrain(ntu, 0.0f, 3000.0f);
  Serial.printf("raw=%.0f  Vadc=%.3f  Vsensor=%.3f  NTU=%.1f\n", adc, vAdc, v, ntu);
  delay(500);
}
