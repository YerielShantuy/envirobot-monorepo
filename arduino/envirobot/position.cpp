#include "position.h"
#include "config.h"
#include <Arduino.h>

// No wheel encoders and no IMU on this robot. Both heading and distance
// are integrated open-loop from the commanded wheel speeds using the
// standard differential-drive model:
//   v     = (cmdL + cmdR) / 2              forward speed
//   omega = (cmdR - cmdL) / config.wheelBaseMm  turn rate (rad/s)
// Rough, and it drifts (nothing corrects wheel slip) — used only for the
// path[] log, never for safety-critical decisions.

static Position pos = {0.0f, 0.0f, 0.0f};
static float cmdL_mm_s   = 0.0f;
static float cmdR_mm_s   = 0.0f;
static unsigned long lastUpdateMs = 0;

void initPosition() {
  pos = {0.0f, 0.0f, 0.0f};
  lastUpdateMs = millis();
}

void setCommandedSpeed(float left_mm_s, float right_mm_s) {
  cmdL_mm_s = left_mm_s;
  cmdR_mm_s = right_mm_s;
}

void updatePosition() {
  unsigned long now = millis();
  if (now - lastUpdateMs < POS_UPDATE_MS) return;
  float dt = (now - lastUpdateMs) / 1000.0f;
  lastUpdateMs = now;

  // Turn rate from the wheel-speed difference. In-place turns (L = -R)
  // rotate heading with zero forward motion.
  float omega = (cmdR_mm_s - cmdL_mm_s) / config.wheelBaseMm;
  pos.heading_rad += omega * dt;
  // Normalise to -π..π
  while (pos.heading_rad >  M_PI) pos.heading_rad -= 2 * M_PI;
  while (pos.heading_rad < -M_PI) pos.heading_rad += 2 * M_PI;

  float v = (cmdL_mm_s + cmdR_mm_s) / 2.0f;
  pos.x_mm += v * dt * cosf(pos.heading_rad);
  pos.y_mm += v * dt * sinf(pos.heading_rad);
}

Position getPosition() { return pos; }

// No IMU to fail — open-loop heading is always available (just imprecise).
bool headingValid() { return true; }

// Re-zero at a known pose — used at run start and after officials
// re-centre a stuck robot (REZERO cmd; operator eyeballs the heading).
void resetPose(float heading_rad) {
  pos = {0.0f, 0.0f, heading_rad};
  // Reset the integration clock too — otherwise the first update after a
  // reset applies all motion since the previous update to the new origin.
  lastUpdateMs = millis();
}

void resetPosition() { resetPose(0.0f); }
