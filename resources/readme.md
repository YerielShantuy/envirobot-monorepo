# Autonomous Terrain Survey Robot — Master Plan (Rev 5)

---

## 0. Competition Quick Reference

| Item | Value |
|---|---|
| Competition | EnviroBot — Environmental Robotics Challenge |
| Run limit | **8 minutes** maximum |
| Arena | Octagonal, ~1.5m diameter, 4 terrain sectors |
| Measurements | 12 total (4 water turbidity + 8 soil moisture) |
| Testing Day | Thu W9 (robot run + inspection + 3-min innovation pitch) |
| Closing Night | Fri 31 July, 5–9pm (case competition presentation) |

### Scoring Breakdown

| # | Category | Points |
|---|---|---|
| 1 | Technical Measurement Performance | **30** |
| 2 | Data Output & Visualisation | **10** |
| 3 | Build Quality & Workmanship | **5** |
| 4 | Innovation & Engineering Design | **10** |
| 5 | Robot Aesthetics | **5** |
| 6 | Closing Night Case Competition | **40** |
| | **TOTAL** | **100** |

> **Rulebook inconsistency noted:** Section headers list Cat 1 as 40pts and Cat 6 as 25pts — conflicting with the scoring table above. The table totals correctly to 100, so treat it as authoritative. However, Cat 6 is the highest single category by any reading — prioritise the Day 2 presentation.

**Deductions to avoid:**

| Infraction | Deduction |
|---|---|
| Prohibited components | –10 pts or DQ |
| Run overtime | –5 pts |
| Pre-loaded / hardcoded sensor data | Cat 1 & 2 zeroed |
| Arena surface damage | –5 pts per incident |

### 8-Minute Time Budget Analysis

At 100 mm/s drive speed with 6 strips per sector:

| Phase | Time/sector | × Sectors | Total |
|---|---|---|---|
| Sector sweep (6 strips × 11.5s) | 69s | ×4 | 276s |
| Measurements (3 zones × ~10s) | 30s | ×4 | 120s |
| Inter-sector transit to centre | —  | ×3 | 30s |
| Buffer (rerouting, RECOVERY) | — | — | 30s |
| **Total** | | | **~456s (7.6 min) ✓** |

---

## 1. Mission

Navigate an octagonal 4-sector arena (~1.5m diameter). Each sector contains **1 water zone** and **2 soil patches** at random positions within the sector. Water zones are recessed diamond-shaped slots embedded in the terrain. The robot samples from the edge and reroutes around. Track GPS-less position via dead-reckoning (encoder odometry + IMU fusion). Maintain exclusion zones to avoid re-sampling. Log **12 measurements** within 8 minutes.

---

## 2. Arena Definition

| Parameter | Value |
|---|---|
| Shape | Octagonal, ~1.5m diameter, dark-stained timber walls |
| Sectors | 4 quadrants (~90° each, ~750mm radius) |
| Center | Crosspoint of centerlines — navigation anchor and dead-reckoning re-sync |
| Sector boundaries | **No physical dividers** — terrain surface change only (TCS34725) |
| Outer wall | Physical octagon timber wall — detected by VL53L0X |
| Zones per sector | 3 — 1 water zone + 2 soil patches |
| Total measurements | 12 (4 water turbidity + 8 soil moisture) |
| Water zones | Recessed diamond-shaped slots (~45° rotation viewed from above), blue water, **random positions within sector** |
| Soil patches | Flat reddish-brown circular discs embedded in surface, ~30mm radius, **random positions within sector** |

### Sector Terrain Types

| Sector | Terrain | Dominant Colour | Notes |
|---|---|---|---|
| 1 (top-left) | Sandpaper | Light beige / matte gray | High friction terrain |
| 2 (bottom-left) | Grass | Muted green | Uneven, high drag |
| 3 (top-right) | Wet soil | Dark brown, moist | Reflective highlights |
| 4 (bottom-right) | Sand | Yellow-tan, rippled | Wheel sinking risk |

### Arena Geometry

```
           Outer octagon wall (~1.5m diameter, dark timber)
          ╱──────────────────────────────────────╲
         ╱  S1: Sandpaper (beige) │ S3: Wet Soil  ╲
        ╱   ◇ water  ● soil      │  ● soil ◇ water ╲
       │    ● soil               ─┼─         ● soil  │
        ╲   S2: Grass (green)   ─ ─  S4: Sand        ╱
         ╲  ◇ water  ● soil      │  ● soil ◇ water  ╱
          ╲─────────────────────────────────────────╱

Centre crosspoint: dead-reckoning re-sync between sectors
No walls between sectors — boundary = terrain colour change (TCS34725)
Outer wall = only physical barrier (detected by VL53L0X)
◇ = water slot (diamond, blue)   ● = soil disc (reddish-brown)
```

---

## 3. Hardware (Rev 5)

### 3.1 Controller — ESP32 (from kit)

Dual-core Xtensa LX6, 240 MHz, 4 MB flash, WiFi 802.11 b/g/n, 2× hardware I2C buses.

### 3.2 Colour & Distance Sensing

| Component | Qty | Role |
|---|---|---|
| TCS34725 | 3 | L/C/R front array — terrain ID, zone detection, sector boundary |
| TCA9548A | 1 | I2C mux at 0x70 — all 3 TCS34725 share address 0x29 |
| VL53L0X | 1 | Forward-facing ToF — outer wall detection (on Wire1, avoids 0x29 conflict) |

### 3.3 Position Tracking — Dead-Reckoning 2.0

| Component | Qty | Role |
|---|---|---|
| MPU6050 IMU | 1 | 6-axis IMU — yaw integration via DMP for heading correction |
| Hall-effect encoder | 2 | One per motor shaft — pulse count → distance per wheel (odometry) |

