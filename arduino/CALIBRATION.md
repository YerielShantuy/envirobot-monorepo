# EnviroBot — Full Calibration Guide

Run these **in order** — later steps depend on earlier ones. Every result lands in
the **runtime config** (SPIFFS `/config.json`), edited from the diagnostics page at
`http://192.168.4.1/test` → **CALIBRATION WIZARDS**. No reflash: a wizard fills the
config editor, you hit **Save**, the value persists across power cycles.

Join the robot's own network first: WiFi **RobotAP** / `enviro123` → `http://192.168.4.1`.
There is no hotspot and no internet — the robot *is* the network.

| # | What | Tool | Config keys | When |
|---|---|---|---|---|
| 1 | Soil moisture endpoints | Diag page → **Soil probe** wizard (or `tests/test_soil`) | `soilDryVal`, `soilWetVal` | Bench, any time |
| 2 | Turbidity zero + divider check | Diag page → **Turbidity** wizard (or `tests/test_turbidity`) | `turbidityZeroRaw` (+ verify `TURBIDITY_DIVIDER_RATIO`) | Bench, before Testing Day |
| 3 | Drive speed | Diag page → **Motion** wizard | `mmPerSecAtDrive` | Arena-like surface, charged battery |
| 4 | Wheel base (turn rate) | Same wizard | `wheelBaseMm` | After step 3 — the formula needs the speed |
| 5 | Collision stop distance | Diag page → **Collision distance** wizard (or `tests/test_ultrasonic`) | `collisionGuardOn`, `obstacleStopMm` | **On the real arena**, after step 3 |

Config is **locked while a run is live** — hit **🏁 End Run** on the RC page before
calibrating, or every wizard returns HTTP 409.

---

## 1. Soil moisture endpoints → `soilDryVal` / `soilWetVal`

1. Diag page → **Soil probe →**.
2. Step 1: probe in **dry air** (or bone-dry soil), wipe it if damp. Capture — 5s settle + median window.
3. Step 2: probe **in a cup of water**, up to the marked line only, never the electronics. Capture.
4. The wizard rejects a dry/wet span under 200 counts — that means a faulty probe or a step
   that wasn't actually wet/dry. Retry rather than saving a bad span.
5. Sanity: dry raw must be **higher** than wet raw (capacitive probes fall when wet).

## 2. Turbidity → `turbidityZeroRaw`

The hardware divider is fixed: SEN0189 signal → **220 Ω** → tap → **470 Ω** → GND, ADC on
the tap. `TURBIDITY_DIVIDER_RATIO = 690/470 = 1.4681` in `hardware_pins.h` — a compile-time
constant, because changing a number cannot change a resistor. Only touch it if you rewire.

1. Flash `tests/test_turbidity/test_turbidity.ino`, Serial Monitor @115200.
2. Dunk the SEN0189 in **clear water**: `Vsensor` should read ~4.1–4.3 V and NTU near 0.
   - If `Vsensor` is way off, the resistors aren't 220/470 — measure them and recompute
     `(R_top + R_bottom) / R_bottom`, then fix `TURBIDITY_DIVIDER_RATIO` and reflash.
   - `Vadc` above 3.3 V means the divider is wrong or absent. **Stop** — you are over the
     ADC limit.
3. Test a **murky sample** (stir in some soil): NTU should rise clearly.
4. Now the wizard: Diag page → **Turbidity →**, probe in clear tap water, no bubbles on the
   optical window. Capture → Save. This offsets the curve so clear water reads 0 NTU.
5. The DFRobot NTU curve is only trusted ~2.5–4.2 V. If scoring needs absolute accuracy,
   note readings against a reference sample on Testing Day and offset in analysis — don't
   bend the curve blind.

## 3. Drive speed → `mmPerSecAtDrive`

Setup: robot **on battery, untethered**, on an arena-like surface, battery at race-day
charge (open-loop speed drops as voltage sags).

