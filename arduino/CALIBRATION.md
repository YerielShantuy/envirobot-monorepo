# EnviroBot — Full Calibration Guide

Run these **in order** — later steps depend on earlier ones. Every result lands in
[`envirobot/config.h`](envirobot/config.h); reflash the main sketch once at the end
of each session, not after every number.

| # | What | Tool | Config keys | When |
|---|---|---|---|---|
| 1 | Soil moisture endpoints | `tests/test_soil` | `SOIL_DRY_VAL`, `SOIL_WET_VAL` | Bench, any time |
| 2 | Turbidity sanity check | `tests/test_turbidity` | `TURBIDITY_DIVIDER_RATIO` (verify) | Bench, before Testing Day |
| 3 | Zone colour thresholds | `tests/test_tcs34725` | `WATER_CLEAR_MIN`, `WATER_RED_RATIO_MAX`, `SOIL_RED_RATIO_MIN`, `SOIL_BLUE_RATIO_MAX` | **On the real arena** (venue lighting) |
| 4 | Drive speed | Main sketch → RC page → CALIBRATION panel | `MM_PER_SEC_AT_DRIVE` | Arena-like surface, charged battery |
| 5 | Wheel base (turn rate) | Same panel | `WHEEL_BASE_MM` | After step 4 — formula needs the speed |

---

## 1. Soil moisture endpoints

1. Flash `tests/test_soil/test_soil.ino`. Open Serial Monitor @115200.
2. Probe in **dry air** (or bone-dry soil), wait for the raw value to flatten → that raw is `SOIL_DRY_VAL`.
3. Probe fully in **a cup of water** (up to the marked line, never the electronics) → that raw is `SOIL_WET_VAL`.
4. Copy both into `config.h`. Sanity: dry raw must be **higher** than wet raw (capacitive probes fall when wet).

## 2. Turbidity sanity check

1. Flash `tests/test_turbidity/test_turbidity.ino`. Serial Monitor @115200.
2. Dunk the SEN0189 in **clear water**: `Vsensor` should read ~4.1–4.3V and NTU near 0.
   - `Vsensor` way off (e.g. reads half of expected) → your hardware divider ratio isn't the configured `TURBIDITY_DIVIDER_RATIO`. Recompute `(R_top + R_bottom) / R_bottom` from the actual resistors and fix `config.h`.
3. Test a **murky sample** (stir some soil in): NTU should rise clearly.
4. The NTU curve is only trusted ~2.5–4.2V. If competition scoring needs absolute accuracy, note readings against a reference sample on Testing Day and offset in analysis — don't bend the curve blind.

## 3. Zone colour thresholds — on site

Venue lighting changes everything; do this on the real arena on Testing Day.

> **Note**: the main sketch now runs the TCS at **24ms** integration (faster
> pond reaction), but `test_tcs34725` uses 50ms — raw counts there are ~2×
> the main sketch's. Either change the test sketch to
> `TCS34725_INTEGRATIONTIME_24MS` before calibrating, or halve the `C`
> values you record.

1. Flash `tests/test_tcs34725/test_tcs34725.ino`. Serial Monitor @115200.
2. Hold the sensor at **mounted height** over each surface and note `C`, `r/c`, `b/c` for: blue water slot, soil disc, sand, grass, sandpaper.
3. Pick thresholds with margin between classes:
   - `WATER_CLEAR_MIN` — below the water slot's `C`, above every other surface's `C`
   - `WATER_RED_RATIO_MAX` — above water's `r/c`, below everything else's
   - `SOIL_RED_RATIO_MIN` / `SOIL_BLUE_RATIO_MAX` — bracket the soil disc vs sand (closest impostor)
4. Update `config.h`, reflash main sketch, verify live: the test prints a `WATER`/`SOIL` verdict per line.

## 4. Drive speed → `MM_PER_SEC_AT_DRIVE`

Setup: main `envirobot` sketch flashed, robot **on battery, untethered**, on an
arena-like surface, battery at race-day charge (open-loop speed drops as voltage sags).

1. Phone → RC page → expand **CALIBRATION**.
2. Put a tape mark on the floor at the robot's front edge.
3. Tap **▶ Drive** (2000 ms default). Robot drives exactly 2s and auto-stops.
4. Tape-measure start mark → front edge, in **mm**.
5. Repeat 3× and average the distances.
6. Type the average into the `measured mm` field → panel prints a ready-to-paste
   `#define MM_PER_SEC_AT_DRIVE` line for `config.h`.

## 5. Wheel base → `WHEEL_BASE_MM`

Do step 4 first — the formula uses the calibrated speed (leave the numbers in the fields).

1. Tape an arrow on top of the robot, note its start direction against a floor mark.
2. Tap **↻ Spin** (4000 ms default). Robot spins in place and auto-stops.
3. Total rotation in degrees = full turns × 360 + final offset (e.g. 2 turns + 90° = 810).
4. Repeat 3×, average, type into `total deg` → panel prints the `#define WHEEL_BASE_MM` line.
5. This is the *effective* wheel base (includes spin friction) — expect it slightly larger than the ruler-measured track width. Use the calibrated number.

## Verify the pair

Reflash with the new constants, then drive a ~1m square via RC while watching the
**x/y/h readout** at the top of the RC page. Back at the start, x/y should be within
roughly ±150mm and heading near 0°. Dead reckoning is open-loop — drift is normal and
grows with jerky driving; the path log is decoration, never navigation.

## 6. Probe creep offset (autonomous only)

The TCS detects a zone at its own footprint; the probe tip touches down
some distance behind it. If that offset is meaningful, the autonomous robot
samples short of the target.

1. On the bench, measure the horizontal distance (mm) from the TCS's view
   point on the floor to where the water-probe tip lands at servo 0°.
2. `SAMPLE_CREEP_MS = offset_mm / (MM_PER_SEC_AT_DRIVE × SAMPLE_CREEP_SPEED / DRIVE_SPEED) × 1000`
   — e.g. 40mm offset at defaults (150 × 120/180 = 100mm/s) → 400ms.
3. Set it in `config.h` (default 0 = off). **Verify on the arena**: creep must
   never carry the wheels to the slot edge — recheck the wheel stop margin
   from PLAN.md Phase 1 with creep enabled.

## 7. Autonomous start pose

Place the robot at the **arena centre**, note which way it faces — that's
+x, and quadrant→sector numbering follows `SECTOR_MAP` in `config.h`
(confirm against the real sector layout on Testing Day). Start the run with
the RC page **▶ Start Auto** button or a BOOT press. If officials re-centre
a stuck robot mid-run, immediately send **⌖ Re-zero pose** (calibration
panel) with the eyeballed heading in degrees.

## Caveats

- **Battery**: recalibrate speed after swapping or heavily draining the battery. A 10% voltage drop is a visible speed drop.
- **Surface**: calibrate on the surface you'll drive on — carpet vs board changes both constants.
- **RC page constants**: the panel's wheel-base math mirrors `DRIVE_SPEED`/`TURN_SPEED` from `config.h`. If you change those, update the two constants at the top of the calibration JS in `rc_page/index.html` and regenerate `rc_page.h` (one-liner in `PLAN.md`).