Combined accuracy: **±20 mm over 750 mm sector** (vs ±60 mm pure time-based).

### 3.4 Measurement — Single Servo Arm (Seesaw Mechanism)

```
           SERVO SHAFT (pivot)
                  │
    ┌─────────────┴─────────────┐
    │                           │
  Arm A                       Arm B
  SEN0189                 Cap. Moisture v2.0
  (Water probe)            (Soil probe)

Servo 90°:   NEUTRAL — both arms horizontal (travelling position)
Servo 0°:    Arm A DOWN — water sensor deployed  (CCW)
Servo 180°:  Arm B DOWN — soil sensor deployed   (CW)
```

One servo replaces two linear actuators — simpler, lighter, fewer failure modes.

### 3.5 Actuation

| Component | Role | Driver |
|---|---|---|
| JGA25-370 geared motor 12V 300RPM (×2) | Differential drive | L298N (from kit) |
| MG996R servo (from kit) | Seesaw sensor arm | Direct PWM from ESP32 GPIO 16 |

### 3.6 Power — 12V from 8× AA 1.5V Batteries

> **L298N takes 12V directly** — only ONE LM2596 needed (5V logic rail). This resolves the original two-buck problem.

```
8× AA (12V nominal, ~2000–3000 mAh)
    │
    ├── [KILL SWITCH] ← large latching button, top of chassis, accessible
    │
    ├── L298N VM pin (12V direct, ~2V L298N drop → 10V effective at motors ✓)
    │
    ├── LM2596 Buck → 5V ──→ SEN0189 (5V required)
    │                         MG996R servo (4.8–7.2V ✓)
    │                         L298N logic pin
    │
    └── AMS1117-3.3 (from 5V) ──→ ESP32
                                    TCS34725 ×3 · TCA9548A
                                    VL53L0X (Wire1)
                                    MPU6050
                                    Capacitive moisture sensor
                                    Hall encoders (signal via 5V→3.3V divider)
```

### 3.7 Physical Compliance

| Constraint | Limit | Estimated | Status |
|---|---|---|---|
| Length | 400 mm | 200 mm (chassis) | ✓ |
| Width | 300 mm | 160 mm | ✓ |
| Height (starting config, arm neutral) | 400 mm | ~250 mm | ✓ |
| Weight | 1.5 kg | ~850 g (see below) | ✓ |

**Weight estimate:**

| Component | Est. |
|---|---|
| 8× AA batteries | 200 g |
| JGA25-370 motors ×2 | 150 g |
| Chassis (3D printed + laser cut) | 250 g |
| Electronics (ESP32, L298N, sensors, PCB) | 120 g |
| Servo + arm assembly | 80 g |
| Wiring + fasteners | 50 g |
| **Total** | **~850 g ✓** |

---

## 4. Why 3 TCS34725 Sensors

| Capability | 1 sensor | 3 sensors |
|---|---|---|
| Detect zone exists | ✓ | ✓ |
| Centre arm accurately over zone | ✗ | ✓ |
| Detect water slot from side approach | ✗ | ✓ |
| Distinguish zone vs sector boundary | ✗ | ✓ |
| 2.5× wider strip coverage per pass | ✗ | ✓ |
| Detect reddish-brown soil disc | ✓ | ✓ |
| Detect blue water slot reflectance | ✓ | ✓ |

### Centering logic

```
L reads    C reads    R reads    → Action
TERRAIN    ZONE       TERRAIN    → Perfectly centred — deploy arm
ZONE       ZONE       TERRAIN    → Drifted left — nudge right
TERRAIN    ZONE       ZONE       → Drifted right — nudge left
ZONE       TERRAIN    TERRAIN    → Zone to the left — steer toward
```

### Why 3 is enough for the 1.5m arena

```
1 sensor:   max strip = 2 × zone_radius = 60mm per pass
3 sensors:  max strip = 2 × zone_radius + 60mm spread = 120mm per pass
Result:     2× fewer strips → faster sector scan → fits 8-min budget
```

---

## 5. Chassis Layout

```
                 FRONT
    ┌──────────────────────────────────┐
    │  [VL53L0X]    [KILL SWITCH]     │ ← wall sensor + power cutoff
    │  [TCS-L]  [TCS-C]  [TCS-R]    │ ← 8 mm above ground
    │                                  │
    │  ╔══ SERVO ARM (seesaw) ════╗   │ ← front-mounted, pivots down
    │  ║  [SEN0189] ←→ [Soil]   ║   │
    │  ╚══════════════════════════╝   │
    │                                  │
    │  ◉ Motor-L     Motor-R ◉       │ ← hall encoders on motor shafts
    │  [ESP32] [L298N] [MPU6050]     │
    │  [8× AA pack 12V] [LM2596]     │
    └──────────────────────────────────┘
                 REAR

Chassis: ~200mm × 160mm × 250mm (H, arm neutral)
Wheel axle: ≥ 80mm behind front TCS34725 sensors
Servo arm pivot: at front bumper, arm ~60mm each side
```

---

## 6. Pin Assignment (ESP32 — Rev 5)

