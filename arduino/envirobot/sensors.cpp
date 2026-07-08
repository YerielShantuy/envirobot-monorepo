#include "sensors.h"
#include "config.h"
#include <Adafruit_TCS34725.h>
#include <VL53L0X.h>

static Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
static VL53L0X tof;

void initSensors() {
  if (!tcs.begin()) Serial.println("[WARN] TCS34725 not found");
  // VL53L0X shares address 0x29 with the TCS34725 — it lives on Wire1
  Wire1.begin(PIN_I2C2_SDA, PIN_I2C2_SCL);
  tof.setBus(&Wire1);
  if (!tof.init()) Serial.println("[WARN] VL53L0X not found");
  tof.setTimeout(500);
  tof.startContinuous();
}

float readTurbidityNTU() {
  long raw = 0;
  for (int i = 0; i < SENSOR_SAMPLES; i++) raw += analogRead(PIN_TURBIDITY);
  float vAdc = (raw / (float)SENSOR_SAMPLES) * (3.3f / 4095.0f);
  // Undo the hardware voltage divider to recover the true sensor output
  // (SEN0189 swings to ~4.5V — direct connection would clip AND damage the pin)
  float v = vAdc * TURBIDITY_DIVIDER_RATIO;
  // DFRobot SEN0189 curve (5V supply). ponytail: curve is only valid
  // ~2.5–4.2V — MUST recalibrate against reference samples before Testing Day.
  float ntu = -1120.4f * v * v + 5742.3f * v - 4353.8f;
  return constrain(ntu, 0.0f, 3000.0f);
}

float readSoilMoisturePct() {
  long raw = 0;
  for (int i = 0; i < SENSOR_SAMPLES; i++) raw += analogRead(PIN_SOIL_SENSE);
  raw /= SENSOR_SAMPLES;
  float pct = (float)(SOIL_DRY_VAL - raw) / (float)(SOIL_DRY_VAL - SOIL_WET_VAL) * 100.0f;
  return constrain(pct, 0.0f, 100.0f);
}

// Blue water in a recessed slot: bright-ish, low red ratio.
// Thresholds live in config.h — calibrate on the REAL arena; sand and
// wet soil can false-positive under venue lighting.
bool isWaterZone() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  if (c < 100) return false;
  return c > WATER_CLEAR_MIN && ((float)r / c) < WATER_RED_RATIO_MAX;
}

bool isSoilPatch() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  if (c < 50) return false;
  // Reddish-brown disc: elevated red ratio, low blue
  float rRatio = (float)r / c;
  float bRatio = (float)b / c;
  return rRatio > SOIL_RED_RATIO_MIN && bRatio < SOIL_BLUE_RATIO_MAX;
}

float readToFDistanceMM() {
  return (float)tof.readRangeContinuousMillimeters();
}
