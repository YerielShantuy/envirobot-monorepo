#include "config_store.h"
#include <SPIFFS.h>

template <typename T>
static T clampLog(const char* name, T v, T lo, T hi) {
  if (v < lo || v > hi) {
    T c = v < lo ? lo : hi;
    Serial.printf("[CONFIG] %s out of range, clamped\n", name);
    return c;
  }
  return v;
}

template <typename T>
static const char* checkRange(const char* name, T v, T lo, T hi) {
  return (v < lo || v > hi) ? name : nullptr;
}

// Silent clamp for values coming off SPIFFS — a corrupt/hand-edited file
// must never hang the boot; log and continue.
static void clampConfig(RobotConfiguration& c) {
  c.servoNeutral = clampLog("servoNeutral", c.servoNeutral, 0, 180);
  c.servoWater   = clampLog("servoWater", c.servoWater, 0, 180);
  c.servoSoil    = clampLog("servoSoil", c.servoSoil, 0, 180);
  c.driveSpeed   = clampLog("driveSpeed", c.driveSpeed, 1, 255);   // navigation.cpp divides by it
  c.turnSpeed    = clampLog("turnSpeed", c.turnSpeed, 0, 255);
  c.soilDryVal   = clampLog("soilDryVal", c.soilDryVal, 0, 4095);
  c.soilWetVal   = clampLog("soilWetVal", c.soilWetVal, 0, 4095);
  if (c.soilDryVal == c.soilWetVal) {
    Serial.println("[CONFIG] soilDryVal == soilWetVal, reverting both to defaults");
    RobotConfiguration def;
    c.soilDryVal = def.soilDryVal;
    c.soilWetVal = def.soilWetVal;
  }
  c.turbidityZeroRaw   = clampLog("turbidityZeroRaw", c.turbidityZeroRaw, 0, 4095);
  c.mmPerSecAtDrive    = clampLog("mmPerSecAtDrive", c.mmPerSecAtDrive, 10.0f, 1000.0f);
  c.wheelBaseMm        = clampLog("wheelBaseMm", c.wheelBaseMm, 50.0f, 500.0f);
  c.obstacleStopMm     = clampLog("obstacleStopMm", c.obstacleStopMm, 30, 2000);
  c.sampleSettleMs     = clampLog("sampleSettleMs", c.sampleSettleMs, (uint32_t)200, (uint32_t)10000);
  c.sampleWindowMs     = clampLog("sampleWindowMs", c.sampleWindowMs, (uint32_t)500, (uint32_t)10000);
}

// Rejecting validation for POST — never mutates, names the first bad field.
const char* validateConfig(const RobotConfiguration& c) {
  const char* f;
  if ((f = checkRange("servoNeutral", c.servoNeutral, 0, 180))) return f;
  if ((f = checkRange("servoWater", c.servoWater, 0, 180))) return f;
  if ((f = checkRange("servoSoil", c.servoSoil, 0, 180))) return f;
  if ((f = checkRange("driveSpeed", c.driveSpeed, 1, 255))) return f;
  if ((f = checkRange("turnSpeed", c.turnSpeed, 0, 255))) return f;
  if ((f = checkRange("soilDryVal", c.soilDryVal, 0, 4095))) return f;
  if ((f = checkRange("soilWetVal", c.soilWetVal, 0, 4095))) return f;
  if (c.soilDryVal == c.soilWetVal) return "soilDryVal";
  if ((f = checkRange("turbidityZeroRaw", c.turbidityZeroRaw, 0, 4095))) return f;
  if ((f = checkRange("mmPerSecAtDrive", c.mmPerSecAtDrive, 10.0f, 1000.0f))) return f;
  if ((f = checkRange("wheelBaseMm", c.wheelBaseMm, 50.0f, 500.0f))) return f;
  if ((f = checkRange("obstacleStopMm", c.obstacleStopMm, 30, 2000))) return f;
  if ((f = checkRange("sampleSettleMs", c.sampleSettleMs, (uint32_t)200, (uint32_t)10000))) return f;
  if ((f = checkRange("sampleWindowMs", c.sampleWindowMs, (uint32_t)500, (uint32_t)10000))) return f;
  return nullptr;
}