| GPIO | Function |
|---|---|
| 8 / 9 | I2C Wire0 SDA / SCL — TCA9548A (0x70), MPU6050 (0x68) |
| 21 / 22 | I2C Wire1 SDA / SCL — VL53L0X only (avoids 0x29 address conflict) |
| 1 | SEN0189 ADC — 10 kΩ/20 kΩ voltage divider (×1.5 correction) |
| 2 | Capacitive moisture ADC — direct, 3.3V safe |
| 10 | L298N ENA — Motor L speed (PWM) |
| 11 | L298N IN1 — Motor L direction A |
| 12 | L298N IN2 — Motor L direction B |
| 13 | L298N ENB — Motor R speed (PWM) |
| 14 | L298N IN3 — Motor R direction A |
| 15 | L298N IN4 — Motor R direction B |
| 16 | MG996R Servo PWM — 50 Hz, 1–2 ms pulse width |
| 17 | Hall encoder LEFT — interrupt (IRAM_ATTR) |
| 18 | Hall encoder RIGHT — interrupt (IRAM_ATTR) |
| 23 | NeoPixel RGB LED status ring |

### I2C Address Map

| Device | Address | Bus | Notes |
|---|---|---|---|
| TCA9548A | 0x70 | Wire0 | Main I2C mux |
| TCS34725 Left | 0x29 | Wire0 via TCA ch0 | Mux-isolated ✓ |
| TCS34725 Centre | 0x29 | Wire0 via TCA ch1 | Mux-isolated ✓ |
| TCS34725 Right | 0x29 | Wire0 via TCA ch2 | Mux-isolated ✓ |
| MPU6050 | 0x68 | Wire0 | No conflict ✓ |
| VL53L0X | 0x29 | Wire1 | Separate bus — no conflict ✓ |

---

## 7. Dead-Reckoning 2.0 — Encoder Odometry + IMU Fusion

### Encoder calibration

```cpp
// Calibrate once: drive exactly 500mm forward, count total encoder ticks
// Example: JGA25-370 300RPM with 48 PPR encoder, 65mm wheel:
// Theoretical: π × 65 / 48 = 4.25mm/tick — MEASURE actual value

volatile int32_t enc_left = 0, enc_right = 0;

void IRAM_ATTR onEncLeft()  { enc_left++;  }
void IRAM_ATTR onEncRight() { enc_right++; }

#define MM_PER_TICK    4.25f  // measure and update at build time
#define WHEEL_BASE_MM  120.0f // centre-to-centre wheel distance
```

### Position update (every 20ms)

```cpp
struct RobotPos { float x, y, heading; };
RobotPos pos = {0, 0, 0};  // origin = arena centre

float get_imu_delta_yaw();  // implemented via MPU6050 DMP FIFO read

void update_pos_v2() {
    // Snapshot and reset encoder counts atomically
    noInterrupts();
    int32_t dl_ticks = enc_left;
    int32_t dr_ticks = enc_right;
    enc_left = enc_right = 0;
    interrupts();

    float dl = dl_ticks * MM_PER_TICK;
    float dr = dr_ticks * MM_PER_TICK;
    float dc = (dl + dr) * 0.5f;

    float dtheta_enc = (dr - dl) / WHEEL_BASE_MM;
    float dtheta_imu = get_imu_delta_yaw();  // radians since last call

    // Fuse: 60% IMU heading, 40% encoder heading
    float dtheta = 0.6f * dtheta_imu + 0.4f * dtheta_enc;

    pos.heading += dtheta;
    pos.x += cosf(pos.heading) * dc;
    pos.y += sinf(pos.heading) * dc;
}
// Called every 20ms from FreeRTOS navigation task
```

### Exclusion zones

```cpp
#define EXCL_RADIUS 120.0f  // mm — absorbs ±20mm odometry error + zone radius margin

bool already_sampled(float x, float y) {
    for (int i = 0; i < sample_count; i++) {
        float d = hypotf(x - past_samples[i].x, y - past_samples[i].y);
        if (d < EXCL_RADIUS) return true;
    }
    return false;
}
```

### Drift mitigation summary

| Technique | Benefit |
|---|---|
| Encoder odometry | 5× more accurate than time-based (±20mm vs ±60mm) |
| MPU6050 DMP yaw fusion | Corrects heading drift from asymmetric wheel slip |
| Sector boundary re-align per strip | Resets lateral drift to zero at every strip edge |
| Arena centre transit | Full pos/heading re-sync (0,0) between sectors |
| 120mm exclusion radius | Absorbs remaining accumulated drift |

---

## 8. Water Zone — Sampling and Rerouting

Water zones are diamond-shaped recessed slots (~45° rotation). Robot cannot drive into them. On detection:

1. Stop at edge
2. Check exclusion zone — already sampled? → skip to reroute
3. Deploy water sensor (servo CCW to 0°) → stabilise 1.5s → read NTU → retract (90°)
4. Record position + exclusion zone, POST JSON, write SPIFFS
5. Navigate around the slot
6. Resume lawnmower sweep

### Circumnavigation algorithm

```
   Current heading →
   ─────────────────────[WATER SLOT]────────────────────
                              ↑
                        Water detected
                              │
   ←────── Reverse 80mm ─────┘
                              │
              Turn right 90° (TURN_90_MS = 1885ms)
                              │
   ──────→ Follow slot edge (RIGHT sensor: water, CENTER: terrain)
                                              │
                                    Turn left 90°
                                              │
   ──────────────────────────────────────────→
   Restore original heading and resume sweep
```

```cpp
#define TURN_90_MS 1885  // π/2 × 120mm wheelbase / 100mm/s × 1000 = 1885ms

void reroute_around_pond() {
    reverse(100, 800);               // back away 80mm
    turn_right(100, TURN_90_MS);

    while (true) {
        bool wR = in_water_range(sensors[RIGHT]);
        bool wC = in_water_range(sensors[CENTER]);
        bool wL = in_water_range(sensors[LEFT]);

        if (wC || wL)   { turn_right(60, 200); }          // slot creeping left
        else if (!wR)   { forward(100, 500); break; }     // cleared slot edge
        else            { forward(100, 100); }             // parallel to slot

        update_pos_v2();
    }

    turn_left(100, TURN_90_MS);   // restore heading
    state = SWEEP;
}
```

