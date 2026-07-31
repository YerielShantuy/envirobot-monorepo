# EnviroBot — Project Instructions

## What This Is
ESP32 environmental survey robot + Next.js web platform for the EnviroBot Environmental Robotics Challenge.
- **Competition dates**: Testing Day Thu 30 July (robot run + inspection) · Closing Night Fri 31 July 5–9pm (case competition)
- **Mission**: 12 measurements (4 water turbidity + 8 soil moisture) in ≤8 minutes — 1 water zone + 2 soil zones per sector, on a 1500 mm square four-terrain arena
- **Scoring**: 100 pts across 6 categories — Closing Night presentation (40) is the biggest; always check rulebook before changing behaviour
- **Strategy**: Remote controlled, full stop — there is no autonomous mode in the firmware. The operator drives, tags each sample with the S1–S4 selector on the RC page, and enforces the 8-minute limit from the run clock.

## Hardware Truths (do not contradict)
- **No wheel encoders, no IMU.** Position AND heading are open-loop dead-reckoned from commanded wheel speeds (`position.cpp`), path log only — never for safety decisions.
- **SEN0189 needs a voltage divider** before the ESP32 ADC (outputs ~4.5V, pin max 3.3V): signal → 220 Ω → tap → 470 Ω → GND, ADC reads the tap (470 Ω nearer ground). `TURBIDITY_DIVIDER_RATIO` = 690/470 = 1.4681. Turbidity GPIO34, soil GPIO35 — both ADC1.
- **HC-SR04 is the collision guard, and the only obstacle sensor** — TRIG GPIO16, ECHO GPIO17; ECHO returns 5V so it needs a 5V→3.3V divider. Anything closer than `obstacleStopMm` cuts the motors and refuses forward drive; reverse and turns stay available so the operator can back out.
- **No I2C bus on the robot** — no colour sensor, no zone auto-detection; GPIO 21/22 are free.
- **WiFi is AP-only** — the robot IS the network: SSID `RobotAP`, password `enviro123`, at `http://192.168.4.1` (or `http://envirobot.local`). No STA mode, no internet, no upload of any kind.

## Repo Structure
```
Enviorbot/
├── PRD.md / PLAN.md / DESIGN.md / CHECKPOINT.md   → planning artifacts (repo root)
├── webapp/          → Next.js 15 + TypeScript + Tailwind 4 → Cloudflare Pages
│   ├── app/
│   │   ├── page.tsx              → Landing (GSAP ScrollTrigger, dark sci-fi)
│   │   ├── docs/[[...slug]]/     → MDX docs (sidebar + search)
│   │   ├── code/                 → Arduino code browser
│   │   ├── design/               → CAD viewer
│   │   ├── viz/                  → Visualiser — drop a results.json, replays 2D Canvas + 3D (r3f) + Chart.js. Client-side only
│   │   ├── admin/                → Content admin (Supabase auth)
│   │   └── api/                  → Edge routes (Supabase + R2 backed)
│   └── lib/                      → arena.ts, docs-content.ts, supabase.ts, r2.ts
├── arduino/
│   ├── envirobot/
│   │   ├── envirobot.ino         → Main sketch; brings up the WiFi AP + server, then serves RC commands
│   │   ├── hardware_pins.h       → Physical pins + divider ratio — compile-time, never runtime
│   │   ├── robot_config.h        → The 14 runtime-tunable fields (5 wizard-owned + 9 hand-tunable)
│   │   ├── config_store.cpp/.h   → Loads/clamps /config.json over those defaults at boot
│   │   ├── config.h              → The 10 constants that stayed compile-time (WiFi, tick/ping timings)
│   │   ├── sensors.cpp/.h        → Turbidity (divider) + soil + HC-SR04 raw reads
│   │   ├── collision.h           → Pure debounce/hysteresis guard logic — host-tested, no Arduino deps
│   │   ├── servo_arm.cpp/.h      → Double-arm servo (0°=water, 90°=neutral, 180°=soil)
│   │   ├── navigation.cpp/.h     → Motor layer only — drive/turn/brake + collision guard veto, no FSM
│   │   ├── position.cpp/.h       → open-loop dead reckoning from commanded speeds (NO encoders, NO IMU)
│   │   ├── data_logger.cpp/.h    → Frozen-schema JSON → SPIFFS + /data
│   │   ├── rc_page.h             → AUTO-GENERATED PROGMEM copy of rc_page/index.html
│   │   ├── data_page.h           → AUTO-GENERATED from data_page/index.html (the /view dashboard)
│   │   ├── test_page.h           → AUTO-GENERATED from test_page/index.html (the /test diag page)
│   │   └── wifi_server.cpp/.h    → AP-only HTTP server (POST /rc, GET /pos, /samples, /data, /config)
│   ├── rc_page/ data_page/ test_page/  → HTML sources for the three PROGMEM pages
│   └── tools/
│       ├── gen-pages.cjs         → Regenerates ALL THREE *_page.h — run after editing any page HTML
│       ├── live_plot.py          → The judged data output (see §4.6 below)
│       └── test_collision.cpp    → Host unit test for collision.h — g++ it, no board needed
└── resources/                    → Rulebook, arena spec, reference visualiser
```

