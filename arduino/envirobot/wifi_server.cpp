#include "wifi_server.h"
#include "config.h"
#include "navigation.h"
#include "servo_arm.h"
#include "sensors.h"
#include "data_logger.h"
#include "rc_page.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer server(80);
static bool g_rcMode = true;

void setServerRCMode(bool rc) { g_rcMode = rc; }

void startWiFiAP() {
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] AP IP: ");
  Serial.println(WiFi.softAPIP());
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

  // ── RC command endpoint ─────────────────────────────────────────
  server.on("/rc", HTTP_POST, []() {
    if (!server.hasArg("plain")) { sendJSON(400, "{\"error\":\"no body\"}"); return; }

    StaticJsonDocument<128> req;
    if (deserializeJson(req, server.arg("plain"))) {
      sendJSON(400, "{\"error\":\"invalid json\"}"); return;
    }

    const char* cmd = req["cmd"] | "";
    int sector      = req["sector"] | 0;   // operator-selected, 1-4
    int speed       = req["speed"]  | DRIVE_SPEED;
    speed = constrain(speed, 0, 255);

    // Autonomous mode: the FSM owns motors and servo. A stray phone tap
    // must not deploy the arm mid-drive. Only STOP and END_RUN pass.
    if (!g_rcMode && strcmp(cmd, "STOP") != 0 && strcmp(cmd, "END_RUN") != 0) {
      sendJSON(409, "{\"error\":\"autonomous mode - only STOP/END_RUN accepted\"}");
      return;
    }

    if (strcmp(cmd, "SAMPLE_WATER") == 0) {
      armDeployWater();
      float ntu = readTurbidityNTU();
      armNeutral();
      logSample("WATER", ntu, 0.0f, sector);
      // Serial output doubles as the rulebook-approved data display
      Serial.printf("[SAMPLE] S%d WATER %.1f NTU\n", sector, ntu);
      sendJSON(200, "{\"ntu\":" + String(ntu, 1) + "}");

    } else if (strcmp(cmd, "SAMPLE_SOIL") == 0) {
      armDeploySoil();
      float pct = readSoilMoisturePct();
      armNeutral();
      logSample("SOIL", 0.0f, pct, sector);
      Serial.printf("[SAMPLE] S%d SOIL %.1f%%\n", sector, pct);
      sendJSON(200, "{\"pct\":" + String(pct, 1) + "}");

    } else if (strcmp(cmd, "END_RUN") == 0) {
      driveStop();
      finaliseRun();
      Serial.println("[RUN] Finalised — results.json saved");
      sendJSON(200, "{\"saved\":true}");

    } else if (strcmp(cmd, "FWD")   == 0) { driveForward(speed);   sendJSON(200, "{}"); }
    else if (strcmp(cmd, "BWD")     == 0) { driveForward(-speed);  sendJSON(200, "{}"); }
    else if (strcmp(cmd, "LEFT")    == 0) { turnLeft(TURN_SPEED);  sendJSON(200, "{}"); }
    else if (strcmp(cmd, "RIGHT")   == 0) { turnRight(TURN_SPEED); sendJSON(200, "{}"); }
    else if (strcmp(cmd, "STOP")    == 0) { driveStop();           sendJSON(200, "{}"); }
    else sendJSON(400, "{\"error\":\"unknown cmd\"}");
  });

  // ── Download run data ───────────────────────────────────────────
  server.on("/data", HTTP_GET, []() {
    sendJSON(200, getRunJSON());
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
}
