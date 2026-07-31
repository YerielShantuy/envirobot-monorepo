/*
 * EnviroBot — Environmental Survey Robot
 * ESP32 DevKit v1
 *
 * Remote control only. There is no autonomous mode: every motion comes from
 * an operator tap on the RC page. An HC-SR04 collision guard cuts forward
 * drive when something sits inside config.obstacleStopMm.
 *
 * WiFi is AP-only — the robot IS the network ("RobotAP", http://192.168.4.1).
 * No router, no hotspot, no cloud. Run data leaves as a downloaded
 * results.json (⬇ Download JSON → GET /data) for webapp playback; serial
 * sample lines stay the rulebook-approved output (§4.6).
 */
#include "config.h"
#include "config_store.h"
#include "position.h"
#include "sensors.h"
#include "servo_arm.h"
#include "navigation.h"
#include "data_logger.h"
#include "wifi_server.h"
#include <SPIFFS.h>

RobotConfiguration config;

void setup() {
  Serial.begin(115200);

  // LED on GPIO2 — solid = ready
  pinMode(2, OUTPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("[ERR] SPIFFS mount failed");
  }
  loadConfig();   // merge /config.json over defaults, clamp, apply — before anything reads `config`

  initMotors();
  initServo();
  initSensors();
  initPosition();   // open-loop odometry (no IMU) — instant
  initDataLogger();

  startWiFiAP();
  startWebServer();

  Serial.println("[MODE] RC");
  digitalWrite(2, HIGH);
}

void loop() {
  handleWebServer();
  updatePosition();
  logPathPoint();
}
