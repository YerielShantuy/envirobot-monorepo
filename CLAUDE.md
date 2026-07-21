# EnviroBot — Project Instructions

## What This Is
ESP32 environmental survey robot + Next.js web platform for the EnviroBot Environmental Robotics Challenge.
- **Competition dates**: Testing Day Thu 30 July (robot run + inspection) · Closing Night Fri 31 July 5–9pm (case competition)
- **Mission**: 12 measurements (4 water turbidity + 8 soil moisture) in ≤8 minutes
- **Scoring**: 100 pts across 6 categories — Closing Night presentation (40) is the biggest; always check rulebook before changing behaviour
- **Strategy**: RC-first. Remote control is fully rulebook-legal; autonomy is a stretch goal for the top accuracy band + innovation points.

## Hardware Truths (do not contradict)
- **No wheel encoders, no IMU.** Position AND heading are open-loop dead-reckoned from commanded wheel speeds (`position.cpp`), path log only — never for safety decisions.
- **SEN0189 needs a voltage divider** before the ESP32 ADC (outputs ~4.5V, pin max 3.3V). Ratio in `config.h` `TURBIDITY_DIVIDER_RATIO`.
- **TCS34725 is the only I2C device** (0x29 on `Wire`, pins 21/22). Wall detection is an **HC-SR04 ultrasonic** — TRIG GPIO16, ECHO GPIO17; ECHO returns 5V so it needs a 5V→3.3V divider.
- **GPIO 21/22 are I2C only** — never assign to motors.
- **Colour sensor must be mounted ahead of the front wheels** — water slots are recessed; detection must fire before a wheel reaches the slot edge.

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
│   │   ├── viz/                  → Visualiser (Canvas arena + Chart.js + JSON upload)
│   │   ├── admin/                → Content admin (Supabase auth)
│   │   └── api/                  → Edge routes (Supabase + R2 backed)
│   └── lib/                      → arena.ts, docs-content.ts, supabase.ts, r2.ts
├── arduino/
│   ├── envirobot/
│   │   ├── envirobot.ino         → Main sketch; WiFi AP + server in BOTH modes
│   │   ├── config.h              → Pins, calibration knobs, thresholds
│   │   ├── sensors.cpp/.h        → Turbidity (divider) + soil + TCS34725 + HC-SR04 ultrasonic
│   │   ├── servo_arm.cpp/.h      → Double-arm servo (0°=water, 90°=neutral, 180°=soil)
│   │   ├── navigation.cpp/.h     → FSM: wander + zone detect + pond backoff + 8-min cap
│   │   ├── position.cpp/.h       → open-loop dead reckoning from commanded speeds (NO encoders, NO IMU)
│   │   ├── data_logger.cpp/.h    → Frozen-schema JSON → SPIFFS + /data
│   │   ├── rc_page.h             → AUTO-GENERATED PROGMEM copy of rc_page/index.html
│   │   └── wifi_server.cpp/.h    → AP HTTP server (GET /data, POST /rc)
│   └── rc_page/index.html        → RC page source — regenerate rc_page.h after editing (see PLAN.md)
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
    { "id": "W1", "type": "WATER", "sector": 1, "terrain": "Sandpaper", "value_ntu": 42.3, "value_pct": null, "t_s": 45.2 },
    { "id": "S1", "type": "SOIL",  "sector": 1, "terrain": "Sandpaper", "value_ntu": null, "value_pct": 68.5, "t_s": 78.1 }
  ]
}
```
Terrain names are rulebook "examples only" — `TERRAINS[]` table in `data_logger.cpp` gets confirmed on Testing Day.

## Servo Arm Mechanism
Single servo shaft drives TWO arms simultaneously:
- **Servo 0°** → Arm A (SEN0189) DOWN into water zone | Arm B UP
- **Servo 90°** → NEUTRAL, both arms horizontal (travel position)
- **Servo 180°** → Arm B (Capacitive Soil) DOWN onto disc | Arm A UP

Never move servo while robot is moving. Always return to 90° after sampling.

## RC Commands (POST /rc)
```json
{ "cmd": "FWD"|"BWD"|"LEFT"|"RIGHT"|"STOP", "speed": 0-255 }   // speed optional, drive cmds only
{ "cmd": "SAMPLE_WATER", "sector": 1-4 }   // arm 0° → settle 2s → 3s median window → 90°; ~6s total
{ "cmd": "SAMPLE_SOIL",  "sector": 1-4 }   // arm 180° → same settle+window → 90°
{ "cmd": "CAL_FWD",  "ms": 200-10000 }     // timed drive at DRIVE_SPEED, auto-stop (calibration — see arduino/CALIBRATION.md)
{ "cmd": "CAL_SPIN", "ms": 200-10000 }     // timed in-place spin at TURN_SPEED, auto-stop
{ "cmd": "END_RUN" }                        // finalise + save results.json to SPIFFS + HTTPS POST to webapp /api/viz-runs (practice/demo only, needs phone hotspot; STA creds + UPLOAD_URL in config.h)
{ "cmd": "START" }                          // autonomous only: arms FSM_INIT → run starts (also: BOOT button press). Robot never drives at power-on
{ "cmd": "REZERO", "h": 0-359 }             // re-zero dead-reckoned pose to origin at eyeballed heading (officials re-centre stuck robots — rulebook 5.2.1)
```
`GET /pos` → `{"x","y","h","t","ip"}` — live dead-reckoned position + run seconds (RC page polls at 2Hz, shows ⏱ amber at 7:00 / red at 8:00). `GET /samples` → samples-only JSON for `arduino/tools/live_plot.py`. WiFi is AP+STA: RobotAP always up, STA joins phone hotspot in background for upload.

**Run clock**: `startRun()` (data_logger) resets the JSON doc + clock. Fires on the first RC drive/sample after boot or END_RUN, and at autonomous START. Outside a live run, path logging and `total_duration_s` are frozen — never let `t_s` count from power-on. `WIFI_STA_*` creds in `config.h` are real — `sync-code.mjs` redacts them before the public `/code` browser; keep it that way.

**Autonomous pose convention**: run starts with robot at arena centre, heading = +x. Sector = dead-reckoned quadrant via `SECTOR_MAP` in `config.h` (confirm quadrant→sector mapping on Testing Day). After an official re-centres a stuck robot, send `REZERO` with the eyeballed heading.

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
- After firmware changes: `node scripts/sync-code.mjs`; after docs-content changes: `node scripts/sync-docs.mjs`

## Critical Rules
1. **Rulebook is law** — never modify robot behaviour that affects scoring without checking `/resources/EnviroBot Rulebook.md`. Rulebook has internal contradictions in category points — email organisers before relying on a number.
2. **JSON schema is frozen** — webapp visualiser depends on exact field names
3. **One servo** — never add a second servo or linear actuator; constraint is by design
4. **Wheels never touch water** — pond slots are recessed wheel traps; stop on detection, sample, REVERSE away (`FSM_POND_BACKOFF`), never drive over a slot
5. **Re-sample lockout** — no encoders means no exclusion-zone coordinates; `SAMPLE_LOCKOUT_MS` driving-time lockout prevents re-sampling the same zone
6. **8-minute hard limit** — `RUN_TIME_LIMIT_MS` cap in FSM; exceeding costs −5 pts
7. **Never hardcode sensor data** — pre-loaded readings zero out Categories 1 & 2 and risk disqualification
