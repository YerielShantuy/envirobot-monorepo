# Runtime Config + Calibration Wizards — Implementation Plan

Decided 2026-07-20 via grilling. Competition Testing Day: Thu 30 July (10 days).

**Goal (restated):** change tuning values without recompiling. Nothing more.

**Scope cut:** no cloud config. RobotAP + test page already delivers the goal.
`configFetchUrl` deleted from the design — and `uploadUrl` went the same way when
WiFi became AP-only: the robot has no internet path at all now.

---

## Architecture

```
boot
 ├─ RobotConfiguration config;          // compiled-in defaults == today's #define values
 ├─ SPIFFS.begin()                       // already happens
 └─ loadConfig()                         // merge /config.json over defaults, clamp, apply
                                         // missing file or bad JSON → defaults, log warning, continue

test page (192.168.4.1/test over RobotAP)
 ├─ GET  /config          → { values..., runActive: bool }
 ├─ POST /config          → validate → clamp → apply RAM → write /config.json   [409 if runActive]
 ├─ POST /config/reset    → delete /config.json → revert to defaults            [409 if runActive]
 └─ POST /cal/capture     → { target } → run-path capture → { raw }             [409 if runActive]
```

Single source of truth: ESP32 SPIFFS. No network dependency at boot. No cloud.

---

## 1. `config.h` → split

### `hardware_pins.h` (new, all `#define`, never runtime)
Pins, plus values that describe physical hardware:
`PIN_*`, `ULTRASONIC_TIMEOUT_US`, `TURBIDITY_DIVIDER_RATIO`
(the divider ratio is set by physical resistors — changing it at runtime cannot change the hardware)

### `robot_config.h` (new)
```cpp
#pragma once
#include <Arduino.h>

struct RobotConfiguration {
  // ── Calibration outputs (wizard-owned) ──────────────────────────
  int   soilDryVal        = 2850;
  int   soilWetVal        = 1200;
  int   turbidityZeroRaw  = 0;      // NEW: clear-water ADC baseline, 0 = uncalibrated
  float mmPerSecAtDrive   = 150.0f;
  float wheelBaseMm       = 150.0f;
  bool  collisionGuardOn  = true;   // guard is optional — master switch
  int   obstacleStopMm    = 120;    // collision guard threshold

  // ── Tunable by hand ─────────────────────────────────────────────
  int      driveSpeed        = 255;
  int      turnSpeed         = 255;
  uint32_t sampleSettleMs    = 2000;
  uint32_t sampleWindowMs    = 3000;
  int      servoNeutral      = 90;
  int      servoWater        = 0;
  int      servoSoil         = 180;
};
extern RobotConfiguration config;
```

13 editable fields. The four colour-classification thresholds and
`headingDeadbandRad` went away with the colour sensor and the autonomous FSM;
`wallStopMm` became `obstacleStopMm` when the ultrasonic became a pure
collision guard.

### Stays `#define` in `config.h` / `hardware_pins.h`

AP creds, `POS_UPDATE_MS`, `SAMPLE_TICK_MS`, `SENSOR_SAMPLES`, `BRAKE_MS`,
the collision-guard timing (`OBSTACLE_PING_MS`, `OBSTACLE_TRIP_HITS`,
`OBSTACLE_HYST_MM`), and everything describing physical hardware — pins and
`TURBIDITY_DIVIDER_RATIO`. Changing a number at runtime cannot move a wire or
swap a resistor, so those must not be runtime-editable.

---

## 2. Clamping (mandatory — trust boundary)

Applied on every load and every POST, before the value reaches RAM:

| Field | Clamp | Reason |
|---|---|---|
| `servoNeutral/Water/Soil` | 0–180 | servo stall / gear damage |
| `driveSpeed`, `turnSpeed` | 0–255 | PWM range |
| `soilDryVal`, `soilWetVal` | 0–4095 | 12-bit ADC |
| `soilDryVal != soilWetVal` | reject POST | div-by-zero in moisture map |
| `turbidityZeroRaw` | 0–4095 | ADC |
| `mmPerSecAtDrive` | 10–1000 | absurd values wreck path[] |
| `wheelBaseMm` | 50–500 | ditto |
| `obstacleStopMm` | 30–2000 | below 30 the robot brakes into what it just saw |
| `sampleSettleMs` | 200–10000 | 8-min budget |
| `sampleWindowMs` | 500–10000 | ditto |

Out-of-range on POST → 400 with the offending field named.
Out-of-range on load from file → clamp silently, log to serial.

---

## 3. Persistence

`/config.json` on SPIFFS, ArduinoJson (both already in the project).

- **Load:** struct constructed with defaults → `deserializeJson` → for each key
  present, overwrite. Missing keys keep defaults. No version field, no migration.
- **Save:** serialize all 13 → write. One file write per Save click.
- **Reset:** `SPIFFS.remove("/config.json")` → re-init struct → apply.

---

## 4. Test page UI

Two new sections below the existing diagnostics.

### Section: Calibration wizards
Each wizard is a modal: numbered steps, one instruction per step, big
Ready/Capture button, 5s progress bar during capture, captured value shown
before advancing. Cancel at any step discards.

**Soil** (hand-held, servo stays 90°)
1. "Hold the soil probe in open air, completely dry. Wipe it if damp." → Capture → `soilDryVal`
2. "Submerge the probe in water **up to the marked line only** — never past the electronics." → Capture → `soilWetVal`
3. Show both + computed span. Reject if span < 200 counts ("probe may be faulty or not actually wet/dry").

