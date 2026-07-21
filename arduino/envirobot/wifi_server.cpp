#include "wifi_server.h"
#include "config.h"
#include "config_store.h"
#include "navigation.h"
#include "servo_arm.h"
#include "sensors.h"
#include "data_logger.h"
#include "position.h"
#include "rc_page.h"
#include "test_page.h"
#include "data_page.h"
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

static WebServer server(80);
static bool g_rcMode = true;
// Dead-man: RC page repeats the drive cmd every 120ms while held.
// If WiFi drops mid-hold, stop rather than drive blind into a wall/pond.
static bool driving = false;
static unsigned long lastDriveMs = 0;
// Timed calibration run (CAL_FWD / CAL_SPIN): motors stop at calStopMs.
// Exempt from the dead-man (no repeat cmds arrive during a cal run) —
// bounded to 10s max and only used supervised on the bench.
static bool calActive = false;
static unsigned long calStopMs = 0;

void setServerRCMode(bool rc) { g_rcMode = rc; }

// ── WiFi credential store ────────────────────────────────────────
// Runtime-editable hotspot creds. Seeded from the config.h defaults on first
// boot, persisted to SPIFFS /wifi.json, edited live via GET/POST /wifi —
// changing hotspot never needs a reflash.
static char g_ssid[33] = WIFI_STA_SSID;
static char g_pass[64] = WIFI_STA_PASS;
static bool g_apMode = false;             // true = STA failed, running BACKUP softAP (no STA, no cloud)
static bool g_reconnectPending = false;   // POST /wifi -> reconnect after the reply flushes

static void loadWiFiCreds() {
  File f = SPIFFS.open("/wifi.json", FILE_READ);
  if (!f) return;                          // no file yet — keep compiled-in defaults
  StaticJsonDocument<192> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) { Serial.println("[WiFi] /wifi.json bad json — using defaults"); return; }
  const char* s = doc["ssid"] | "";
  if (s[0]) {                              // only override if the stored ssid is non-empty
    strlcpy(g_ssid, s, sizeof(g_ssid));
    strlcpy(g_pass, doc["pass"] | "", sizeof(g_pass));
  }
}

static void saveWiFiCreds() {
  StaticJsonDocument<192> doc;
  doc["ssid"] = g_ssid;
  doc["pass"] = g_pass;
  File f = SPIFFS.open("/wifi.json", FILE_WRITE);
  if (!f) { Serial.println("[WiFi] /wifi.json write failed"); return; }
  serializeJson(doc, f);
  f.close();
}

// Join the hotspot as STA (control + live data + cloud all over that one LAN).
// One radio = one role: on failure, fall back to a BACKUP softAP so /wifi stays
// reachable and RC still works — a wrong password never locks you out. In that
// backup AP there is no STA and cloud upload is disabled (g_apMode).
static bool connectSTA() {
  g_apMode = false;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                    // no modem sleep — keeps control latency low
  WiFi.begin(g_ssid, g_pass);
  Serial.printf("[WiFi] joining \"%s\" ", g_ssid);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { delay(250); Serial.print("."); }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[WiFi] STA IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("[WiFi] open http://envirobot.local or the IP above");
    return true;
  }
  // Backup AP — provisioning + RC fallback, no STA, cloud disabled.
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS, 1);
  g_apMode = true;
  Serial.print("\n[WiFi] STA join failed — BACKUP AP \"");
  Serial.print(WIFI_SSID);
  Serial.print("\" up at http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/wifi — fix hotspot creds there (cloud upload off in AP mode)");
  return false;
}

// Name kept (called from envirobot.ino). STA-first; backup AP only on failure.
void startWiFiAP() {
  loadWiFiCreds();
  connectSTA();
  if (MDNS.begin("envirobot")) MDNS.addService("http", "tcp", 80);
}