---

## 9. Navigation — Speed and Timing (Rev 5)

### Why 100 mm/s (revised from 40 mm/s)

At 40 mm/s: ~12–16 minute total run — **exceeds 8-minute limit.**
At 100 mm/s: ~7.6 minute total — safely within limit with 24s buffer.

```
At 100mm/s, TCS34725 polled every 50ms → 5mm positional resolution. ✓
VL53L0X range ≥ 100mm → robot stops before hitting wall at any speed. ✓
Encoder tick at 100mm/s with 4.25mm/tick → 23.5 ticks/s — clean interrupt rate. ✓
```

### TURN_90_MS derivation

```
Wheelbase (centre-to-centre): 120mm
Point turn: inner wheel stationary, outer wheel drives arc
Arc length (outer wheel): π/2 × 120 = 188.5mm
Time at 100mm/s: 188.5 / 100 = 1.885s

TURN_90_MS = 1885
```

### Terrain-specific PWM compensation

| Terrain | Resistance | PWM factor | Target speed |
|---|---|---|---|
| Sandpaper | High friction | 100% base | 100 mm/s |
| Grass | High drag, uneven | +20% | ~100 mm/s |
| Wet soil | Slipping, lower grip | +10% | ~100 mm/s |
| Sand | Wheel sinking | –10% | ~90 mm/s |

### Colour classification functions

```cpp
// Calibrated at sector start via recalibrate_for_sector()
ColorReading terrain_baseline;

float color_distance(ColorReading a, ColorReading b) {
    float tA = a.r + a.g + a.b + 1, tB = b.r + b.g + b.b + 1;
    float rA = (float)a.r/tA, gA = (float)a.g/tA;
    float rB = (float)b.r/tB, gB = (float)b.g/tB;
    return sqrtf(powf(rA-rB,2) + powf(gA-gB,2));
}

#define TERRAIN_THRESHOLD 0.25f  // tune per arena lighting

bool in_current_terrain(ColorReading r) {
    return color_distance(r, terrain_baseline) < TERRAIN_THRESHOLD;
}

bool at_sector_boundary() {
    // All 3 must agree — prevents zone-edge false positives
    return !in_current_terrain(sensors[L]) &&
           !in_current_terrain(sensors[C]) &&
           !in_current_terrain(sensors[R]);
}

bool in_water_range(ColorReading r) {
    // Water slot: high blue, low red, reflective
    float t = r.r + r.g + r.b + 1;
    return ((float)r.b / t) > 0.50f;  // tune with actual slot
}

bool in_soil_disc(ColorReading r) {
    // Soil disc: reddish-brown — high red, low blue
    float t = r.r + r.g + r.b + 1;
    return ((float)r.r / t) > 0.40f && ((float)r.b / t) < 0.20f;
}
```

---

## 10. State Machine (Rev 5 — with RECOVERY state)

```
INIT
 │  Init Wire0/Wire1 · WiFi AP "RobotAP" · MPU6050 DMP
 │  Encoder ISRs · L298N · Servo neutral (90°)
 │  LM2596 → 5V, AMS1117 → 3.3V stable
 │  auto-calibrate 5s · clear sample log · pos=(0,0,0) · sector=0
 ↓
SWEEP  ← lawnmower in current sector
 │  update_pos_v2() every 20ms
 │  TCS34725 poll every 50ms (via TCA9548A)
 │  at_outer_wall()          → reverse + shift to next strip
 │  at_sector_boundary()     → reverse + shift to next strip
 │  in_water_range(centre)   → POND_SAMPLE  (if !already_sampled)
 │  in_soil_disc(centre)     → APPROACH_SOIL (if !already_sampled)
 │  6 strips complete + terrain_complete → TRANSIT
 │  6 strips complete + !complete → halve strip width, re-sweep remainder
 │  check_stall() → stall >3s → RECOVERY
 ↓
POND_SAMPLE
 │  stop_motors()
 │  already_sampled(pos.x, pos.y)? → POND_REROUTE
 │  arm_water_down() → delay(1500) → read_turbidity()
 │  arm_neutral() → record_sample(sector, WATER, val)
 │  post_json_sample() · save_json_sample()
 ↓
POND_REROUTE
 │  reroute_around_pond()
 │  update_pos_v2() throughout circumnavigation
 │  cleared → turn_left → → SWEEP (resume)
 ↓
APPROACH_SOIL
 │  forward until arm pivot aligned over disc (arm_length offset)
 │  stop_motors()
 ↓
MEASURE_SOIL
 │  already_sampled(pos.x, pos.y)? → SWEEP
 │  arm_soil_down() → delay(1500) → read_moisture()
 │  arm_neutral() → record_sample(sector, SOIL, val)
 │  post_json_sample() · save_json_sample()
 ↓
SWEEP (resume)
 │  terrain_complete(sector)? → TRANSIT
 ↓
TRANSIT
 │  drive to centre (0,0) using encoder odometry
 │  re-sync pos=(0,0) · re-sync heading from MPU6050 absolute yaw
 │  sector++ · reset strip counter · recalibrate_for_sector()
 │  sector == 4 → DONE
 ↓
RECOVERY  ← *** NEW: handles official intervention ***
 │  Triggered by: stall >3s (encoders idle while in SWEEP)
 │                OR UDP command "RECOVER" from RC phone
 │  Officials have placed robot at arena centre
 │  re-sync pos=(0,0) · re-sync heading from MPU6050
 │  re-enter SWEEP from last known sector index
 │  already_sampled() prevents re-measurement of visited zones
 │  LED: solid purple during RECOVERY, green when resume
 ↓
DONE
   stop_motors() · arm_neutral()
   dump_json_serial() · write SPIFFS final
   LED: solid green
   HTTP GET /results returns results.json
```

