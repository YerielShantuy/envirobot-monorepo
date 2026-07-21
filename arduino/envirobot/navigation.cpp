#include "navigation.h"
#include "config.h"
#include "position.h"
#include "sensors.h"
#include "servo_arm.h"
#include "data_logger.h"
#include <Arduino.h>

#if ENABLE_AUTONOMOUS
static FSMState state           = FSM_INIT;
static int  samplesCollected    = 0;
static int  waterSampled        = 0;       // cap 4 — one water pot per sector
static int  soilSampled         = 0;       // cap 8 — two soil pots per sector
static bool startRequested      = false;   // FSM_INIT gate: START cmd or BOOT button
static bool backoffNoSample     = false;   // avoid-backoff (no sample) → don't re-arm lockout
static unsigned long runStartMs = 0;
static unsigned long lastSampleEndMs = 0;  // zone-detection lockout
static unsigned long backoffStartMs  = 0;
static unsigned long legStartMs      = 0;  // stuck escape: time on current heading leg
static float targetHeading      = 0.0f;
#endif

// ── Motor helpers ────────────────────────────────────────────────
static float pwmToMmS(int pwm) {
  return config.mmPerSecAtDrive * (float)pwm / (float)config.driveSpeed;
}

// LEDC API changed in ESP32 Arduino core 3.x (channel-based → pin-based)
static void motorPwm(int l, int r) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_MOTOR_L_PWM, l); ledcWrite(PIN_MOTOR_R_PWM, r);
#else
  ledcWrite(0, l); ledcWrite(1, r);
#endif
}

void initMotors() {
  pinMode(PIN_MOTOR_L_FWD, OUTPUT); pinMode(PIN_MOTOR_L_BWD, OUTPUT);
  pinMode(PIN_MOTOR_R_FWD, OUTPUT); pinMode(PIN_MOTOR_R_BWD, OUTPUT);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_MOTOR_L_PWM, 5000, 8);
  ledcAttach(PIN_MOTOR_R_PWM, 5000, 8);
#else
  ledcSetup(0, 5000, 8); ledcAttachPin(PIN_MOTOR_L_PWM, 0);
  ledcSetup(1, 5000, 8); ledcAttachPin(PIN_MOTOR_R_PWM, 1);
#endif
  driveStop();
}

void driveForward(int speed) {
  int s = abs(speed);
  bool fwd = speed >= 0;
  digitalWrite(PIN_MOTOR_L_FWD, fwd); digitalWrite(PIN_MOTOR_L_BWD, !fwd);
  digitalWrite(PIN_MOTOR_R_FWD, fwd); digitalWrite(PIN_MOTOR_R_BWD, !fwd);
  motorPwm(s, s);
  float v = pwmToMmS(speed);
  setCommandedSpeed(v, v);
}

void driveStop() {
  digitalWrite(PIN_MOTOR_L_FWD, LOW); digitalWrite(PIN_MOTOR_L_BWD, LOW);
  digitalWrite(PIN_MOTOR_R_FWD, LOW); digitalWrite(PIN_MOTOR_R_BWD, LOW);
  motorPwm(0, 0);
  setCommandedSpeed(0, 0);
}

// Active short-brake: both direction inputs HIGH shorts the motor terminals
// (TB6612 / L298N / DRV8833), dumping momentum so FWD/BWD stop dead instead of
// coasting. Pulse it for BRAKE_MS then call driveStop() to de-energize —
// holding brake just heats the driver.
void driveBrake() {
  digitalWrite(PIN_MOTOR_L_FWD, HIGH); digitalWrite(PIN_MOTOR_L_BWD, HIGH);
  digitalWrite(PIN_MOTOR_R_FWD, HIGH); digitalWrite(PIN_MOTOR_R_BWD, HIGH);
  motorPwm(255, 255);
  setCommandedSpeed(0, 0);
}

void turnLeft(int speed) {
  digitalWrite(PIN_MOTOR_L_FWD, LOW);  digitalWrite(PIN_MOTOR_L_BWD, HIGH);
  digitalWrite(PIN_MOTOR_R_FWD, HIGH); digitalWrite(PIN_MOTOR_R_BWD, LOW);
  motorPwm(speed, speed);
  setCommandedSpeed(-pwmToMmS(speed), pwmToMmS(speed));
}

