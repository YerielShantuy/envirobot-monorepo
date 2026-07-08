/*
 * EnviroBot — Environmental Survey Robot
 * ESP32 DevKit v1
 *
 * Boot with BOOT button held → RC mode (drive via joystick page)
 * Boot normally              → Autonomous FSM mode (stretch goal)
 *
 * WiFi AP + web server run in BOTH modes so judges can always pull
 * /data immediately after a run (rulebook 5.2.1.3).
 */
#include "config.h"
#include "position.h"
#include "sensors.h"
#include "servo_arm.h"
#include "navigation.h"
#include "data_logger.h"
#include "wifi_server.h"
#include <SPIFFS.h>

static bool rcMode = false;

void setup() {
  Serial.begin(115200);

  // Boot button on GPIO0 — hold for RC mode
  pinMode(0, INPUT_PULLUP);
  rcMode = (digitalRead(0) == LOW);

  // LED on GPIO2
  pinMode(2, OUTPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("[ERR] SPIFFS mount failed");
  }

  initMotors();
  initServo();
  initSensors();
  initPosition();   // MPU6050 DMP — takes ~2s to settle and zero heading
  initDataLogger();

  startWiFiAP();
  startWebServer();
  setServerRCMode(rcMode);

  if (rcMode) {
    Serial.println("[MODE] RC");
    digitalWrite(2, HIGH);
  } else {
    Serial.println("[MODE] AUTO");
    initNavigation();
  }
}

void loop() {
  handleWebServer();
  updatePosition();
  logPathPoint();
  if (!rcMode) runFSM();
}
