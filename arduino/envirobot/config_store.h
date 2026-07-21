#pragma once
#include "robot_config.h"
#include <ArduinoJson.h>

void loadConfig();   // merge /config.json over defaults, clamp, apply to `config`
void saveConfig();   // serialize `config` (all 18 fields) to /config.json
void resetConfig();  // delete /config.json, revert `config` to defaults

// Overwrites only the fields present in o. Leaves fields absent from o
// untouched in target — used to merge a partial JSON body over either the
// compiled-in defaults (load) or the live config (POST /config).
void mergeConfigJson(JsonObjectConst o, RobotConfiguration& target);

// Returns nullptr if every field in c is in range, else the name of the
// first offending field (also catches soilDryVal == soilWetVal).
const char* validateConfig(const RobotConfiguration& c);

// { ...18 fields, runActive }
String getConfigJSON(bool runActive);