---

## 11. Zone Coverage Guarantee

```
Strip width (3-sensor array): 100mm
Water slot half-diagonal: ~40mm  → STRIP ≤ 2×40 + 60 = 140mm ✓
Soil disc radius:          ~30mm  → STRIP ≤ 2×30 + 60 = 120mm ✓

6 strips × 100mm = 600mm (80% of 750mm sector radius)
If zones missed after 6 strips → halve to 50mm, sweep remaining 150mm
Re-sweep adds ~25s per sector — still within 8-min budget
```

---

## 12. No-Duplicate Guarantee

```cpp
bool terrain_complete(int sector) {
    int water = 0, soil = 0;
    for (int i = 0; i < sample_count; i++) {
        if (in_sector(past_samples[i].x, past_samples[i].y, sector)) {
            if (past_samples[i].type == WATER) water++;
            if (past_samples[i].type == SOIL)  soil++;
        }
    }
    return (water >= 1 && soil >= 2);
}

// already_sampled() checked before EVERY deployment
// EXCL_RADIUS = 120mm — larger than any zone, absorbs drift
// Two nearby soil patches in same sector: each records its own (x,y)
// Only same-position revisit is blocked, not nearby-but-different patches
```

---

## 13. Measurement Code (Rev 5)

### Servo arm

```cpp
#include <ESP32Servo.h>
Servo arm;
void arm_neutral()      { arm.write(90);  delay(500); }
void arm_water_down()   { arm.write(0);   delay(600); }   // CCW — SEN0189 down
void arm_soil_down()    { arm.write(180); delay(600); }   // CW — soil sensor down
```

### SEN0189 turbidity (voltage divider ×1.5 correction)

```cpp
float read_turbidity() {
    float v = (analogRead(TURB_PIN) / 4095.0f) * 3.3f * 1.5f;
    return fmaxf(0.0f, -1120.4f*v*v + 5742.3f*v - 4352.9f);  // NTU
}
```

### Capacitive soil moisture

```cpp
#define SOIL_DRY  2800   // calibrate in dry air before competition
#define SOIL_WET   900   // calibrate submerged before competition

float read_moisture() {
    int raw = analogRead(SOIL_PIN);
    return fmaxf(0.0f, fminf(100.0f,
        (float)(SOIL_DRY - raw) / (SOIL_DRY - SOIL_WET) * 100.0f));
}
```

### Standardised sample function (all callers pass string value)

```cpp
struct Sample {
    float x, y;
    int   sector;
    ZoneType type;        // WATER or SOIL
    char  value[16];      // "42.3 NTU" or "61.2%"
    long  timestamp_ms;
};

Sample past_samples[12];
int    sample_count = 0;

// Always pass null-terminated string: "%.1f NTU" or "%.1f%%"
void record_sample(int sector, ZoneType type, const char* value) {
    Sample& s = past_samples[sample_count];
    s.x = pos.x; s.y = pos.y; s.sector = sector;
    s.type = type; s.timestamp_ms = millis();
    strncpy(s.value, value, sizeof(s.value) - 1);
    sample_count++;
}

void sample_water(int sector) {
    arm_water_down();  delay(1500);
    float ntu = read_turbidity();
    arm_neutral();
    char val[16]; snprintf(val, 16, "%.1f NTU", ntu);
    record_sample(sector, WATER, val);
    post_json_sample(sector, "WATER", ntu, 0.0f);
    save_json_sample(sector, "WATER", ntu, 0.0f);
}

void sample_soil(int sector) {
    arm_soil_down();  delay(1500);
    float pct = read_moisture();
    arm_neutral();
    char val[16]; snprintf(val, 16, "%.1f%%", pct);
    record_sample(sector, SOIL, val);
    post_json_sample(sector, "SOIL", 0.0f, pct);
    save_json_sample(sector, "SOIL", 0.0f, pct);
}
```

### RECOVERY stall detection

```cpp
void check_stall() {
    static int stall_ms = 0;
    if (state == SWEEP && enc_left == 0 && enc_right == 0) {
        stall_ms += 20;
        if (stall_ms > 3000) {
            stall_ms = 0;
            state = RECOVERY;  // officials likely picked up robot
        }
    } else {
        stall_ms = 0;
    }
}
```

---

## 14. Data Storage — JSON Output (Rev 5)

Results written incrementally to SPIFFS `/results.json`. Served via HTTP GET `/results` after run. Also downloadable by connecting to "RobotAP" WiFi and visiting `192.168.4.1/results` in a browser.

### JSON schema

```json
{
  "run_id": "envirobot_run",
  "total_duration_s": 387.4,
  "sectors_completed": 4,
  "samples": [
    {
      "id": 1,
      "sector": 1,
      "terrain": "sandpaper",
      "type": "WATER",
      "value_ntu": 42.3,
      "value_pct": null,
      "x_mm": 320,
      "y_mm": 280,
      "t_s": 45.2
    },
    {
      "id": 2,
      "sector": 1,
      "terrain": "sandpaper",
      "type": "SOIL",
      "value_ntu": null,
      "value_pct": 23.1,
      "x_mm": 410,
      "y_mm": 195,
      "t_s": 82.7
    }
  ],
  "path": [
    { "x_mm": 0, "y_mm": 0, "t_s": 0.0 },
    { "x_mm": 48, "y_mm": 9, "t_s": 0.5 }
  ],
  "summary": {
    "total_samples": 12,
    "water_samples": 4,
    "soil_samples": 8,
    "run_success": true
  }
}
```

