#include "position.h"
#include "config.h"
#include <Wire.h>
#include <MPU6050_6Axis_MotionApps20.h>

// No wheel encoders on this robot. Heading comes from the MPU6050 DMP
// (absolute yaw, zeroed at init); distance is estimated from commanded
// motor speed — rough, but only used for the path[] log, never for
// safety-critical decisions.

static MPU6050 mpu;
static uint8_t fifoBuffer[64];
static Position pos = {0.0f, 0.0f, 0.0f};
static bool  dmpOK       = false;
static float yawOffset   = 0.0f;
static float cmdL_mm_s   = 0.0f;
static float cmdR_mm_s   = 0.0f;
static unsigned long lastUpdateMs = 0;

static bool readYaw(float* yaw) {
  if (!dmpOK) return false;
  if (!mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) return false;
  Quaternion q;
  VectorFloat gravity;
  float ypr[3];
  mpu.dmpGetQuaternion(&q, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &q);
  mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
  *yaw = ypr[0];
  return true;
}

void initPosition() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  mpu.initialize();
  if (mpu.dmpInitialize() == 0) {
    mpu.setDMPEnabled(true);
    dmpOK = true;
    // Let DMP settle, then zero the heading to the starting orientation
    delay(2000);
    float y;
    for (int i = 0; i < 20; i++) { if (readYaw(&y)) yawOffset = y; delay(20); }
  } else {
    Serial.println("[WARN] MPU6050 DMP init failed — heading unavailable, position estimate degraded");
  }
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

  float yaw;
  if (readYaw(&yaw)) {
    pos.heading_rad = yaw - yawOffset;
  }
  // Normalise to -π..π
  while (pos.heading_rad >  M_PI) pos.heading_rad -= 2 * M_PI;
  while (pos.heading_rad < -M_PI) pos.heading_rad += 2 * M_PI;

  // Speed-model distance: average of commanded wheel speeds.
  // In-place turns (L = -R) contribute zero forward motion.
  float v = (cmdL_mm_s + cmdR_mm_s) / 2.0f;
  pos.x_mm += v * dt * cosf(pos.heading_rad);
  pos.y_mm += v * dt * sinf(pos.heading_rad);
}

Position getPosition() { return pos; }

bool headingValid() { return dmpOK; }

void resetPosition() {
  pos = {0.0f, 0.0f, 0.0f};
  float y;
  if (readYaw(&y)) yawOffset = y;
}
