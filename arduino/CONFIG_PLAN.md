# Runtime Config + Calibration Wizards — Implementation Plan

Decided 2026-07-20 via grilling. Competition Testing Day: Thu 30 July (10 days).

**Goal (restated):** change tuning values without recompiling. Nothing more.

**Scope cut:** no cloud config. RobotAP + test page already delivers the goal.
`configFetchUrl` deleted from the design. `uploadUrl` stays (viz-runs, unrelated).

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
  int   waterClearMin     = 800;
  float waterRedRatioMax  = 0.40f;
  float soilRedRatioMin   = 0.45f;
  float soilBlueRatioMax  = 0.20f;

  // ── Tunable by hand ─────────────────────────────────────────────
  int      driveSpeed        = 255;
  int      turnSpeed         = 255;
  int      wallStopMm        = 120;
  uint32_t sampleSettleMs    = 2000;
  uint32_t sampleWindowMs    = 3000;
  int      servoNeutral      = 90;
  int      servoWater        = 0;
  int      servoSoil         = 180;
  float    headingDeadbandRad = 0.20f;
};
extern RobotConfiguration config;
```
18 editable fields.

### Stays `#define` in `config.h`
WiFi creds, `UPLOAD_URL`, `POS_UPDATE_MS`, `SAMPLE_TICK_MS`, `SENSOR_SAMPLES`,
`RUN_TIME_LIMIT_MS`, `SECTOR_MAP`, `ENABLE_AUTONOMOUS`, and all autonomous-only
knobs (`SAMPLE_LOCKOUT_MS`, `POND_BACKOFF_MS`, `SAMPLE_CREEP_*`,
`SWEEP_LEG_TIMEOUT_MS`, `SAMPLE_TIME_BUDGET_MS`) — `ENABLE_AUTONOMOUS` is 0,
these are dead weight in the UI.

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
| `wallStopMm` | 30–2000 | below 30 the robot hits the wall |
| `sampleSettleMs` | 200–10000 | 8-min budget |
| `sampleWindowMs` | 500–10000 | ditto |
| `headingDeadbandRad` | 0.05–1.5 | below the per-loop turn step → steering hunts |
| `waterClearMin` | 0–65535 | uint16 channel |
| ratio fields | 0.0–1.0 | ratios |

Out-of-range on POST → 400 with the offending field named.
Out-of-range on load from file → clamp silently, log to serial.

---

## 3. Persistence

`/config.json` on SPIFFS, ArduinoJson (both already in the project).

- **Load:** struct constructed with defaults → `deserializeJson` → for each key
  present, overwrite. Missing keys keep defaults. No version field, no migration.
- **Save:** serialize all 18 → write. One file write per Save click.
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

**Colour / zone** (autonomous only — greyed out with a note while `ENABLE_AUTONOMOUS == 0`)
1. "Park the colour sensor over a water zone." → Capture
2. "Park over a soil disc." → Capture
3. "Park over bare arena floor." → Capture
4. Auto-derive `waterClearMin` / `waterRedRatioMax` / `soilRedRatioMin` / `soilBlueRatioMax`
   at the midpoint between each target cluster and the floor cluster.
5. **Refuse to save if clusters overlap** — show "water and floor are not distinguishable
   by this sensor at this height/lighting" rather than writing thresholds that can't work.

### Section: Config editor
18 fields, grouped Calibration / Tuning. Each shows current value, unit, and
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

`POST /cal/capture` with `{"target":"soil_dry"|"soil_wet"|"turb_zero"|"color_water"|"color_soil"|"color_floor"}`

Runs the **same** settle → window → median path a real sample uses
(`SAMPLE_SETTLE_MS` → `SAMPLE_WINDOW_MS` at `SAMPLE_TICK_MS`, `SENSOR_SAMPLES`
ADC averages per tick), returns the raw pre-scaling value. Does not write config —
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
| Colour wizard | 1.5h |
| Hardware verification | 2h |
| **Total** | **~12h** |

Colour wizard is the only one deferrable — `ENABLE_AUTONOMOUS` is 0.
Cut it first if time is short. ~10.5h without it.
