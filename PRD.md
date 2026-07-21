# EnviroBot — Product Requirements Document

**Version**: 1.1 | **Date**: 2026-07-07 | **Status**: Approved (audit revision)

> v1.1 corrects the scoring model against the rulebook, switches strategy to RC-first,
> removes wheel encoders (hardware doesn't have them), adds data-output compliance and
> pond-safety requirements, and brings webapp scope in line with what is built.

---

## 1. Overview

EnviroBot is a two-part system: an ESP32-based environmental survey robot and a Next.js web platform. The robot competes in the EnviroBot Environmental Robotics Challenge — navigating a ~1.5m octagonal arena, collecting water turbidity and soil moisture measurements from four terrain sectors, and outputting structured JSON data. The web platform presents the project, visualises run data, and serves as the competition showcase.

### 1.1 Competition Context
| Item | Value |
|---|---|
| Competition | EnviroBot — Environmental Robotics Challenge |
| Testing Day | Thu 30 July (W9) — inspection + measurement run + robot assessment |
| Closing Night | Fri 31 July, 5–9pm — case competition (scenario revealed on the night, 1h prep, 3-min presentation + Q&A) |
| Run time limit | 8 minutes (−5 pts if exceeded) |
| Measurements | 12 total: 4 water turbidity + 8 soil moisture (1 water + 2 soil per sector) |
| Arena | Octagonal, ~1.5m diameter, 4 terrain sectors, recessed water slots |
| Max score | 100 points across 6 judging categories |

---

## 2. Strategy

**RC-first.** The rulebook explicitly allows remote-controlled robots. Autonomy affects only the top band of Category 1 ("navigates independently") and some Innovation points. With no wheel encoders and 3 weeks to Testing Day:

1. **Tier 1 (must work)**: RC driving + manual sampling + sector tagging + compliant data output. This alone reaches the 18–22 band of Category 1 if sensors are accurate.
2. **Tier 2 (must work)**: Sensor calibration to ±10% of reference — highest-value engineering time in the project.
3. **Tier 3 (must prepare)**: Closing Night case competition — 40 pts, the single biggest category. Practice arguing from collected data.
4. **Tier 4 (stretch)**: Autonomous wander mode (dead-reckoned heading + ultrasonic wall avoidance + colour zone detection). More degraded without an IMU — demoed only if reliable.

---

## 3. Scoring Breakdown (100 pts, rulebook §6.1)

| # | Category | Points | What it requires |
|---|---|---|---|
| 1 | Technical Measurement Performance | 30 | Both sensors active, readings within ±10% of reference; top band requires independent navigation |
| 2 | Data Output & Visualisation | 10 | Legible output, labelled units (NTU / %), graph or log, available immediately after run |
| 3 | Build Quality & Workmanship | 5 | Structural integrity, neat wiring, mounting, manoeuvrability |
| 4 | Innovation & Engineering Design | 10 | Novel solutions (double-arm servo, auto-deploy probe), iteration evidence |
| 5 | Robot Aesthetics | 5 | Finish, branding, professional look |
| 6 | Closing Night Case Competition | 40 | Data-driven argument, tradeoff handling, Q&A, teamwork |

⚠️ **Rulebook is internally inconsistent** (Cat 1 heading says 40, table 30, bands cap at 25; Cat 6 heading 25, table 40; Cat 3 heading 10, table 5). The summary table totals 100 and is treated as authoritative. **Action item: email organisers (createunsw@gmail.com AND unsw@studentenergy.org) to confirm weightings.**

**Deductions**: prohibited components −10/DQ · over 8 min −5 · pre-loaded/hardcoded data zeroes Cat 1+2 (and risks DQ) · arena damage −5 per incident.

**Priority order**: Presentation (40) ≥ Measurement accuracy (30) > Data output (10) + Innovation (10) > Build + Aesthetics (10).

---

## 4. Robot Requirements

### 4.1 Physical Constraints (inspection, pass/fail)
| Constraint | Limit |
|---|---|
| Length | ≤ 400 mm |
| Width | ≤ 300 mm |
| Height (arm neutral 90°) | ≤ 400 mm |
| Weight | ≤ 1.5 kg |

Measured in starting configuration; deployable parts may not exceed limits. Inspection also verifies: both mandated sensors present + functional, battery safety, one demonstrated data reading, no prohibited components. Fail = 30 minutes to rectify or forfeit.

### 4.2 Required Sensors (mandated by rulebook — using others risks DQ)
- **Water Turbidity Sensor SEN0189** — NTU, deployed into water slot. **Outputs up to ~4.5V — must pass through a voltage divider before the ESP32 ADC (3.3V max).**
- **Capacitive Soil Moisture Sensor v2.0** — moisture %, pressed onto soil disc.

### 4.3 Servo Arm Mechanism (Single Servo, Two Arms)
One MG996R servo shaft carries two rigid arms:

```
Servo   0° → Arm A (SEN0189) DOWN into water | Arm B UP
Servo  90° → NEUTRAL, both arms horizontal (travel)
Servo 180° → Arm B (soil probe) DOWN on disc | Arm A UP
```

**Rationale**: One servo replaces two linear actuators — simpler, lighter, fewer failure points. This is also the headline Innovation & Design story.

### 4.4 Hardware Bill of Materials
| Component | Qty | Role |
|---|---|---|
| ESP32 DevKit v1 | 1 | Main MCU — WiFi AP, FSM, REST API |
| MG996R Servo | 1 | Double-arm sensor deployment |
| SEN0189 Turbidity Sensor | 1 | Water NTU (via voltage divider → GPIO34) |
| Capacitive Soil Moisture v2 | 1 | Soil moisture % (GPIO35) |
| TCS34725 Colour Sensor | 1 | Zone detection (I2C, 0x29) — **mounted ahead of front wheels** |
| HC-SR04 Ultrasonic Sensor | 1 | Wall detection (TRIG GPIO16 / ECHO GPIO17 via 5V→3.3V divider) |
| L298N Motor Driver | 1 | Drives both DC gearmotors (~2V drop — expect reduced speed on 7.4V) |
| JGA25-370 DC Gearmotor | 2 | Differential drive — **no encoders** |
| LM2596 Buck Converter | 1 | 7.4V → 5V for servo/sensors |
| AMS1117 3.3V Regulator | 1 | 3.3V rail for ESP32 |
| LiPo 7.4V 2200mAh | 1 | Main power |

Removed: hall encoders (hardware doesn't have them), TCA9548A mux, **MPU6050 IMU**, and **VL53L0X ToF** (2026-07-10 component revision). Wall detection is now a single HC-SR04 ultrasonic; TCS34725 is the sole I2C device. Trade-off: no IMU means heading is open-loop dead reckoning only (drifts).

### 4.5 Operation Modes

**Mode 1 — Remote Control (primary)**
- ESP32 starts as WiFi AP (`RobotAP`, `192.168.4.1`), serves RC page from PROGMEM
- D-pad drive + sector selector (S1–S4) + Sample Water / Sample Soil / End Run buttons
- `POST /rc`: `{"cmd":"FWD"|"BWD"|"LEFT"|"RIGHT"|"STOP","speed":0-255}` (speed optional)
- Sampling: `{"cmd":"SAMPLE_WATER","sector":1-4}` / `{"cmd":"SAMPLE_SOIL","sector":1-4}` — deploys arm, reads, logs with operator-tagged sector
- `{"cmd":"END_RUN"}` finalises and persists `results.json` to SPIFFS
- Every sample also printed to serial — the rulebook-approved output fallback

**Mode 2 — Autonomous FSM (stretch)**
- Wander pattern: dead-reckoned heading hold, ~135° bounce at walls (HC-SR04) and after samples
- Zone detection via TCS34725 (thresholds calibrated on real arena)
- Time-based re-sample lockout (no coordinates without encoders)
- Hard 8-minute cap → COMPLETE + finalise
- **Pond safety**: stop on water detection, sample, then REVERSE away — wheels never cross a recessed slot

### 4.6 FSM State Machine
```
INIT → SWEEP ←→ POND_SAMPLE → POND_BACKOFF (reverse + turn away)
             ←→ MEASURE_SOIL
             → COMPLETE (12 samples or 8 min elapsed; server stays up for /data)
```

Known limitation (accepted): with no IMU or encoders, heading and position drift over a run and are fully lost if officials reposition the robot. Path log is best-effort; samples and sectors (operator-tagged in RC) remain valid.

### 4.7 Data Output Compliance (rulebook §4.6)
Approved: serial monitor, live plot on connected computer, printed log/CSV.
- **Fallback (always legal)**: serial output of every sample with units.
- **Webapp visualiser = alternative method — requires prior written approval.** Action item: email Project Directors before Testing Day. If not approved, present via serial + laptop plot; webapp remains the Closing Night showcase.

### 4.8 JSON Output Schema (frozen)
```json
{
  "run_id": "run_001",
  "total_duration_s": 480,
  "sectors_completed": 4,
  "path": [{ "t_s": 0.0, "x_mm": 0, "y_mm": 0 }],
  "samples": [
    { "id": "W1", "type": "WATER", "sector": 1, "terrain": "Sandpaper", "value_ntu": 42.3, "value_pct": null, "t_s": 45.2 },
    { "id": "S1", "type": "SOIL",  "sector": 1, "terrain": "Sandpaper", "value_ntu": null, "value_pct": 68.5, "t_s": 78.1 }
  ]
}
```
Terrain names are rulebook "examples only" — confirm actual terrains on Testing Day and update `TERRAINS[]` in `data_logger.cpp`.

---

## 5. Web Platform Requirements

### 5.1 Pages (all built)
- **Landing (`/`)** — dark sci-fi showcase, GSAP ScrollTrigger, Higgsfield hero asset (pending)
- **Docs (`/docs/...`)** — MDX, sidebar, Cmd+K search
- **Code Browser (`/code`)** — Arduino source, syntax highlighted
- **Design (`/design`)** — CAD viewer
- **Visualiser (`/viz`)** — Canvas 2D arena + path replay + Chart.js (NTU bar, moisture bar, path XY) + JSON drag-drop
- **Admin (`/admin`)** — content management (auth via Supabase)

### 5.2 Technical Stack (as built)
| Layer | Choice |
|---|---|
| Framework | Next.js 15 (App Router) |
| Language | TypeScript |
| Styling | Tailwind CSS v4 |
| Animation | GSAP + ScrollTrigger (scroll); CSS transitions (UI) |
| Charts | Chart.js 4 |
| Storage | Supabase (content/auth) + Cloudflare R2 (assets) |
| Fonts | Syne (display) · IBM Plex Sans (body) · JetBrains Mono (code) |
| Deploy | Cloudflare Workers via `@opennextjs/cloudflare` + Wrangler (never Vercel) |

**Deploy note (2026-07-08)**: migrated from Pages + `@cloudflare/next-on-pages` (deprecated for Next 15, unbuildable on Windows) to the OpenNext adapter. `npm run deploy` builds and ships via Wrangler.

### 5.3 Cloudflare Compatibility Rules
- **No** `export const runtime = 'edge'` anywhere — OpenNext uses the Node runtime in workerd
- Static assets in `public/`; docs content baked in at build time
- `wrangler.jsonc` + `open-next.config.ts` at webapp root
- Secrets via `wrangler secret put` (`R2_*`, `SUPABASE_*`, admin password)

---

## 6. Out of Scope (v1)
- Real-time live tracking via WebSocket (replay only)
- Multi-robot support
- Mobile app (web is mobile-responsive instead)
- Machine learning on-device navigation
- Multi-team leaderboard
- Coordinate-accurate autonomous mapping (no encoders — accepted)
