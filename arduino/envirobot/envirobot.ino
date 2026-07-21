/*
 * EnviroBot — Environmental Survey Robot
 * ESP32 DevKit v1
 *
 * Build mode set by ENABLE_AUTONOMOUS in config.h:
 *   0 (default) → RC-only: always boots in remote-control mode.
 *   1           → hold BOOT at power-up for RC, else autonomous FSM.
 *
 * WiFi AP + web server run in BOTH modes so judges can always pull
 * /data immediately after a run (rulebook 5.2.1.3).
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
static bool rcMode = false;

void setup() {
  Serial.begin(115200);

  // Boot button on GPIO0 — hold for RC mode
  pinMode(0, INPUT_PULLUP);
#if ENABLE_AUTONOMOUS
  rcMode = (digitalRead(0) == LOW);
#else
  rcMode = true;   // RC-only build — autonomous FSM compiled out
#endif

  // LED on GPIO2
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
  setServerRCMode(rcMode);

  if (rcMode) {
    Serial.println("[MODE] RC");
    digitalWrite(2, HIGH);
  }
#if ENABLE_AUTONOMOUS
  else {
    Serial.println("[MODE] AUTO");
    initNavigation();
  }
#endif
}

void loop() {
  handleWebServer();
  updatePosition();
  logPathPoint();
#if ENABLE_AUTONOMOUS
  if (!rcMode) runFSM();
#endif
}
