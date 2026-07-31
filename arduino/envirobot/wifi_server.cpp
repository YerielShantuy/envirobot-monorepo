#include "wifi_server.h"
#include "config.h"
#include "collision.h"
#include "config_store.h"
#include "navigation.h"
#include "servo_arm.h"
#include "sensors.h"
#include "data_logger.h"
#include "position.h"
#include "rc_page.h"
#include "test_page.h"
#include "data_page.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

static WebServer server(80);
// Dead-man: RC page repeats the drive cmd every 120ms while held.
// If WiFi drops mid-hold, stop rather than drive blind into a wall/pond.
static bool driving = false;
static unsigned long lastDriveMs = 0;
// Timed calibration run (CAL_FWD / CAL_SPIN): motors stop at calStopMs.
// Exempt from the dead-man (no repeat cmds arrive during a cal run) —
// bounded to 10s max and only used supervised on the bench.
static bool calActive = false;
static unsigned long calStopMs = 0;

// ── Collision guard (HC-SR04) ────────────────────────────────────
// Polled at OBSTACLE_PING_MS in handleWebServer(). When something sits
// closer than config.obstacleStopMm for OBSTACLE_TRIP_HITS consecutive
// pings, the motors are cut and forward drive is refused until the reading
// clears by OBSTACLE_HYST_MM. Reverse and both turns stay available — a
// guard that also blocks escaping leaves the robot stranded, which costs
// more than the collision would.
// Optional: config.collisionGuardOn switches the intervention off. Pings keep
// running either way so the distance readout and calibration still work —
// only the motor cut and the FWD refusal are gated.
// Debounce/hysteresis state machine lives in collision.h (host-tested by
// tools/test_collision.cpp); this file owns the ping timing and the motor cut.
static float    obstacleMm      = 8190.0f;  // last ping, mm (8190 = nothing in range)
static bool     obstacleBlocked = false;
static ObstacleGuard obstacleGuard;
static unsigned long lastPingMs = 0;

void startWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS, WIFI_AP_CHANNEL);
  WiFi.setSleep(false);            // no modem sleep — keeps RC latency low
  Serial.print("[WiFi] AP \"");
  Serial.print(WIFI_SSID);
  Serial.print("\" up at http://");
  Serial.println(WiFi.softAPIP());
  if (MDNS.begin("envirobot")) MDNS.addService("http", "tcp", 80);
}

