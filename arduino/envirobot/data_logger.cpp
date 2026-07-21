#include "data_logger.h"
#include "config.h"
#include "position.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ponytail: 48KB doc — 8-min run at 1Hz path (~480 pts) + 12 samples with headroom
static DynamicJsonDocument doc(49152);
static JsonArray pathArr;
static JsonArray samplesArr;
static int waterCount = 0;
static int soilCount  = 0;
static uint8_t sectorMask = 0;
static unsigned long startMs = 0;
// Run clock: false at boot and after finaliseRun(). While false, path
// logging and total_duration_s are held — otherwise the judge-facing JSON
// counts idle time from power-on and the 480-pt path cap fills with
// stationary pre-run points. startRun() flips it (first RC drive/sample,
// or autonomous START).
static bool live = false;

// Rulebook 5.1: terrains are "examples only" — confirm on Testing Day and
// update this table before the run.
static const char* TERRAINS[5] = {"Unknown", "Sandpaper", "Wet Soil", "Grass", "Sand"};

void initDataLogger() {
  doc.clear();
  waterCount = soilCount = 0;
  sectorMask = 0;
  char runId[24];
  snprintf(runId, sizeof(runId), "run_%lu", millis());
  doc["run_id"]            = runId;
  doc["total_duration_s"]  = 0;
  doc["sectors_completed"] = 0;
  pathArr    = doc.createNestedArray("path");
  samplesArr = doc.createNestedArray("samples");
  startMs    = millis();
  live       = false;
}

void startRun() {
  initDataLogger();
  live = true;
  Serial.println("[RUN] clock started");
}

bool runLive() { return live; }
bool isRunActive() { return live; }

float runElapsedS() {
  return live ? (millis() - startMs) / 1000.0f : 0.0f;
}

void logPathPoint() {
  if (!live) return;  // no pre-run idle points, no growth after finalise
  static unsigned long lastLog = 0;
  if (millis() - lastLog < 1000) return;  // 1 Hz path logging
  // Cap at 8 min of points — an uncapped log fills the JsonDocument in
  // ~12.5 min of uptime, after which logSample() silently drops samples.
  if (pathArr.size() >= 480) return;
  lastLog = millis();

  Position p = getPosition();
  JsonObject pt = pathArr.createNestedObject();
  pt["t_s"]  = (millis() - startMs) / 1000.0f;
  pt["x_mm"] = roundf(p.x_mm * 10) / 10.0f;
  pt["y_mm"] = roundf(p.y_mm * 10) / 10.0f;
}

// Frozen schema: id "W1"/"S1", type, sector, terrain, value_ntu, value_pct
// (null for the non-applicable value), t_s — visualiser depends on it.
void logSample(const char* type, float ntu, float pct, int sector) {
  bool isWater = strcmp(type, "WATER") == 0;
  char id[8];
  snprintf(id, sizeof(id), "%c%d", isWater ? 'W' : 'S',
           isWater ? ++waterCount : ++soilCount);

  if (sector < 0 || sector > 4) sector = 0;
  if (sector > 0) sectorMask |= (1 << (sector - 1));

  JsonObject s = samplesArr.createNestedObject();
  s["id"]      = id;
  s["type"]    = type;
  s["sector"]  = sector;
  s["terrain"] = TERRAINS[sector];
  if (isWater) { s["value_ntu"] = roundf(ntu * 10) / 10.0f; s["value_pct"] = nullptr; }
  else         { s["value_ntu"] = nullptr; s["value_pct"] = roundf(pct * 10) / 10.0f; }
  s["t_s"]     = (millis() - startMs) / 1000.0f;
}

static void refreshTotals() {
  doc["total_duration_s"]  = (millis() - startMs) / 1000.0f;
  int sectors = 0;
  for (int i = 0; i < 4; i++) if (sectorMask & (1 << i)) sectors++;
  doc["sectors_completed"] = sectors;
}

void finaliseRun() {
  refreshTotals();
  live = false;  // freeze totals + path; next startRun() opens a fresh doc
  if (doc.overflowed()) Serial.println("[ERR] JSON doc overflowed — data truncated");
  File f = SPIFFS.open("/data/results.json", FILE_WRITE);
  if (!f) { Serial.println("[ERR] Failed to open results.json"); return; }
  serializeJson(doc, f);
  f.close();
}

// Lightweight live feed for tools/live_plot.py (rulebook §4.6 "live table
// plotted on a connected computer") — samples only, no 48KB path array.
String getSamplesJSON() {
  String arr;
  serializeJson(samplesArr, arr);
  if (arr.length() == 0 || arr == "null") arr = "[]";
  return String("{\"t_s\":") + String((millis() - startMs) / 1000.0f, 1) +
         ",\"samples\":" + arr + "}";
}

String getRunJSON() {
  // Fresh boot with no samples yet → serve the last saved run so judges
  // can still pull data after a reboot.
  if (samplesArr.size() == 0 && SPIFFS.exists("/data/results.json")) {
    File f = SPIFFS.open("/data/results.json", FILE_READ);
    if (f) { String out = f.readString(); f.close(); return out; }
  }
  if (live) refreshTotals();  // finalised totals stay frozen
  String out;
  serializeJson(doc, out);
  return out;
}