### SPIFFS write (ArduinoJson)

```cpp
#include <ArduinoJson.h>
#include <SPIFFS.h>

void save_json_sample(int sector, const char* type, float ntu, float pct) {
    File f = SPIFFS.open("/results.json", "r");
    DynamicJsonDocument doc(16384);
    if (f) { deserializeJson(doc, f); f.close(); }
    JsonArray samples = doc["samples"].as<JsonArray>();
    JsonObject s = samples.createNestedObject();
    s["id"]        = (int)samples.size();
    s["sector"]    = sector;
    s["terrain"]   = terrain_name(sector);
    s["type"]      = type;
    s["value_ntu"] = (strcmp(type,"WATER")==0) ? ntu : (float)NULL;
    s["value_pct"] = (strcmp(type,"SOIL")==0)  ? pct : (float)NULL;
    s["x_mm"]      = (int)pos.x;
    s["y_mm"]      = (int)pos.y;
    s["t_s"]       = millis() / 1000.0f;
    File fw = SPIFFS.open("/results.json", "w");
    serializeJson(doc, fw);
    fw.close();
}
```

---

## 15. Auto-Calibration at Startup

| Step | Duration | What happens |
|---|---|---|
| Power on | 0s | Init Wire0/Wire1, WiFi AP, MPU6050 DMP, encoder ISRs, servo 90° |
| Phase 1 | 0–3s | Read terrain floor 30× at centre → average → terrain_baseline |
| Phase 2 | 3–4s | Seed water_ref (blue-dominant) and soil_ref (red-dominant) hue |
| Phase 3 | 4–5s | Verify baselines separated ≥ 3× threshold; abort + re-seat if not |
| Ready | 5s | Green LED · WiFi AP active · web UI at 192.168.4.1 |

Between sectors: `recalibrate_for_sector()` reads 10 samples of new terrain floor.

**LED status ring (NeoPixel):**

| Colour | Meaning |
|---|---|
| Blue | Calibrating — do not move |
| Green | Ready / DONE |
| Yellow | Zone detected |
| White | Measuring |
| Red | Wall detected / rerouting |
| Purple | RECOVERY mode |

---

## 16. Communications — WiFi AP Mode

```cpp
WiFi.softAP("RobotAP", "password123");
// All devices connect directly — no router, no internet
// Robot IP: 192.168.4.1
```

| Channel | Protocol | Endpoint | Use |
|---|---|---|---|
| RC commands | WebSocket | ws://192.168.4.1:81 | Joystick JSON, mode commands (~10ms) |
| Sample log | HTTP POST | :5000/sample | JSON per measurement during run |
| Results download | HTTP GET | /results | Full results.json after run |
| Mobile RC web UI | HTTP GET | /rc | Virtual joystick touch page |
| Status | HTTP GET | /status | State, sector, sample count |

---

## 17. Remote Control — Mobile Touch Web UI

Phone connects to "RobotAP" → opens `192.168.4.1/rc` in browser.

### Interface elements

| Element | Function |
|---|---|
| Virtual joystick (nipple.js) | Proportional drive — x = steer, y = throttle |
| AUTO button | Toggle autonomous sweep mode |
| RECOVER button | Trigger RECOVERY state after official intervention |
| E-STOP button | Immediate motor cut (hardware safety) |
| Live reading display | Last NTU and moisture % |
| Sector indicator | Current sector number and terrain name |
| Sample count | e.g. "7 / 12 zones sampled" |

### Joystick → motor translation

```cpp
// WebSocket receives: {"x": -0.7, "y": 0.5}  (normalised -1 to 1)
void handle_joystick(float jx, float jy) {
    float left  = fmaxf(-1, fminf(1, jy + jx));
    float right = fmaxf(-1, fminf(1, jy - jx));
    set_motors((int)(left * 255), (int)(right * 255));
}

// UDP fallback for keyboard clients:
// Commands: "FWD" "BACK" "LEFT" "RIGHT" "STOP" "AUTO" "RECOVER"
```

### Semi-autonomous hybrid mode (recommended for competition)

1. Human navigates phone joystick to each sector — faster than full-auto
2. TCS34725 auto-detects zones when centred sensor reads zone colour
3. Robot auto-stops, auto-deploys arm, auto-records measurement
4. Phone screen shows: "WATER — 42.3 NTU ✓ Continue driving"
5. Human drives to next zone, repeats
6. No autonomous path-planning needed → eliminates navigation failures

---

## 18. Data Visualisation Web App (`visualizer.html`)

Single self-contained HTML file. No install, no server. Double-click to open in browser. Drag-drop `results.json` to load.

### Layout

```
┌────────────────────────────────────────────────────────────┐
│  HEADER: EnviroBot Results Visualiser · [Load JSON] · Rev  │
├─────────────────────────────────────┬──────────────────────┤
│                                     │  RUN SUMMARY         │
│   THREE.JS 3D ARENA                 │  ─────────────────   │
│   (orbit / zoom / click zones)      │  12 / 12 samples     │
│                                     │  387s run time       │
│   - Octagon dark timber walls       │  4 sectors ✓         │
│   - 4 textured terrain sectors      │                      │
│   - Blue diamond water slots        │  SECTOR BREAKDOWN    │
│   - Red-brown soil discs            │  S1 Sandpaper: ✓✓✓  │
│   - Glowing robot path trace        │  S2 Grass:     ✓✓✓  │
│   - Floating reading labels         │  S3 Wet Soil:  ✓✓✓  │
│   - Click zone → popup details      │  S4 Sand:      ✓✓✓  │
│                                     │                      │
├─────────────────────────────────────┴──────────────────────┤
│  [◀────────────── TIMELINE SCRUBBER ──────────────────▶]   │
├────────────────────────┬───────────────────────────────────┤
│  WATER TURBIDITY (NTU) │  SOIL MOISTURE (%)                │
│  [grouped bar chart]   │  [grouped bar chart]              │
│  S1 | S2 | S3 | S4    │  S1a S1b | S2a S2b | S3a ...     │
└────────────────────────┴───────────────────────────────────┘
```