static void sendJSON(int code, const String& body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "application/json", body);
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
  // Refused during a scored run (would burn run clock / pollute samples).
  // The two motion tests additionally require {"armed":true} from the
  // on-blocks toggle.
  server.on("/selftest", HTTP_POST, []() {
    if (!server.hasArg("plain")) { sendJSON(400, "{\"error\":\"no body\"}"); return; }

    StaticJsonDocument<128> req;
    if (deserializeJson(req, server.arg("plain"))) {
      sendJSON(400, "{\"error\":\"invalid json\"}"); return;
    }

    const char* test = req["test"]  | "";
    bool armed       = req["armed"] | false;

    if (runLive()) { sendJSON(409, "{\"error\":\"run in progress - End Run first\"}"); return; }

    bool ok = false;
    String value;

    if (strcmp(test, "turbidity") == 0) {
      float raw = 0.0f;
      float ntu = readTurbidityNTU(&raw);
      float v   = raw * (3.3f / 4095.0f) * TURBIDITY_DIVIDER_RATIO;
      ok    = ntu >= 0.0f && ntu <= 4000.0f;
      // Raw + volts alongside the NTU: the curve clamps to 0 both above 4.20V
      // (clear water) and below 0.93V (dead/unpowered sensor), and a stale
      // turbidityZeroRaw offset zeroes everything — the volts tell them apart.
      // Healthy SEN0189 on 5V sits 2.5-4.2V; anything else means wiring.
      value = String(ntu, 1) + " NTU (raw " + String(raw, 0)
            + ", " + String(v, 2) + " V, zero " + String(config.turbidityZeroRaw) + ")";

    } else if (strcmp(test, "soil") == 0) {
      float pct = readSoilMoisturePct();
      ok    = pct >= 0.0f && pct <= 100.0f;
      value = String(pct, 1) + " %";

    } else if (strcmp(test, "obstacle") == 0) {
      float mm = readObstacleDistanceMM();
      ok    = mm > 0.0f && mm < 8190.0f;   // 8190 = pulseIn timed out, no echo
      value = ok ? String(mm, 0) + " mm (stop at " + String(config.obstacleStopMm) + ")" : "no echo";

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

    } else if (strcmp(target, "obstacle") == 0) {
      // Collision-threshold capture: one live ping, no settle/median (the
      // HC-SR04 answers in ms). Operator parks the robot at the distance it
      // should stop from and reads the number into obstacleStopMm.
      sendJSON(200, "{\"raw\":" + String(readObstacleDistanceMM(), 1) + "}");

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

    // Run clock: first drive/sample after boot (or after END_RUN) opens a
    // fresh run doc — otherwise t_s/total_duration_s count idle time from
    // power-on and the path cap fills before the run starts. CAL_*, STOP and
    // REZERO are bench/setup commands and don't start a run.
    if (!runLive() &&
        (strcmp(cmd, "FWD") == 0 || strcmp(cmd, "BWD") == 0 ||
         strcmp(cmd, "LEFT") == 0 || strcmp(cmd, "RIGHT") == 0 ||
         strcmp(cmd, "SAMPLE_WATER") == 0 || strcmp(cmd, "SAMPLE_SOIL") == 0)) {
      startRun();
    }

    // Collision guard: refuse anything that drives the robot FORWARD while
    // an obstacle sits inside config.obstacleStopMm. Reverse/turns pass so
    // the operator can always back out of the block.
    if (obstacleBlocked && (strcmp(cmd, "FWD") == 0 || strcmp(cmd, "CAL_FWD") == 0)) {
      sendJSON(409, "{\"error\":\"obstacle\",\"mm\":" + String(obstacleMm, 0) + "}");
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
      driveBrake(); delay(BRAKE_MS); driveStop();   // never move the servo while driving — brake, don't coast
      driving = false;
      armDeployWater();
      float ntu = readTurbidityNTU();
      armNeutral();
      logSample("WATER", ntu, 0.0f, sector);
      // Serial output doubles as the rulebook-approved data display
      Serial.printf("[SAMPLE] S%d WATER %.1f NTU\n", sector, ntu);
      sendJSON(200, "{\"ntu\":" + String(ntu, 1) + "}");

    } else if (strcmp(cmd, "SAMPLE_SOIL") == 0) {
      driveBrake(); delay(BRAKE_MS); driveStop();
      driving = false;
      armDeploySoil();
      float pct = readSoilMoisturePct();
      armNeutral();
      logSample("SOIL", 0.0f, pct, sector);
      Serial.printf("[SAMPLE] S%d SOIL %.1f%%\n", sector, pct);
      sendJSON(200, "{\"pct\":" + String(pct, 1) + "}");

    } else if (strcmp(cmd, "END_RUN") == 0) {
      // Finalise + write /data/results.json to SPIFFS. Nothing is uploaded
      // anywhere — the operator pulls the file with ⬇ Download JSON (GET
      // /data) and drops it into the webapp visualiser for playback.
      driveStop();
      armNeutral();      // park the arm — a reset landing mid-sample leaves it deployed
      driving = false;
      calActive = false;
      finaliseRun();
      Serial.println("[RUN] Finalised — results.json saved to SPIFFS");
      sendJSON(200, "{\"saved\":true}");

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

  // ── Download run data ───────────────────────────────────────────
  // The ⬇ Download JSON button on the RC page points straight here.
  server.on("/data", HTTP_GET, []() {
    server.sendHeader("Content-Disposition", "attachment; filename=\"envirobot_run.json\"");
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

  // Live dead-reckoned position + run clock + collision state — RC page
  // polls at ~2Hz (doubles as heartbeat). Heading in degrees, t = run
  // seconds (0 = run not started; the 8-min limit is the operator's job),
  // d = last obstacle ping in mm, blocked = forward drive currently refused,
  // guard = collision guard switched on (distance is reported either way).
  server.on("/pos", HTTP_GET, []() {
    Position p = getPosition();
    char buf[176];
    snprintf(buf, sizeof(buf),
             "{\"x\":%.0f,\"y\":%.0f,\"h\":%.1f,\"t\":%.0f,\"d\":%.0f,\"blocked\":%s,\"guard\":%s}",
             p.x_mm, p.y_mm, p.heading_rad * 180.0f / (float)M_PI, runElapsedS(),
             obstacleMm, obstacleBlocked ? "true" : "false",
             config.collisionGuardOn ? "true" : "false");
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

// One ultrasonic ping per OBSTACLE_PING_MS. Trips only after
// OBSTACLE_TRIP_HITS consecutive in-range reads (a single HC-SR04 glitch
// must not halt a run) and clears only once the reading is OBSTACLE_HYST_MM
// past the threshold (a bare threshold chatters at the boundary).
static void updateCollisionGuard() {
  if (millis() - lastPingMs < OBSTACLE_PING_MS) return;
  lastPingMs = millis();
  obstacleMm = readObstacleDistanceMM();

  // Guard switched off: keep reporting distance, never intervene. Reset the
  // debounce so re-enabling starts clean instead of tripping on stale hits.
  if (!config.collisionGuardOn) {
    obstacleGuard = ObstacleGuard();
    if (obstacleBlocked) {
      obstacleBlocked = false;
      Serial.println("[COLLISION] guard disabled — forward drive unrestricted");
    }
    return;
  }

  bool blocked = obstacleStep(obstacleGuard, obstacleMm, config.obstacleStopMm,
                              OBSTACLE_TRIP_HITS, OBSTACLE_HYST_MM);
  if (blocked && !obstacleBlocked) {
    // Cut the motors on the trip edge, not every tick — otherwise holding
    // BWD to escape would be braked 10× a second.
    driveBrake(); delay(BRAKE_MS); driveStop();
    driving   = false;
    calActive = false;
    Serial.printf("[COLLISION] obstacle at %.0f mm (stop %d mm) — motors cut, reverse to clear\n",
                  obstacleMm, config.obstacleStopMm);
  } else if (!blocked && obstacleBlocked) {
    Serial.println("[COLLISION] clear — forward drive re-enabled");
  }
  obstacleBlocked = blocked;
}

void handleWebServer() {
  server.handleClient();

  updateCollisionGuard();

  // Timed calibration run finished → stop exactly on the deadline
  if (calActive && millis() >= calStopMs) {
    driveStop();
    calActive = false;
    Serial.println("[CAL] timed run complete");
  }
  // Dead-man: RC page repeats drive cmds every 120ms while held; if none
  // arrive for 500ms (WiFi drop, phone sleep), stop the motors.
  if (driving && millis() - lastDriveMs > 500) {
    driveBrake(); delay(BRAKE_MS); driveStop();   // brake so a WiFi drop mid-forward halts, not coasts
    driving = false;
    Serial.println("[RC] drive timeout — stopped");
  }
}