## JSON Output Schema (must match exactly — visualiser depends on it)
```json
{
  "run_id": "run_001",
  "total_duration_s": 480,
  "sectors_completed": 4,
  "path": [{ "t_s": 0.0, "x_mm": 0, "y_mm": 0 }],
  "samples": [
    { "id": "W1", "type": "WATER", "sector": 1, "terrain": "Urban", "value_ntu": 42.3, "value_pct": null, "t_s": 45.2 },
    { "id": "S1", "type": "SOIL",  "sector": 1, "terrain": "Urban", "value_ntu": null, "value_pct": 68.5, "t_s": 78.1 }
  ]
}
```
`terrain` is one of `Urban` / `Field` / `Desert` / `Stone`, or `Unknown` for sector 0 — `TERRAINS[]` table in `data_logger.cpp`.

## Servo Arm Mechanism
Single servo shaft drives TWO arms simultaneously:
- **Servo 0°** → Arm A (SEN0189) DOWN into water zone | Arm B UP
- **Servo 90°** → NEUTRAL, both arms horizontal (travel position)
- **Servo 180°** → Arm B (Capacitive Soil) DOWN onto disc | Arm A UP

Never move servo while robot is moving. Always return to 90° after sampling.

## RC Commands (POST /rc)
```json
{ "cmd": "FWD"|"BWD"|"LEFT"|"RIGHT"|"STOP", "speed": 0-255 }   // speed optional, drive cmds only; FWD is refused with 409 while the collision guard is blocked
{ "cmd": "SAMPLE_WATER", "sector": 1-4 }   // arm 0° → settle 2s → 3s median window → 90°; ~6s total
{ "cmd": "SAMPLE_SOIL",  "sector": 1-4 }   // arm 180° → same settle+window → 90°
{ "cmd": "CAL_FWD",  "ms": 200-10000 }     // timed drive at driveSpeed, auto-stop; also 409 while blocked (calibration — see arduino/CALIBRATION.md)
{ "cmd": "CAL_SPIN", "ms": 200-10000 }     // timed in-place spin at turnSpeed, auto-stop
{ "cmd": "END_RUN" }                        // finalise + save results.json to SPIFFS (no upload — operator downloads it from the RC page)
{ "cmd": "REZERO", "h": 0-359 }             // re-zero dead-reckoned pose to origin at eyeballed heading (officials re-centre stuck robots — rulebook 5.2.1)
```
`GET /pos` → `{"x","y","h","t","d","blocked","guard"}` — live dead-reckoned pose, run seconds, guard distance, blocked flag, and whether the guard is even enabled (RC page polls at 2Hz, shows ⏱ amber at 7:00 / red at 8:00). `GET /samples` → samples-only JSON for `arduino/tools/live_plot.py`. `GET /data` → the saved `results.json`, which the ⬇ Download JSON button on the RC page pulls off the robot.

