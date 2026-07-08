#pragma once
#include <Arduino.h>

void startWiFiAP();
void startWebServer();
void handleWebServer();
void setServerRCMode(bool rc);   // false = autonomous: drive/sample cmds rejected