**Turbidity** (hand-held, one point)
1. "Submerge the SEN0189 probe in clear tap water. No bubbles on the optical window." → Capture → `turbidityZeroRaw`
2. Slope stays the SEN0189 datasheet curve. NTU = datasheet_curve(raw) offset so clear water reads 0.

**Motion** (uses existing `CAL_FWD` / `CAL_SPIN`)
1. "Place robot on the arena surface with 2m clear ahead. Mark the starting point." → Run (`CAL_FWD` 3000ms)
2. "Measure the distance travelled." → user types mm → `mmPerSecAtDrive = mm / 3.0`
3. "Mark the robot's heading. Robot will spin." → Run (`CAL_SPIN` 2000ms)
4. "How many degrees did it actually turn?" → user types deg → solve `wheelBaseMm` from commanded vs observed
5. Offer "Spin again to verify" — repeat until estimated heading returns to start

**Collision distance**
1. "Park the robot facing an arena wall at the distance you want it to stop from." → Capture
2. "How far does it coast after a STOP at full drive speed?" → user types mm
3. `obstacleStopMm` = standoff + stopping distance — a threshold below the stopping
   distance means the robot brakes *into* the obstacle it just detected.
4. **Refuse to continue on "no echo"** — an angled or soft wall reflects the ping away,
   and a threshold captured from a phantom reading is worse than no guard.

### Section: Config editor

13 fields, grouped Calibration / Tuning. Each shows current value, unit, and
allowed range. Wizard-owned fields are editable too (wizards just fill them).

- Edits are local to the page. **Nothing applies until Save.**
- Dirty fields highlighted; Save button enabled only when dirty.
- `Save` → POST /config → on success, toast + clear dirty state.
- `Restore defaults` → confirm dialog → POST /config/reset.

### Run lockout
`GET /config` returns `runActive`. Test page polls it with the existing 2Hz
`/pos` poll. When true: all inputs disabled, wizards disabled, red banner
"Run in progress — config locked until END_RUN". Server enforces independently
with 409 (never trust the client).

Needs `bool isRunActive()` exported from `data_logger` — the flag `startRun()`
already implies.

---

## 5. Firmware capture endpoint

`POST /cal/capture` with `{"target":"soil_dry"|"soil_wet"|"turb_zero"|"obstacle"}`

The analogue targets run the **same** settle → window → median path a real sample uses
(`SAMPLE_SETTLE_MS` → `SAMPLE_WINDOW_MS` at `SAMPLE_TICK_MS`, `SENSOR_SAMPLES`
ADC averages per tick), returning the raw pre-scaling value. `obstacle` is a single
live ping instead — the HC-SR04 answers in milliseconds, so there is nothing to average
away. None of them write config —
the page collects captures and sends them in the normal Save.

Rationale: calibration endpoints must be measured by the function that measures
the run, or every reading carries a systematic offset.

---

## 6. Migration steps

1. Create `hardware_pins.h` + `robot_config.h`. Keep `config.h` as the remaining `#define`s, including both new headers.
2. Declare `RobotConfiguration config;` in `envirobot.ino`.
3. Swap the 18 macros to `config.field` across `sensors.cpp`, `servo_arm.cpp`, `navigation.cpp`, `position.cpp`.
4. Add `config_store.cpp/.h`: `loadConfig()`, `saveConfig()`, `resetConfig()`, `clampConfig()`.
5. Call `loadConfig()` in `setup()` after `SPIFFS.begin()`, before WiFi.
6. Add `isRunActive()` to `data_logger`.
7. Add the four routes to `wifi_server.cpp`.
8. Edit `test_page/index.html`; **regenerate `test_page.h`** (PROGMEM copy — the build step).
9. Verify on hardware.

---

## 7. Verification (before Testing Day)

- [ ] Boot with no `/config.json` → defaults load, serial logs "no config, using defaults"
- [ ] Save a value → power-cycle → value survives
- [ ] Hand-write invalid JSON to `/config.json` → boot uses defaults, does not hang
- [ ] POST `servoSoil: 400` → 400 response, servo never moves
- [ ] POST `soilDryVal == soilWetVal` → 400, no div-by-zero
- [ ] Start a run → config editor locks, POST returns 409
- [ ] Restore defaults → values match current `#define`s exactly
- [ ] Full soil wizard → resulting `soilDryVal`/`soilWetVal` match a manual serial read
- [ ] Motion wizard → drive 1m, verify `path[]` endpoint is within ~10%
- [ ] Serial `[SAMPLE]` lines unchanged in format (live_plot.py depends on it)

---

## 8. Effort

| Item | Est. |
|---|---|
| Header split + macro swap | 1h |
| `config_store.cpp` (load/save/clamp/reset) | 1.5h |
| 4 HTTP routes + `isRunActive()` | 1h |
| Config editor UI | 2h |
| Soil + turbidity wizards | 1.5h |
| Motion wizard | 1.5h |
| Collision-distance wizard | 1h |
| Hardware verification | 2h |
| **Total** | **~12h** |

The collision-distance wizard is the only deferrable one — `obstacleStopMm` can also
be set by hand in the config editor off a `tests/test_ultrasonic` reading.