1. Diag page → **Motion →**. Clear 2 m ahead, mark the robot's front edge on the floor.
2. Step 1 **Run**: robot drives 3000 ms at `driveSpeed` and auto-stops.
3. Step 2: tape-measure start mark → front edge in **mm**, type it in.
4. Repeat the whole wizard 3× and use the average distance for a stable number.

## 4. Wheel base → `wheelBaseMm`

Continues in the same wizard — the formula uses the speed from step 3.

1. Step 3 **Run**: robot spins in place for 2000 ms at `turnSpeed`.
2. Step 4: total rotation in degrees = full turns × 360 + final offset (e.g. 2 turns + 90° = 810).
3. **Save to editor** → hit **Save**. Use **Spin again to verify** before saving if unsure.
4. This is the *effective* wheel base (includes spin friction) — expect it slightly larger
   than the ruler-measured track width. Use the calibrated number.

### Verify the pair

Drive a ~1 m square via RC while watching the **x/y/h** readout at the top of the RC page.
Back at the start, x/y should be within roughly ±150 mm and heading near 0°. Dead reckoning
is open-loop with no encoders and no IMU — drift is normal and grows with jerky driving.
The path log is decoration, never navigation.

## 5. Collision stop distance → `obstacleStopMm`

This is the safety number. The firmware pings the HC-SR04 every 100 ms; after **2
consecutive** reads closer than `obstacleStopMm` it brakes, stops, and refuses `FWD`
(HTTP 409) until a ping reads **40 mm further** than the threshold. Reverse and both turns
are never blocked — you always drive out of a block backwards.

**The guard is optional.** `collisionGuardOn` (checkbox in the config editor, default
**on**) switches the intervention off when the sensor is in your way — creeping up to a
wall, sampling hard against an edge, or a surface the ultrasonic misreads. With it off the
distance is still measured and still shown on the RC page (amber `⚠ guard off`), so you can
calibrate and watch the number without the motors being cut. Turning it back on resets the
debounce, so it never trips on a stale reading from before the switch.

Set it **larger than the robot's stopping distance**, or the robot brakes *into* the wall
it just saw.

1. Measure the stopping distance first: on the arena, drive forward at full `driveSpeed`,
   hit **■ STOP**, and measure the overshoot past where you released. Repeat 3×, take the worst.
2. Diag page → **Collision distance →**.
3. Step 1: park the robot facing an arena wall at the gap you want it to hold, capture the
   ultrasonic reading. "No echo" means the wall isn't square to the sensor, or the ECHO
   divider is wrong — fix that before continuing.
4. Step 2: type the stopping distance from step 1 of this section.
5. Step 3: the wizard adds them → **Save to editor** → **Save**.
6. Verify live: the RC page's top-right readout shows the live ping in mm and flips to a red
   **■ BLOCKED** when the guard trips. Walk a board toward the robot and watch it cut.

### Watch for

- **Angled walls and soft surfaces** reflect the ping away and read as "nothing in range".
  The guard is a backstop for the operator's eyes, not a substitute for them.
- **Water slots are recessed wheel traps and the ultrasonic cannot see down into one.**
  Nothing in the firmware protects the wheels from a slot — the operator must not drive
  over one. Sample, then reverse away.
- **The 8-minute limit is yours to enforce**: the RC page clock goes amber at 7:00 and red
  at 8:00. There is no firmware cutoff.

## Caveats

- **Battery**: recalibrate speed after swapping or heavily draining the battery. A 10%
  voltage drop is a visible speed drop, and it moves the stopping distance too.
- **Surface**: calibrate on the surface you'll drive on — carpet vs board changes both
  motion constants.
- **`driveSpeed` / `turnSpeed`**: the motion wizard's wheel-base maths reads these live from
  the robot. Change them and redo steps 3–5; the collision threshold depends on the
  stopping distance, which depends on `driveSpeed`.