// POST results.json to the webapp (existing public /api/viz-runs route —
// run appears in the /viz list). PRACTICE/DEMO ONLY per rulebook §4.6;
// serial sample lines remain the approved output for scored runs.
static bool uploadResults() {
  // STA is already up (robot lives on the hotspot) → upload is a plain POST.
  // Backup AP mode has no internet, so cloud is disabled there.
  if (g_apMode) {
    Serial.println("[UPLOAD] backup AP mode — cloud disabled (data safe in SPIFFS)");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[UPLOAD] not on hotspot — skipped (data safe in SPIFFS)");
    return false;
  }
  File f = SPIFFS.open("/data/results.json", FILE_READ);
  if (!f || f.size() == 0) {
    Serial.println("[UPLOAD] results.json missing");
    return false;
  }
  WiFiClientSecure client;
  client.setInsecure();  // ponytail: telemetry to a public endpoint — pin ISRG root if this ever carries secrets
  HTTPClient https;
  if (!https.begin(client, UPLOAD_URL)) { f.close(); return false; }
  https.addHeader("Content-Type", "application/json");
  https.setTimeout(10000);
  // Stream straight from SPIFFS — avoids a 48KB String copy next to TLS buffers
  int code = https.sendRequest("POST", &f, f.size());
  f.close();
  https.end();
  Serial.printf("[UPLOAD] HTTP %d, free heap %u\n", code, ESP.getFreeHeap());
  return code >= 200 && code < 300;
}

static void sendJSON(int code, const String& body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "application/json", body);
}

// I2C bus scan — the one self-test with no equivalent already exposed by the
// sensor modules (ports tests/test_i2c_scan). Wire is already begun by
// initSensors(), so this must not re-init it.
static String i2cScan(int* count) {
  String found;
  int n = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      if (n++) found += ",";
      char hex[8];
      snprintf(hex, sizeof(hex), "0x%02X", a);
      found += hex;
    }
  }
  *count = n;
  return found;
}