### Three.js scene construction plan

```javascript
// CDN imports (no build step)
// <script src="https://cdn.jsdelivr.net/npm/three@0.165.0/build/three.min.js">
// <script src="https://cdn.jsdelivr.net/npm/three@0.165.0/examples/js/controls/OrbitControls.js">

// Octagonal arena walls
const wallShape = new THREE.Shape();
const R = 750, sides = 8;
for (let i = 0; i < sides; i++) {
    const angle = (i / sides) * Math.PI * 2 + Math.PI / 8;
    i === 0
        ? wallShape.moveTo(R * Math.cos(angle), R * Math.sin(angle))
        : wallShape.lineTo(R * Math.cos(angle), R * Math.sin(angle));
}
wallShape.closePath();
// ExtrudeGeometry with depth 80mm → 3D walls

// 4 terrain quadrants (750×750mm PlaneGeometry per quadrant)
// Material colours: sandpaper #d4c9a8, wet soil #5a3a1a, grass #4a7c4e, sand #e8d5a3

// Water slots: BoxGeometry(60,10,60) rotated 45° around Y-axis, blue material, opacity 0.85
// Soil discs: CylinderGeometry(30,30,5,32), reddish-brown material

// Robot path: THREE.Line with BufferGeometry, cyan emissive material
// Playback: animate path point-by-point when scrubber moves

// Raycaster on click: detect water slot or soil disc mesh → show HTML popup overlay
// Popup: sector, type, value (NTU or %), position (x,y), timestamp
```

### Chart.js data panels

```javascript
// Turbidity bar chart
new Chart(ctx_ntu, {
    type: 'bar',
    data: {
        labels: water_samples.map(s => `S${s.sector} (${s.terrain})`),
        datasets: [{ label: 'Turbidity (NTU)', data: water_samples.map(s => s.value_ntu),
                     backgroundColor: '#29b6f6' }]
    },
    options: {
        plugins: { title: { display: true, text: 'Water Turbidity by Sector' } },
        scales: { y: { title: { display: true, text: 'NTU' }, min: 0 } }
    }
});

// Moisture bar chart (8 bars — 2 per sector)
new Chart(ctx_pct, {
    type: 'bar',
    data: {
        labels: soil_samples.map((s,i) => `S${s.sector}-${i%2===0?'A':'B'}`),
        datasets: [{ label: 'Soil Moisture (%)', data: soil_samples.map(s => s.value_pct),
                     backgroundColor: '#8b4513' }]
    },
    options: {
        plugins: { title: { display: true, text: 'Soil Moisture by Sector' } },
        scales: { y: { title: { display: true, text: 'Moisture (%)' }, min: 0, max: 100 } }
    }
});
```

### Features checklist (Cat 2 scoring)

- [x] Clearly labelled axes: NTU and %
- [x] Graph title on each chart
- [x] All 12 readings displayed
- [x] Available immediately post-run (drag-drop JSON)
- [x] Aids interpretation: 3D arena shows spatial context of readings
- [x] Comparison across sectors: side-by-side bars
- [x] Click-to-inspect: exact reading + position per zone
- [x] PNG export for report/slides

---

## 19. Scoring Strategy — Design Alignment

| Category | Points | How we score |
|---|---|---|
| **Cat 1: Technical Measurement** | 30 | Both mandatory sensors (SEN0189 + cap. moisture). Auto-deploy arm. Exclusion zones prevent re-sampling. Target: 12/12 measurements within ±10% of reference. |
| **Cat 2: Data Output** | 10 | results.json served at 192.168.4.1/results. Visualiser: labelled axes (NTU, %), title on each chart, all readings, available within 30s of run end. |
| **Cat 3: Build Quality** | 5 | L298N heat-sinked, wiring cable-tied, sensors on rigid mount bracket, kill switch labelled and accessible, all exposed connections insulated. |
| **Cat 4: Innovation** | 10 | Seesaw arm (1 servo replaces 2 actuators), 3× TCS centering array, encoder odometry + MPU6050 fusion, mobile touch RC, Three.js 3D visualiser. |
| **Cat 5: Aesthetics** | 5 | Team name + logo on chassis, colour-coded LED ring, clean cable management, colour-matched sensor mounts. |
| **Cat 6: Case Competition** | 40 | See Section 20 — highest-priority category. |

### 3-minute Day 1 innovation pitch outline

1. **Seesaw arm** (30s): "One servo deploys two sensors. Simpler than two linear actuators, half the failure modes."
2. **3× TCS array** (30s): "Three colour sensors let us self-centre over a zone and scan 2× wider strips."
3. **Encoder + IMU fusion** (30s): "Wheel encoders + gyro give us 5× better position tracking than timing alone."
4. **Mobile RC + auto-zone** (30s): "Phone joystick drives navigation. Robot auto-detects and auto-samples each zone."
5. **Live visualiser** (30s): "All 12 readings shown in a 3D arena map with Chart.js comparison panels."

---

## 20. Day 2 — Case Competition Strategy (40 pts)

### Format

- Problem scenario revealed at start of Closing Night
- 1 hour to prepare response using Day 1 sensor data
- 3-minute verbal presentation + Q&A panel