**Config + calibration endpoints**: `GET/POST /config` and `POST /config/reset` edit the 14 runtime fields in `robot_config.h` (persisted to `/config.json`, clamped on every load and every POST — never trust a raw value into RAM). `POST /cal/capture` runs the wizards; targets are `soil_dry`, `soil_wet`, `turb_zero`, `obstacle`. `obstacle` deliberately takes one live ping with no settle and no median. `/view` is the judge-facing live dashboard, `/test` the diagnostics page.

**Run clock**: `startRun()` (data_logger) resets the JSON doc + clock. Fires on the first RC drive/sample after boot or END_RUN. Outside a live run, path logging and `total_duration_s` are frozen — never let `t_s` count from power-on.

**Sector tagging**: every sample carries the sector the operator picked on the RC page's S1–S4 selector. Nothing on the robot infers sector or terrain. After an official re-centres a stuck robot, send `REZERO` with the eyeballed heading so the path log stays usable.

## Data Output Compliance (rulebook §4.6)
Approved formats: serial monitor, live plot on connected computer, printed log/CSV. The WiFi webapp is an **alternative method — needs prior written approval** from Project Directors (createunsw@gmail.com + unsw@studentenergy.org). Serial output of every sample is the always-legal fallback — never remove the `Serial.printf` sample lines.

**Primary judged output**: `arduino/tools/live_plot.py` — matplotlib live table + labelled bar charts (§4.6's own named example) + `samples.csv`. Serial mode (`--port COM5`) for inspection; URL mode (`--url http://192.168.4.1/samples`) over RobotAP during the run. Parses the `[SAMPLE] S<n> <TYPE> <value>` serial format — keep firmware serial lines in that exact shape.

## Deploy (Cloudflare Workers via OpenNext + Wrangler — strictly no Vercel)
```powershell
cd webapp
npm run deploy    # opennextjs-cloudflare build && opennextjs-cloudflare deploy (Wrangler under the hood)
```
- Migrated 2026-07-08 from Pages + `@cloudflare/next-on-pages` (deprecated for Next 15, unbuildable on Windows). Config: `wrangler.jsonc` + `open-next.config.ts`.
- Never use Vercel CLI, `vercel` commands, or deploy to Vercel
- Do NOT add `export const runtime = 'edge'` — OpenNext uses the Node runtime in workerd; edge exports break the build
- Secrets (`R2_*`, `SUPABASE_*`, admin password): `wrangler secret put <NAME>` per key; local dev uses `.dev.vars`
- After firmware changes: `node scripts/sync-code.mjs --push` — plain (no flag) only regenerates `seed-code/route.ts`; `--push` is what actually gets the code onto `/code`, straight to R2 + Supabase from `.env.local`. New firmware file? Add it to `SOURCES` first or it never ships.
- After docs-content changes: `node scripts/sync-docs.mjs` (mirrors `lib/docs-content.ts` → `content/docs/*.mdx`)
- **Never sync `/code` with `wrangler r2 object put`** — the Wrangler R2 CLI hits a different store than the app's S3 endpoint

## Critical Rules
1. **Rulebook is law** — never modify robot behaviour that affects scoring without checking `/resources/EnviroBot Rulebook.md`. Rulebook has internal contradictions in category points — email organisers before relying on a number.
2. **JSON schema is frozen** — webapp visualiser depends on exact field names
3. **One servo** — never add a second servo or linear actuator; constraint is by design
4. **Wheels never touch water** — water slots are recessed wheel traps; stop short, sample, reverse away, never drive over a slot
5. **8-minute hard limit** — operator-enforced from the RC page run clock; exceeding costs −5 pts
6. **Never hardcode sensor data** — pre-loaded readings zero out Categories 1 & 2 and risk disqualification
7. **Arena is a 1500 mm square** — four 750 mm quadrants, viewed from above with +y up: S1 Urban (top-left) · S2 Field (top-right) · S3 Desert (bottom-left) · S4 Stone (bottom-right)
8. **Edited a `*_page/index.html`? Run `node arduino/tools/gen-pages.cjs`** — the robot serves the generated PROGMEM `.h`, never the HTML. An unregenerated header means the fix simply is not on the robot.
9. **Runs are never uploaded** — the operator downloads `results.json` off the robot and drops it into `/viz`, which parses it client-side into `sessionStorage`. There is no upload endpoint on either side; don't reintroduce one.
