#pragma once
#include <Arduino.h>

void startWiFiAP();       // brings up the RobotAP soft access point (AP-only, no STA)
void startWebServer();
void handleWebServer();   // also drives the collision guard + dead-man — call every loop()
