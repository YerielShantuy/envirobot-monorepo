#include "navigation.h"
#include "config.h"
#include "position.h"
#include "sensors.h"
#include "servo_arm.h"
#include "data_logger.h"
#include <Arduino.h>

static FSMState state           = FSM_INIT;
static int  samplesCollected    = 0;
static unsigned long runStartMs = 0;
static unsigned long lastSampleEndMs = 0;  // zone-detection lockout
static unsigned long backoffStartMs  = 0;
static float targetHeading      = 0.0f;

// ── Motor helpers ────────────────────────────────────────────────
static float pwmToMmS(int pwm) {
  return MM_PER_SEC_AT_DRIVE * (float)pwm / (float)DRIVE_SPEED;
}

void initMotors() {
  pinMode(PIN_MOTOR_L_FWD, OUTPUT); pinMode(PIN_MOTOR_L_BWD, OUTPUT);
  pinMode(PIN_MOTOR_R_FWD, OUTPUT); pinMode(PIN_MOTOR_R_BWD, OUTPUT);
  ledcSetup(0, 5000, 8); ledcAttachPin(PIN_MOTOR_L_PWM, 0);
  ledcSetup(1, 5000, 8); ledcAttachPin(PIN_MOTOR_R_PWM, 1);
  driveStop();
}

void driveForward(int speed) {
  int s = abs(speed);
  bool fwd = speed >= 0;
  digitalWrite(PIN_MOTOR_L_FWD, fwd); digitalWrite(PIN_MOTOR_L_BWD, !fwd);
  digitalWrite(PIN_MOTOR_R_FWD, fwd); digitalWrite(PIN_MOTOR_R_BWD, !fwd);
  ledcWrite(0, s); ledcWrite(1, s);
  float v = pwmToMmS(speed);
  setCommandedSpeed(v, v);
}

void driveStop() {
  digitalWrite(PIN_MOTOR_L_FWD, LOW); digitalWrite(PIN_MOTOR_L_BWD, LOW);
  digitalWrite(PIN_MOTOR_R_FWD, LOW); digitalWrite(PIN_MOTOR_R_BWD, LOW);
  ledcWrite(0, 0); ledcWrite(1, 0);
  setCommandedSpeed(0, 0);
}

void turnLeft(int speed) {
  digitalWrite(PIN_MOTOR_L_FWD, LOW);  digitalWrite(PIN_MOTOR_L_BWD, HIGH);
  digitalWrite(PIN_MOTOR_R_FWD, HIGH); digitalWrite(PIN_MOTOR_R_BWD, LOW);
  ledcWrite(0, speed); ledcWrite(1, speed);
  setCommandedSpeed(-pwmToMmS(speed), pwmToMmS(speed));
}

void turnRight(int speed) {
  digitalWrite(PIN_MOTOR_L_FWD, HIGH); digitalWrite(PIN_MOTOR_L_BWD, LOW);
  digitalWrite(PIN_MOTOR_R_FWD, LOW);  digitalWrite(PIN_MOTOR_R_BWD, HIGH);
  ledcWrite(0, speed); ledcWrite(1, speed);
  setCommandedSpeed(pwmToMmS(speed), -pwmToMmS(speed));
}

// ── Heading controller — steer toward target angle ────────────────
static void steerToHeading(float targetRad) {
  Position p = getPosition();
  float err = targetRad - p.heading_rad;
  while (err >  M_PI) err -= 2 * M_PI;
  while (err < -M_PI) err += 2 * M_PI;
  if (fabsf(err) > 0.12f) {
    err > 0 ? turnLeft(TURN_SPEED) : turnRight(TURN_SPEED);
  } else {
    driveForward(DRIVE_SPEED);
  }
}

// ponytail: no localization — sector is estimated from collection order.
// Accurate sector labels come from RC mode where the operator selects it.
static int sectorEstimate() { return min(4, samplesCollected / 3 + 1); }

// Pick a new wander heading after a wall or a sample (~135° bounce)
static void newHeading(bool clockwise) {
  targetHeading += clockwise ? -2.36f : 2.36f;
  while (targetHeading >  M_PI) targetHeading -= 2 * M_PI;
  while (targetHeading < -M_PI) targetHeading += 2 * M_PI;
}