void turnRight(int speed) {
  digitalWrite(PIN_MOTOR_L_FWD, HIGH); digitalWrite(PIN_MOTOR_L_BWD, LOW);
  digitalWrite(PIN_MOTOR_R_FWD, LOW);  digitalWrite(PIN_MOTOR_R_BWD, HIGH);
  motorPwm(speed, speed);
  setCommandedSpeed(pwmToMmS(speed), -pwmToMmS(speed));
}

#if ENABLE_AUTONOMOUS
// ── Heading controller — steer toward target angle ────────────────
static void steerToHeading(float targetRad) {
  Position p = getPosition();
  float err = targetRad - p.heading_rad;
  while (err >  M_PI) err -= 2 * M_PI;
  while (err < -M_PI) err += 2 * M_PI;
  if (fabsf(err) > config.headingDeadbandRad) {
    err > 0 ? turnLeft(config.turnSpeed) : turnRight(config.turnSpeed);
  } else {
    driveForward(config.driveSpeed);
  }
}

// Sector from the dead-reckoned quadrant. Start pose convention: robot at
// arena centre, heading = +x (quadrant boundaries are the sector lines).
// Quadrant-level resolution outlives drift far longer than mm accuracy.
// SECTOR_MAP in config.h maps quadrant → rulebook sector number.
static const int SECTOR_MAP_ARR[4] = SECTOR_MAP;
static int sectorEstimate() {
  Position p = getPosition();
  int q = (p.x_mm >= 0) ? (p.y_mm >= 0 ? 0 : 3) : (p.y_mm >= 0 ? 1 : 2);
  return SECTOR_MAP_ARR[q];
}

// Pick a new wander heading after a wall or a sample (~135° bounce).
// millis-based jitter (±0.35 rad) breaks corner ping-pong loops that a
// fixed bounce angle can lock into on the octagon's 135° walls.
static void newHeading(bool clockwise) {
  float jitter = ((int)(millis() % 141) - 70) / 200.0f;
  targetHeading += (clockwise ? -1.0f : 1.0f) * (2.36f + jitter);
  while (targetHeading >  M_PI) targetHeading -= 2 * M_PI;
  while (targetHeading < -M_PI) targetHeading += 2 * M_PI;
  legStartMs = millis();
}

static bool lockoutActive() {
  return millis() - lastSampleEndMs < SAMPLE_LOCKOUT_MS;
}

// Enough run time left to finish a full sample cycle? Overrunning the
// 8-minute cap costs points — better to skip the last sample.
static bool timeForSample() {
  return millis() - runStartMs < RUN_TIME_LIMIT_MS - SAMPLE_TIME_BUDGET_MS;
}

// TCS detects at the sensor footprint, but the probe tip lands behind it.
// Bench-measure the TCS→probe offset and set SAMPLE_CREEP_MS (0 = off).
static void creepToTarget() {
  if (SAMPLE_CREEP_MS <= 0) return;
  driveForward(SAMPLE_CREEP_SPEED);
  delay(SAMPLE_CREEP_MS);
  driveStop();
}

// ── FSM ──────────────────────────────────────────────────────────
void initNavigation() {
  state = FSM_INIT;
  samplesCollected = 0;
  waterSampled = 0;
  soilSampled  = 0;
  startRequested  = false;
  backoffNoSample = false;
}

