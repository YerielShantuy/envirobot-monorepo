#include "sensors.h"
#include "config.h"
#include <Adafruit_TCS34725.h>

// 24ms integration: halves detection reaction distance vs 50ms — pond
// stops are the safety-critical path. Zone thresholds MUST be calibrated
// at this integration time (raw counts are ~half the 50ms values).
static Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_4X);

void initSensors() {
  if (!tcs.begin()) Serial.println("[WARN] TCS34725 not found");
  // HC-SR04 ultrasonic wall sensor
  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
}

// One quick reading: SENSOR_SAMPLES ADC averages (takes ~1ms)
static float quickRead(int pin) {
  long raw = 0;
  for (int i = 0; i < SENSOR_SAMPLES; i++) raw += analogRead(pin);
  return raw / (float)SENSOR_SAMPLES;
}

// Median of quick readings taken every SAMPLE_TICK_MS over config.sampleWindowMs.
// Median (not mean) so a single ADC spike can't drag the reported value.
// Blocking by design — only called with the robot stopped and the probe
// deployed; delay() between ticks keeps the WiFi stack fed.
// Buffer sized to the clamp ceiling (10000ms window) — sampleWindowMs is
// runtime-configurable, so it can't size a stack array by itself.
static float medianAnalogWindow(int pin) {
  const int n = constrain((int)(config.sampleWindowMs / SAMPLE_TICK_MS), 1, 10000 / SAMPLE_TICK_MS);
  float buf[10000 / SAMPLE_TICK_MS];
  for (int i = 0; i < n; i++) {
    buf[i] = quickRead(pin);
    delay(SAMPLE_TICK_MS);
  }
  // insertion sort — n is small (~60)
  for (int i = 1; i < n; i++) {
    float v = buf[i];
    int j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v;
  }
  return (n & 1) ? buf[n / 2] : (buf[n / 2 - 1] + buf[n / 2]) / 2.0f;
}

// DFRobot SEN0189 curve (5V supply). ponytail: curve is only valid
// ~2.5–4.2V — MUST recalibrate against reference samples before Testing Day.
static float turbidityCurve(float rawAdc) {
  float v = rawAdc * (3.3f / 4095.0f) * TURBIDITY_DIVIDER_RATIO;
  return -1120.4f * v * v + 5742.3f * v - 4353.8f;
}

float readTurbidityNTU() {
  float raw = medianAnalogWindow(PIN_TURBIDITY);
  float ntu = turbidityCurve(raw);
  // turbidityZeroRaw wizard point: offset the curve so clear water reads 0.
  // 0 = uncalibrated, curve used as-is.
  if (config.turbidityZeroRaw > 0) ntu -= turbidityCurve(config.turbidityZeroRaw);
  return constrain(ntu, 0.0f, 3000.0f);
}

float readSoilMoisturePct() {
  float raw = medianAnalogWindow(PIN_SOIL_SENSE);
  float pct = (config.soilDryVal - raw) / (float)(config.soilDryVal - config.soilWetVal) * 100.0f;
  return constrain(pct, 0.0f, 100.0f);
}

// Hand-held wizard captures — same settle → window → median path a real
// sample uses, returns the raw pre-scaling value (no percent/NTU conversion).
float captureRawSoil() {
  delay(config.sampleSettleMs);
  return medianAnalogWindow(PIN_SOIL_SENSE);
}

float captureRawTurbidity() {
  delay(config.sampleSettleMs);
  return medianAnalogWindow(PIN_TURBIDITY);
}

void captureRawColor(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* c) {
  delay(config.sampleSettleMs);
  tcs.getRawData(r, g, b, c);
}

// Blue water in a recessed slot: bright-ish, low red ratio.
// Thresholds live in `config` — calibrate on the REAL arena; sand and
// wet soil can false-positive under venue lighting.
static bool classifyWater(uint16_t r, uint16_t c) {
  if (c < 100) return false;
  return c > config.waterClearMin && ((float)r / c) < config.waterRedRatioMax;
}

static bool classifySoil(uint16_t r, uint16_t b, uint16_t c) {
  if (c < 50) return false;
  // Reddish-brown disc: elevated red ratio, low blue
  return ((float)r / c) > config.soilRedRatioMin && ((float)b / c) < config.soilBlueRatioMax;
}

// getRawData() blocks for one integration period (~50ms) — read once,
// classify both zone types.
void readZones(bool* water, bool* soil) {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  *water = classifyWater(r, c);
  *soil  = classifySoil(r, b, c);
}

bool isWaterZone() {
  bool w, s;
  readZones(&w, &s);
  return w;
}

bool isSoilPatch() {
  bool w, s;
  readZones(&w, &s);
  return s;
}

float readWallDistanceMM() {
  // 10µs trigger pulse, then time the echo. pulseIn returns 0 on timeout.
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (us == 0) return 8190.0f;   // no echo → "no wall in sight"
  // distance = time × speed_of_sound / 2. 0.343 mm/µs at ~20°C.
  return (float)us * 0.343f / 2.0f;
}
