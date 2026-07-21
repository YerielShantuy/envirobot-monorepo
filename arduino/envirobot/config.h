#pragma once

#include "hardware_pins.h"
#include "robot_config.h"

// ── WiFi ────────────────────────────────────────────────────────
// STA-only: the robot JOINS your phone hotspot. Control, live data and cloud
// upload all run over that one LAN — no dual radio, no channel conflict.
// Reach it at http://envirobot.local or the IP printed on serial at boot.
// Creds below are FIRST-BOOT DEFAULTS only — runtime-editable at /wifi and
// persisted to SPIFFS /wifi.json, so changing hotspot needs no reflash.
// If STA can't join at boot, the robot falls back to a provisioning softAP
// (WIFI_SSID/WIFI_PASS) so /wifi stays reachable — a bad cred never bricks it.
// Cloud upload stays PRACTICE/DEMO ONLY (rulebook §4.6); serial sample lines
// remain the legal output for scored runs.
#define WIFI_STA_SSID "Yer"                                     // default hotspot name (editable at /wifi)
#define WIFI_STA_PASS "yerielkeren"                             // default hotspot password (editable at /wifi)
#define WIFI_SSID "RobotAP"                                     // provisioning-AP SSID (STA-fail fallback)
#define WIFI_PASS "enviro123"                                   // provisioning-AP password (8+ chars)
#define UPLOAD_URL "https://envirobot.yerielph.workers.dev/api/viz-runs" // deployed webapp (Workers)

// ── Motion model timing ───────────────────────────────────────────
#define POS_UPDATE_MS 20

// ── Motor braking ─────────────────────────────────────────────────
// On release, pulse an active short-brake (both motor terminals shorted) then
// coast-off — stops FWD/BWD dead instead of coasting on momentum. Tune to your
// driver + robot mass; 0 = disable (revert to plain coast). Requires a driver
// that short-brakes on both direction pins HIGH (TB6612 / L298N / DRV8833).
#define BRAKE_MS 80

// ── Sensor sampling ────────────────────────────────────────────────
#define SAMPLE_TICK_MS 50     // one reading per tick, median reported
#define SENSOR_SAMPLES 10     // ADC averages per tick

// ── Build mode ───────────────────────────────────────────────────
// 0 = RC-only build: autonomous FSM compiled out, robot always boots in
//     remote-control mode. This is the competition-default (RC-first).
// 1 = enable autonomous FSM (stretch goal). Hold BOOT at power-up for RC.
#define ENABLE_AUTONOMOUS 0

// ── Autonomous driving (dead code while ENABLE_AUTONOMOUS == 0) ───
#define RUN_TIME_LIMIT_MS (8UL * 60UL * 1000UL) // hard 8-minute cap
#define SAMPLE_LOCKOUT_MS 4000                  // min driving time between zone detections
#define POND_BACKOFF_MS 900                     // reverse duration after water sample
#define SAMPLE_CREEP_MS 0                       // fwd nudge after detect so probe tip reaches target; bench-measure TCS→probe offset (0 = off)
#define SAMPLE_CREEP_SPEED 120                  // PWM for the creep nudge
#define SWEEP_LEG_TIMEOUT_MS 8000               // no zone/wall event this long → forced new heading (stuck escape; angled walls eat the ultrasonic ping)
#define SAMPLE_TIME_BUDGET_MS 8000              // don't start a sample without this much run time left (8-min overrun = -5 pts)
#define SECTOR_MAP {1, 2, 3, 4}                 // dead-reckoned quadrant (+x+y, -x+y, -x-y, +x-y) → sector number; confirm mapping on Testing Day