// Called from the web server on {"cmd":"START"} — also triggered by a
// BOOT button press while armed (see FSM_INIT).
void navigationStart() { startRequested = true; }

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
      // ARMED, not running: without this gate the robot starts driving
      // (and burning its 8-min clock) the moment power connects. Start
      // signal: {"cmd":"START"} from the RC page, or a BOOT press.
      // LED blinks slowly while armed, goes solid when the run starts.
      static unsigned long lastBlink = 0;
      if (millis() - lastBlink > 600) {
        digitalWrite(2, !digitalRead(2));
        lastBlink = millis();
      }
      if (digitalRead(0) == LOW) startRequested = true;
      if (!startRequested) break;

      digitalWrite(2, HIGH);
      // Heading is open-loop dead reckoning (no IMU) — always available,
      // but it drifts, so this sweep is a stretch goal. RC mode is primary.
      // Start pose convention: arena centre, heading = +x (sector lines
      // are the quadrant boundaries — see SECTOR_MAP).
      resetPosition();
      startRun();       // fresh doc + run clock — t_s/duration/path from START, not boot
      runStartMs = millis();
      // Backdate so the first sample is allowed immediately — a plain 0
      // reads as an active lockout when START comes <4s after boot.
      lastSampleEndMs = millis() - SAMPLE_LOCKOUT_MS;
      targetHeading = 0.0f;
      legStartMs = millis();
      Serial.println("[RUN] started");
      state = FSM_SWEEP;
      break;
    }

    case FSM_SWEEP: {
      if (samplesCollected >= 12 || (waterSampled >= 4 && soilSampled >= 8)) {
        driveStop();
        state = FSM_COMPLETE;
        break;
      }
      // Water zone: stop IMMEDIATELY — the slot is recessed and a wheel
      // entering it strands the robot. Colour sensor must be mounted
      // ahead of the front wheels so this fires before wheels reach it.
      // Detection runs EVERY iteration: the sample lockout gates
      // re-sampling only, never hazard avoidance — 4s blind at driveSpeed
      // is ~600mm, enough to drop a wheel into a slot.
      {
        bool water, soil;
        readZones(&water, &soil);   // one ~24ms read for both checks
        bool soilWanted = soil && !lockoutActive() && soilSampled < 8 && timeForSample();
        if (water || soilWanted) {
          driveStop();
          // One confirmation read — a single glitched integration (venue
          // lighting, reflections) otherwise costs a ~6s phantom sample.
          bool w2, s2;
          readZones(&w2, &s2);
          water = water && w2;
          soilWanted = soilWanted && s2;
        }
        if (water) {
          if (!lockoutActive() && waterSampled < 4 && timeForSample()) {
            creepToTarget();
            state = FSM_POND_SAMPLE;
          } else {
            // Avoid without sampling: reverse away exactly like the
            // post-sample path, just skip FSM_POND_SAMPLE.
            Serial.println("[POND] avoided (no-sample backoff)");
            backoffNoSample = true;
            backoffStartMs = millis();
            driveForward(-config.driveSpeed);
            state = FSM_POND_BACKOFF;
          }
          break;
        }
        if (soilWanted) {
          creepToTarget();
          state = FSM_MEASURE_SOIL;
          break;
        }
      }
      // Wall ahead → bounce to a new heading
      if (readWallDistanceMM() < config.wallStopMm) {
        driveStop();
        newHeading(samplesCollected % 2 == 0);  // alternate bounce direction
      }
      // Stuck escape: no zone/wall/sample event on this leg for too long.
      // Shallow-angle wall contact reflects the ultrasonic ping away and
      // reads as "no wall" while the robot grinds in place.
      if (millis() - legStartMs > SWEEP_LEG_TIMEOUT_MS) {
        driveStop();
        Serial.println("[STUCK] leg timeout — new heading");
        newHeading((millis() & 1) == 0);
      }
      steerToHeading(targetHeading);
      break;
    }

    case FSM_POND_SAMPLE: {
      armDeployWater();
      float ntu = readTurbidityNTU();
      armNeutral();
      int sec = sectorEstimate();
      logSample("WATER", ntu, 0.0f, sec);
      samplesCollected++;
      waterSampled++;
      // Same "S%d" format as RC mode — tools/live_plot.py parses this line
      Serial.printf("[SAMPLE] S%d WATER %.1f NTU  (%d/12)\n", sec, ntu, samplesCollected);
      backoffNoSample = false;
      backoffStartMs = millis();
      driveForward(-config.driveSpeed);   // reverse — never drive over the slot
      state = FSM_POND_BACKOFF;
      break;
    }

    case FSM_POND_BACKOFF:
      if (millis() - backoffStartMs >= POND_BACKOFF_MS) {
        driveStop();
        newHeading(true);           // turn away from the pond
        // Only a REAL sample re-arms the lockout. Re-arming after an
        // avoid-backoff starves any unsampled pond the robot circles near.
        if (!backoffNoSample) lastSampleEndMs = millis();
        backoffNoSample = false;
        state = FSM_SWEEP;
      }
      break;

    case FSM_MEASURE_SOIL: {
      armDeploySoil();
      float pct = readSoilMoisturePct();
      armNeutral();
      int sec = sectorEstimate();
      logSample("SOIL", 0.0f, pct, sec);
      samplesCollected++;
      soilSampled++;
      lastSampleEndMs = millis();
      Serial.printf("[SAMPLE] S%d SOIL %.1f%%  (%d/12)\n", sec, pct, samplesCollected);
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
#endif  // ENABLE_AUTONOMOUS