static bool lockoutActive() {
  return millis() - lastSampleEndMs < SAMPLE_LOCKOUT_MS;
}

// ── FSM ──────────────────────────────────────────────────────────
void initNavigation() {
  state = FSM_INIT;
  samplesCollected = 0;
}

void runFSM() {
  // Hard 8-minute cap — rulebook deducts points past the limit
  if (state != FSM_COMPLETE && state != FSM_INIT &&
      millis() - runStartMs > RUN_TIME_LIMIT_MS) {
    driveStop();
    Serial.println("[TIME] 8-minute limit reached");
    state = FSM_COMPLETE;
  }

  switch (state) {

    case FSM_INIT: {
      // Dead IMU = no heading feedback = the robot would spin at full
      // speed chasing a target it can never reach. Refuse to start.
      if (!headingValid()) {
        static bool warned = false;
        if (!warned) {
          Serial.println("[ERR] MPU6050 DMP dead — autonomous mode refused. Use RC mode.");
          warned = true;
        }
        driveStop();
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 100) {   // fast blink = fault
          digitalWrite(2, !digitalRead(2));
          lastBlink = millis();
        }
        break;
      }
      runStartMs = millis();
      lastSampleEndMs = 0;
      targetHeading = 0.0f;
      state = FSM_SWEEP;
      break;
    }

    case FSM_SWEEP: {
      if (samplesCollected >= 12) {
        driveStop();
        state = FSM_COMPLETE;
        break;
      }
      // Water zone: stop IMMEDIATELY — the slot is recessed and a wheel
      // entering it strands the robot. Colour sensor must be mounted
      // ahead of the front wheels so this fires before wheels reach it.
      if (!lockoutActive() && isWaterZone()) {
        driveStop();
        state = FSM_POND_SAMPLE;
        break;
      }
      if (!lockoutActive() && isSoilPatch()) {
        driveStop();
        state = FSM_MEASURE_SOIL;
        break;
      }
      // Wall ahead → bounce to a new heading
      if (readToFDistanceMM() < WALL_STOP_MM) {
        driveStop();
        newHeading(samplesCollected % 2 == 0);  // alternate bounce direction
      }
      steerToHeading(targetHeading);
      break;
    }

    case FSM_POND_SAMPLE: {
      armDeployWater();
      float ntu = readTurbidityNTU();
      armNeutral();
      logSample("WATER", ntu, 0.0f, sectorEstimate());
      samplesCollected++;
      Serial.printf("[SAMPLE] WATER %.1f NTU  (%d/12)\n", ntu, samplesCollected);
      backoffStartMs = millis();
      driveForward(-DRIVE_SPEED);   // reverse — never drive over the slot
      state = FSM_POND_BACKOFF;
      break;
    }

    case FSM_POND_BACKOFF:
      if (millis() - backoffStartMs >= POND_BACKOFF_MS) {
        driveStop();
        newHeading(true);           // turn away from the pond
        lastSampleEndMs = millis(); // lockout starts after the backoff
        state = FSM_SWEEP;
      }
      break;

    case FSM_MEASURE_SOIL: {
      armDeploySoil();
      float pct = readSoilMoisturePct();
      armNeutral();
      logSample("SOIL", 0.0f, pct, sectorEstimate());
      samplesCollected++;
      lastSampleEndMs = millis();
      Serial.printf("[SAMPLE] SOIL  %.1f%%  (%d/12)\n", pct, samplesCollected);
      newHeading(false);            // move off the patch before re-detecting
      state = FSM_SWEEP;
      break;
    }

    case FSM_COMPLETE: {
      // Non-blocking: finalise once, keep blinking; web server stays alive
      static bool finalised = false;
      if (!finalised) {
        driveStop();
        armNeutral();
        finaliseRun();
        Serial.println("[DONE] Run complete. Results saved to SPIFFS.");
        finalised = true;
      }
      static unsigned long lastBlink = 0;
      if (millis() - lastBlink > 300) {
        digitalWrite(2, !digitalRead(2));
        lastBlink = millis();
      }
      break;
    }
  }
}