void startWebServer() {
  // RC page compiled into flash (rc_page.h) — no SPIFFS upload needed
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", RC_PAGE_HTML);
  });

  // Diagnostics page — same PROGMEM treatment as the RC page
  server.on("/test", HTTP_GET, []() {
    server.send_P(200, "text/html", TEST_PAGE_HTML);
  });

  // Live data page — judge-facing dashboard (arena + live position + bars + log).
  // Polls the existing /pos, /samples, /data; no dedicated endpoint of its own.
  server.on("/view", HTTP_GET, []() {
    server.send_P(200, "text/html", DATA_PAGE_HTML);
  });

  // ── Self-tests ──────────────────────────────────────────────────
  // tests/*.ino ported onto the live firmware so they run from the phone.
  // Refused during a scored run (would burn run clock / pollute samples) and
  // in autonomous mode (FSM owns motors). The two motion tests additionally
  // require {"armed":true} from the on-blocks toggle.
  server.on("/selftest", HTTP_POST, []() {
    if (!server.hasArg("plain")) { sendJSON(400, "{\"error\":\"no body\"}"); return; }

    StaticJsonDocument<128> req;
    if (deserializeJson(req, server.arg("plain"))) {
      sendJSON(400, "{\"error\":\"invalid json\"}"); return;
    }

    const char* test = req["test"]  | "";
    bool armed       = req["armed"] | false;

    if (runLive()) { sendJSON(409, "{\"error\":\"run in progress - End Run first\"}"); return; }
    if (!g_rcMode) { sendJSON(409, "{\"error\":\"autonomous mode - self-tests are RC-only\"}"); return; }

    bool ok = false;
    String value;

    if (strcmp(test, "turbidity") == 0) {
      float ntu = readTurbidityNTU();
      ok    = ntu >= 0.0f && ntu <= 4000.0f;
      value = String(ntu, 1) + " NTU";

    } else if (strcmp(test, "soil") == 0) {
      float pct = readSoilMoisturePct();
      ok    = pct >= 0.0f && pct <= 100.0f;
      value = String(pct, 1) + " %";

    } else if (strcmp(test, "zones") == 0) {
      // A read that returns is a pass; both false just means plain floor.
      bool water = false, soil = false;
      readZones(&water, &soil);
      ok    = true;
      value = String("water:") + (water ? "1" : "0") + " soil:" + (soil ? "1" : "0");

    } else if (strcmp(test, "wall") == 0) {
      float mm = readWallDistanceMM();
      ok    = mm > 0.0f;   // 0 = pulseIn timed out, no echo
      value = ok ? String(mm, 0) + " mm" : "no echo";

    } else if (strcmp(test, "i2c") == 0) {
      int n = 0;
      String addrs = i2cScan(&n);
      ok    = addrs.indexOf("0x29") >= 0;   // TCS34725 is the only device on the bus
      value = n ? addrs + " (" + String(n) + ")" : "no devices";

    } else if (strcmp(test, "motors") == 0) {
      if (!armed) { sendJSON(409, "{\"error\":\"not armed - robot must be on blocks\"}"); return; }
      // Bounded and blocking: always ends stopped, so the dead-man is not involved.
      driveForward(config.driveSpeed);  delay(500); driveStop(); delay(250);
      driveForward(-config.driveSpeed); delay(500); driveStop(); delay(250);
      turnLeft(config.turnSpeed);       delay(500); driveStop(); delay(250);
      turnRight(config.turnSpeed);      delay(500); driveStop();
      ok    = true;
      value = "fwd/bwd/left/right done";

    } else if (strcmp(test, "servo") == 0) {
      if (!armed) { sendJSON(409, "{\"error\":\"not armed - arm path must be clear\"}"); return; }
      armDeployWater(); delay(700);
      armNeutral();     delay(500);
      armDeploySoil();  delay(700);
      armNeutral();     // always park at neutral
      ok    = true;
      value = "water/neutral/soil done";

    } else {
      sendJSON(400, "{\"error\":\"unknown test\"}"); return;
    }

    // Serial mirrors every result — the rulebook-approved output path
    Serial.printf("[TEST] %s: %s (%s)\n", test, value.c_str(), ok ? "PASS" : "FAIL");

    StaticJsonDocument<192> res;
    res["test"]  = test;
    res["ok"]    = ok;
    res["value"] = value;
    String out;
    serializeJson(res, out);
    sendJSON(200, out);
  });

  // ── Runtime config ──────────────────────────────────────────────
  // Single source of truth: ESP32 SPIFFS /config.json. Locked during a
  // run — server enforces independently of the test page's own lockout.
  server.on("/config", HTTP_GET, []() {
    sendJSON(200, getConfigJSON(isRunActive()));
  });

  server.on("/config", HTTP_POST, []() {
    if (isRunActive()) { sendJSON(409, "{\"error\":\"run in progress\"}"); return; }
    if (!server.hasArg("plain")) { sendJSON(400, "{\"error\":\"no body\"}"); return; }
    DynamicJsonDocument req(1024);
    if (deserializeJson(req, server.arg("plain"))) {
      sendJSON(400, "{\"error\":\"invalid json\"}"); return;
    }
    RobotConfiguration candidate = config;
    mergeConfigJson(req.as<JsonObjectConst>(), candidate);
    const char* badField = validateConfig(candidate);
    if (badField) {
      sendJSON(400, String("{\"error\":\"") + badField + " out of range\"}"); return;
    }
    config = candidate;
    saveConfig();
    sendJSON(200, getConfigJSON(false));
  });

  server.on("/config/reset", HTTP_POST, []() {
    if (isRunActive()) { sendJSON(409, "{\"error\":\"run in progress\"}"); return; }
    resetConfig();
    Serial.println("[CONFIG] reset to defaults");
    sendJSON(200, getConfigJSON(false));
  });

  // Calibration wizard captures — same settle→window→median path a real
  // sample uses. Does not write config; the page collects captures and
  // sends the final values through the normal POST /config Save.
  server.on("/cal/capture", HTTP_POST, []() {
    if (isRunActive()) { sendJSON(409, "{\"error\":\"run in progress\"}"); return; }
    if (!server.hasArg("plain")) { sendJSON(400, "{\"error\":\"no body\"}"); return; }
    StaticJsonDocument<96> req;
    if (deserializeJson(req, server.arg("plain"))) {
      sendJSON(400, "{\"error\":\"invalid json\"}"); return;
    }
    const char* target = req["target"] | "";

    if (strcmp(target, "soil_dry") == 0 || strcmp(target, "soil_wet") == 0) {
      float raw = captureRawSoil();
      sendJSON(200, "{\"raw\":" + String(raw, 1) + "}");

    } else if (strcmp(target, "turb_zero") == 0) {
      float raw = captureRawTurbidity();
      sendJSON(200, "{\"raw\":" + String(raw, 1) + "}");

    } else if (strcmp(target, "color_water") == 0 || strcmp(target, "color_soil") == 0 ||
               strcmp(target, "color_floor") == 0) {
      uint16_t r, g, b, c;
      captureRawColor(&r, &g, &b, &c);
      char buf[96];
      snprintf(buf, sizeof(buf), "{\"raw\":{\"r\":%u,\"g\":%u,\"b\":%u,\"c\":%u}}", r, g, b, c);
      sendJSON(200, buf);

    } else {
      sendJSON(400, "{\"error\":\"unknown target\"}");
    }
  });

  // ── RC command endpoint ─────────────────────────────────────────
  server.on("/rc", HTTP_POST, []() {
    if (!server.hasArg("plain")) { sendJSON(400, "{\"error\":\"no body\"}"); return; }

    StaticJsonDocument<128> req;
    if (deserializeJson(req, server.arg("plain"))) {
      sendJSON(400, "{\"error\":\"invalid json\"}"); return;
    }

    const char* cmd = req["cmd"] | "";
    int sector      = req["sector"] | 0;   // operator-selected, 1-4
    int speed       = req["speed"]  | config.driveSpeed;
    speed = constrain(speed, 0, 255);

    // Autonomous mode: the FSM owns motors and servo. A stray phone tap
    // must not deploy the arm mid-drive. Only STOP/END_RUN/START/REZERO pass.
    if (!g_rcMode && strcmp(cmd, "STOP") != 0 && strcmp(cmd, "END_RUN") != 0 &&
        strcmp(cmd, "START") != 0 && strcmp(cmd, "REZERO") != 0) {
      sendJSON(409, "{\"error\":\"autonomous mode - only STOP/END_RUN/START/REZERO accepted\"}");
      return;
    }

    // RC run clock: first drive/sample after boot (or after END_RUN) opens a
    // fresh run doc — otherwise t_s/total_duration_s count idle time from
    // power-on and the path cap fills before the run starts. CAL_*, STOP and
    // REZERO are bench/setup commands and don't start a run.
    if (g_rcMode && !runLive() &&
        (strcmp(cmd, "FWD") == 0 || strcmp(cmd, "BWD") == 0 ||
         strcmp(cmd, "LEFT") == 0 || strcmp(cmd, "RIGHT") == 0 ||
         strcmp(cmd, "SAMPLE_WATER") == 0 || strcmp(cmd, "SAMPLE_SOIL") == 0)) {
      startRun();
    }

    if (strcmp(cmd, "START") == 0) {
#if ENABLE_AUTONOMOUS
      if (!g_rcMode) {
        navigationStart();
        sendJSON(200, "{\"started\":true}");
      } else {
        sendJSON(409, "{\"error\":\"RC mode - START is autonomous-only\"}");
      }
#else
      sendJSON(409, "{\"error\":\"autonomous build disabled\"}");
#endif
      return;
    }

    if (strcmp(cmd, "REZERO") == 0) {
      // Officials re-centre a stuck robot (rulebook 5.2.1) — operator
      // can't touch it, so pose is re-zeroed from the phone. Optional
      // "h" = eyeballed heading in degrees; 8-direction accuracy is
      // enough for quadrant-level sector estimates.
      float hDeg = req["h"] | 0.0f;
      resetPose(hDeg * (float)M_PI / 180.0f);
      Serial.printf("[POS] re-zeroed, heading %.0f deg\n", hDeg);
      sendJSON(200, "{\"rezeroed\":true}");
      return;
    }

    if (strcmp(cmd, "SAMPLE_WATER") == 0) {
      driveStop();                 // never move the servo while driving
      driving = false;
      armDeployWater();
      float ntu = readTurbidityNTU();
      armNeutral();
      logSample("WATER", ntu, 0.0f, sector);
      // Serial output doubles as the rulebook-approved data display
      Serial.printf("[SAMPLE] S%d WATER %.1f NTU\n", sector, ntu);
      sendJSON(200, "{\"ntu\":" + String(ntu, 1) + "}");

    } else if (strcmp(cmd, "SAMPLE_SOIL") == 0) {
      driveStop();
      driving = false;
      armDeploySoil();
      float pct = readSoilMoisturePct();
      armNeutral();
      logSample("SOIL", 0.0f, pct, sector);
      Serial.printf("[SAMPLE] S%d SOIL %.1f%%\n", sector, pct);
      sendJSON(200, "{\"pct\":" + String(pct, 1) + "}");

    } else if (strcmp(cmd, "END_RUN") == 0) {
      driveStop();
      driving = false;
      calActive = false;
      finaliseRun();
      Serial.println("[RUN] Finalised — results.json saved");
      bool up = uploadResults();
      sendJSON(200, String("{\"saved\":true,\"uploaded\":") + (up ? "true" : "false") + "}");

    } else if (strcmp(cmd, "CAL_FWD") == 0 || strcmp(cmd, "CAL_SPIN") == 0) {
      // Timed calibration run — drives at the exact constants the config
      // knobs refer to (driveSpeed / turnSpeed), auto-stops after ms.
      int ms = constrain((int)(req["ms"] | 2000), 200, 10000);
      if (strcmp(cmd, "CAL_FWD") == 0) driveForward(config.driveSpeed);
      else                             turnRight(config.turnSpeed);
      calActive = true;
      calStopMs = millis() + ms;
      driving = false;   // dead-man must not cut the timed run short
      sendJSON(200, "{\"cal_ms\":" + String(ms) + "}");

    } else if (strcmp(cmd, "FWD")   == 0) { driveForward(speed);   driving = true; lastDriveMs = millis(); sendJSON(200, "{}"); }
    else if (strcmp(cmd, "BWD")     == 0) { driveForward(-speed);  driving = true; lastDriveMs = millis(); sendJSON(200, "{}"); }
    else if (strcmp(cmd, "LEFT")    == 0) { turnLeft(config.turnSpeed);  driving = true; lastDriveMs = millis(); sendJSON(200, "{}"); }
    else if (strcmp(cmd, "RIGHT")   == 0) { turnRight(config.turnSpeed); driving = true; lastDriveMs = millis(); sendJSON(200, "{}"); }
    else if (strcmp(cmd, "STOP")    == 0) { driveBrake(); delay(BRAKE_MS); driveStop(); driving = false; calActive = false; sendJSON(200, "{}"); }
    else sendJSON(400, "{\"error\":\"unknown cmd\"}");
  });

  // ── WiFi provisioning ───────────────────────────────────────────
  // Runtime-editable hotspot creds — no reflash to switch networks. GET serves
  // a tiny form (reachable over STA or the backup AP); POST validates,
  // persists to SPIFFS, and reconnects. Password never echoed back — leave it
  // blank to keep the current one.
  server.on("/wifi", HTTP_GET, []() {
    bool conn = WiFi.status() == WL_CONNECTED;
    String p; p.reserve(1100);
    p  = F("<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
           "<title>EnviroBot WiFi</title><style>body{font-family:sans-serif;max-width:22rem;"
           "margin:2rem auto;padding:0 1rem}input{width:100%;padding:.5rem;margin:.3rem 0 1rem;"
           "box-sizing:border-box}button{padding:.6rem 1rem}</style><h2>EnviroBot WiFi</h2><p>Status: ");
    p += conn ? F("<b style=color:#2a7>connected</b> ") : F("<b style=color:#c33>backup AP (not on hotspot)</b>");
    if (conn) p += WiFi.localIP().toString();
    p += F("</p><form onsubmit=\"event.preventDefault();fetch('/wifi',{method:'POST',headers:"
           "{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid.value,pass:pass.value})})"
           ".then(r=>r.json()).then(j=>msg.textContent=j.error||'Saved - reconnecting, find the robot "
           "on your hotspot');\"><label>Hotspot name</label><input id=ssid maxlength=32 required value='");
    p += g_ssid;
    p += F("'><label>Password</label><input id=pass type=password maxlength=63 placeholder='(blank = "
           "keep current)'><button>Save &amp; connect</button></form><p id=msg></p>");
    server.send(200, "text/html", p);
  });

  server.on("/wifi", HTTP_POST, []() {
    if (!server.hasArg("plain")) { sendJSON(400, "{\"error\":\"no body\"}"); return; }
    StaticJsonDocument<192> req;
    if (deserializeJson(req, server.arg("plain"))) { sendJSON(400, "{\"error\":\"invalid json\"}"); return; }
    const char* ssid = req["ssid"] | "";
    const char* pass = req["pass"] | "";
    size_t sl = strlen(ssid), pl = strlen(pass);
    if (sl < 1 || sl > 32) { sendJSON(400, "{\"error\":\"ssid must be 1-32 chars\"}"); return; }
    if (pl > 63)           { sendJSON(400, "{\"error\":\"password too long (max 63)\"}"); return; }
    if (pl > 0 && pl < 8)  { sendJSON(400, "{\"error\":\"WPA password needs 8+ chars\"}"); return; }
    strlcpy(g_ssid, ssid, sizeof(g_ssid));
    if (pl > 0) strlcpy(g_pass, pass, sizeof(g_pass));   // blank keeps the stored password
    saveWiFiCreds();
    Serial.printf("[WiFi] creds updated -> \"%s\"\n", g_ssid);
    sendJSON(200, "{\"ok\":true}");
    g_reconnectPending = true;                           // reconnect once the reply has flushed
  });

  // ── Download run data ───────────────────────────────────────────
  server.on("/data", HTTP_GET, []() {
    sendJSON(200, getRunJSON());
  });

  // Live sample feed for tools/live_plot.py — small (no path array),
  // polled every ~2s by the laptop on RobotAP during a run.
  server.on("/samples", HTTP_GET, []() {
    sendJSON(200, getSamplesJSON());
  });

  // Cheap heartbeat — RC page polls this instead of serializing the
  // whole run document every 4s
  server.on("/ping", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(204);
  });

  // Live dead-reckoned position + run clock + STA IP — RC page polls at
  // ~2Hz (doubles as heartbeat). Heading in degrees, t = run seconds
  // (0 = run not started; 8-min limit is the operator's job in RC mode).
  server.on("/pos", HTTP_GET, []() {
    Position p = getPosition();
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"x\":%.0f,\"y\":%.0f,\"h\":%.1f,\"t\":%.0f,\"ip\":\"%s\"}",
             p.x_mm, p.y_mm, p.heading_rad * 180.0f / (float)M_PI, runElapsedS(),
             WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "");
    sendJSON(200, buf);
  });

  server.on("/data", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin",  "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
  });

  server.begin();
  Serial.println("[WiFi] WebServer started on port 80");
}

void handleWebServer() {
  server.handleClient();

  // POST /wifi asked for a reconnect — do it here so the HTTP 200 flushes
  // before the radio drops (reconnecting inside the handler kills the reply).
  if (g_reconnectPending) {
    g_reconnectPending = false;
    delay(150);
    connectSTA();
  }

  // Timed calibration run finished → stop exactly on the deadline
  if (calActive && millis() >= calStopMs) {
    driveStop();
    calActive = false;
    Serial.println("[CAL] timed run complete");
  }
  // Dead-man: RC page repeats drive cmds every 120ms while held; if none
  // arrive for 500ms (WiFi drop, phone sleep), stop the motors.
  if (g_rcMode && driving && millis() - lastDriveMs > 500) {
    driveBrake(); delay(BRAKE_MS); driveStop();   // brake so a WiFi drop mid-forward halts, not coasts
    driving = false;
    Serial.println("[RC] drive timeout — stopped");
  }
}
