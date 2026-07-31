#pragma once

#include "hardware_pins.h"
#include "robot_config.h"

// ── WiFi ────────────────────────────────────────────────────────
// AP-ONLY. The robot IS the access point: join "RobotAP" from the phone or
// laptop and everything (RC page, live data, /data download) happens on that
// direct link. No router, no hotspot, no internet, nothing to configure at
// the venue and nothing to fail if the venue WiFi is congested.
// Reach it at http://192.168.4.1 or http://envirobot.local
// There is no cloud upload — run data leaves the robot as a downloaded
// results.json (⬇ Download JSON on the RC page), which is what the webapp
// visualiser plays back. Serial sample lines stay the rulebook-approved
// output for scored runs (§4.6).
#define WIFI_SSID "RobotAP"                                     // AP SSID
#define WIFI_PASS "enviro123"                                   // AP password (8+ chars)
#define WIFI_AP_CHANNEL 1

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

// ── Collision detection (HC-SR04) ─────────────────────────────────
// Forward drive is cut when the ultrasonic reads closer than
// config.obstacleStopMm (calibrate that number on the real arena — it is a
// runtime knob in the config editor, not a compile-time constant).
#define OBSTACLE_PING_MS 100   // ping rate — one pulseIn per tick, keeps RC latency low
#define OBSTACLE_TRIP_HITS 2   // consecutive in-range pings before cutting motors (one HC-SR04 glitch must not stop the robot)
#define OBSTACLE_HYST_MM 40    // must read this much FURTHER than the threshold to clear the block (hysteresis — a bare threshold chatters)