void mergeConfigJson(JsonObjectConst o, RobotConfiguration& t) {
  if (o.containsKey("soilDryVal"))         t.soilDryVal         = o["soilDryVal"];
  if (o.containsKey("soilWetVal"))         t.soilWetVal         = o["soilWetVal"];
  if (o.containsKey("turbidityZeroRaw"))   t.turbidityZeroRaw   = o["turbidityZeroRaw"];
  if (o.containsKey("mmPerSecAtDrive"))    t.mmPerSecAtDrive    = o["mmPerSecAtDrive"];
  if (o.containsKey("wheelBaseMm"))        t.wheelBaseMm        = o["wheelBaseMm"];
  if (o.containsKey("driveSpeed"))         t.driveSpeed         = o["driveSpeed"];
  if (o.containsKey("turnSpeed"))          t.turnSpeed          = o["turnSpeed"];
  if (o.containsKey("collisionGuardOn"))   t.collisionGuardOn   = o["collisionGuardOn"];
  if (o.containsKey("obstacleStopMm"))     t.obstacleStopMm     = o["obstacleStopMm"];
  if (o.containsKey("sampleSettleMs"))     t.sampleSettleMs     = o["sampleSettleMs"];
  if (o.containsKey("sampleWindowMs"))     t.sampleWindowMs     = o["sampleWindowMs"];
  if (o.containsKey("servoNeutral"))       t.servoNeutral       = o["servoNeutral"];
  if (o.containsKey("servoWater"))         t.servoWater         = o["servoWater"];
  if (o.containsKey("servoSoil"))          t.servoSoil          = o["servoSoil"];
}

static void fillDoc(JsonDocument& doc) {
  doc["soilDryVal"]         = config.soilDryVal;
  doc["soilWetVal"]         = config.soilWetVal;
  doc["turbidityZeroRaw"]   = config.turbidityZeroRaw;
  doc["mmPerSecAtDrive"]    = config.mmPerSecAtDrive;
  doc["wheelBaseMm"]        = config.wheelBaseMm;
  doc["driveSpeed"]         = config.driveSpeed;
  doc["turnSpeed"]          = config.turnSpeed;
  doc["collisionGuardOn"]   = config.collisionGuardOn;
  doc["obstacleStopMm"]     = config.obstacleStopMm;
  doc["sampleSettleMs"]     = config.sampleSettleMs;
  doc["sampleWindowMs"]     = config.sampleWindowMs;
  doc["servoNeutral"]       = config.servoNeutral;
  doc["servoWater"]         = config.servoWater;
  doc["servoSoil"]          = config.servoSoil;
}

void loadConfig() {
  config = RobotConfiguration();
  File f = SPIFFS.open("/config.json", FILE_READ);
  if (!f) {
    Serial.println("[CONFIG] no config, using defaults");
  } else {
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
      Serial.println("[CONFIG] bad json, using defaults");
      config = RobotConfiguration();
    } else {
      mergeConfigJson(doc.as<JsonObjectConst>(), config);
    }
  }
  clampConfig(config);
}

void saveConfig() {
  DynamicJsonDocument doc(1024);
  fillDoc(doc);
  File f = SPIFFS.open("/config.json", FILE_WRITE);
  if (!f) { Serial.println("[CONFIG] failed to open /config.json for write"); return; }
  serializeJson(doc, f);
  f.close();
}

void resetConfig() {
  SPIFFS.remove("/config.json");
  config = RobotConfiguration();
}

String getConfigJSON(bool runActive) {
  DynamicJsonDocument doc(1024);
  fillDoc(doc);
  doc["runActive"] = runActive;
  String out;
  serializeJson(doc, out);
  return out;
}