### Data we collect (Day 1)

| Zone | Type | Expected range |
|---|---|---|
| S1 Sandpaper | Turbidity | Low–medium (0–80 NTU) |
| S2 Grass | Turbidity | Low (0–40 NTU) |
| S3 Wet Soil | Turbidity | Medium–high (30–200 NTU) |
| S4 Sand | Turbidity | Variable (10–150 NTU) |
| S1 Sandpaper ×2 | Moisture | Low (5–20%) |
| S2 Grass ×2 | Moisture | Medium (20–50%) |
| S3 Wet Soil ×2 | Moisture | High (50–90%) |
| S4 Sand ×2 | Moisture | Low–medium (10–35%) |

### 1-hour preparation plan

| Time | Task | Who |
|---|---|---|
| 0–5 min | Download results.json, open visualiser, verify all 12 readings | Software member |
| 5–15 min | Read problem scenario, identify which readings are most relevant | All |
| 15–30 min | Build argument: data → environmental conclusion → recommendation | All |
| 30–45 min | Prepare 3-min verbal script + which visualiser slides to show | All |
| 45–60 min | Practice run, Q&A rehearsal, assign speaking segments | All |

### Argument structure template

```
Opening (30s):
"Our robot collected [N] environmental readings across four terrain types.
The data reveals [key finding]."

Data section per terrain (30s × 4):
"In [terrain], turbidity was [X] NTU — [interpretation: clean/moderate/polluted].
Soil moisture was [Y]% — [interpretation: dry/healthy/waterlogged]."

Recommendation (30s):
"Based on this data, we recommend [specific action] because [data-backed reason].
The greatest environmental concern is [terrain] due to [specific reading]."

Limitations + tradeoff (20s):
"Our data is a point-in-time snapshot. Limitations include [dead-reckoning drift,
single-run sample, controlled arena conditions]."

Closing (10s):
"We're confident in the sensor accuracy — both sensors were calibrated against
reference values before the run."
```

### How to use the visualiser during presentation

- Connect laptop to projector
- Open `visualiser.html` → load `results.json`
- Orbit 3D arena as you describe each sector — point to each terrain
- Click a zone pin → popup shows exact NTU or % reading
- Switch to bar charts panel for cross-sector comparison
- The visual contrast between clean (low NTU) and contaminated (high NTU) water zones is immediately obvious

---

## 21. Bill of Materials

### From kit (included)

| Item | Qty |
|---|---|
| ESP32 | 1 |
| Water Turbidity Sensor SEN0189 | 1 |
| Capacitive Soil Moisture Sensor v2.0 | 1 |
| JGA25-370 geared motor 12V 300RPM | 2 |
| L298N motor driver | 1 |
| MG996R servo | 1 |
| LM2596 buck converter | 1 |
| USB to Micro USB cable | 1 |

### Additional purchases (~$55–70 AUD estimated)

| Item | Qty | Est. cost |
|---|---|---|
| TCS34725 colour sensor breakout | 3 | $12 |
| TCA9548A I2C mux (8-channel) | 1 | $4 |
| VL53L0X ToF distance sensor | 1 | $5 |
| MPU6050 IMU module | 1 | $3 |
| Hall-effect encoder for JGA25-370 | 2 | $10 |
| AMS1117-3.3 voltage regulator | 2 | $2 |
| Kill switch (latching, 16mm, red) | 1 | $3 |
| NeoPixel RGB LED ring (12-LED) | 1 | $5 |
| 8× AA battery holder (with switch) | 1 | $3 |
| Dupont jumpers, PCB, resistors, heat-shrink | lot | $8 |
| **Total** | | **~$55 AUD** |

---

## 22. Pre-Competition Inspection Checklist

### Technical Inspection (Day 1 — before run slot)

- [ ] Length ≤ 400mm, Width ≤ 300mm, Height ≤ 400mm (in starting config with arm neutral)
- [ ] Weight ≤ 1.5kg (weigh with batteries installed)
- [ ] SEN0189 functional — dip in water → NTU reading visible on screen
- [ ] Capacitive moisture sensor functional — press to soil → % reading visible
- [ ] Kill switch cuts all power immediately (test this in front of inspector)
- [ ] No prohibited components fitted
- [ ] Battery holder secure, no exposed terminals, wiring insulated
- [ ] Data output demonstrated: at least one reading visible (serial monitor or web)

### Day 1 Morning (pre-run)

- [ ] Replace or charge 8× AA batteries (fresh cells = 12V → motors at full spec)
- [ ] Flash latest firmware to ESP32
- [ ] Power on → blue LED → green LED at 5s (calibration passes)
- [ ] Verify WiFi AP "RobotAP" visible on phone
- [ ] Open `192.168.4.1/rc` on phone → virtual joystick renders ✓
- [ ] Drive manually through one sector, verify zone detection LEDs (yellow on zone)
- [ ] Test water sensor: NTU reading appears in serial/web ✓
- [ ] Test soil sensor: moisture % reading appears ✓
- [ ] Verify SPIFFS clear: `/results.json` reset before run
- [ ] Confirm kill switch accessible and labelled

### Day 2 Morning (case competition prep)

- [ ] Download `results.json` from `192.168.4.1/results`
- [ ] Open `visualiser.html` on laptop — drag-drop `results.json`
- [ ] Verify all 12 sample pins appear in 3D arena ✓
- [ ] Verify both bar charts render with correct axis labels (NTU, %) ✓
- [ ] Identify 2–3 key data insights from the actual readings
- [ ] Assign speaking roles to team members
- [ ] Practice full 3-minute pitch + anticipate Q&A questions
- [ ] Confirm projector / HDMI connection works with laptop
