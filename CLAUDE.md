# EnviroBot — Project Instructions

## What This Is
ESP32 environmental survey robot + Next.js web platform for the EnviroBot Environmental Robotics Challenge.
- **Competition dates**: Testing Day Thu 30 July (robot run + inspection) · Closing Night Fri 31 July 5–9pm (case competition)
- **Mission**: 12 measurements (4 water turbidity + 8 soil moisture) in ≤8 minutes
- **Scoring**: 100 pts across 6 categories — Closing Night presentation (40) is the biggest; always check rulebook before changing behaviour
- **Strategy**: RC-first. Remote control is fully rulebook-legal; autonomy is a stretch goal for the top accuracy band + innovation points.

## Hardware Truths (do not contradict)
- **No wheel encoders.** Position is IMU heading + commanded-speed estimate (`position.cpp`), path log only — never for safety decisions.
- **SEN0189 needs a voltage divider** before the ESP32 ADC (outputs ~4.5V, pin max 3.3V). Ratio in `config.h` `TURBIDITY_DIVIDER_RATIO`.
- **TCS34725 and VL53L0X both use I2C address 0x29** — VL53L0X lives on second bus `Wire1` (pins 16/17). No TCA9548A mux.
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
│   │   ├── sensors.cpp/.h        → Turbidity (divider) + soil + TCS34725 + VL53L0X (Wire1)
│   │   ├── servo_arm.cpp/.h      → Double-arm servo (0°=water, 90°=neutral, 180°=soil)
│   │   ├── navigation.cpp/.h     → FSM: wander + zone detect + pond backoff + 8-min cap
│   │   ├── position.cpp/.h       → IMU heading + speed-model position (NO encoders)
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
{ "cmd": "SAMPLE_WATER", "sector": 1-4 }   // arm 0° → read → 90°; sector tags the sample
{ "cmd": "SAMPLE_SOIL",  "sector": 1-4 }   // arm 180° → read → 90°
{ "cmd": "END_RUN" }                        // finalise + save results.json to SPIFFS
```

## Data Output Compliance (rulebook §4.6)
Approved formats: serial monitor, live plot on connected computer, printed log/CSV. The WiFi webapp is an **alternative method — needs prior written approval** from Project Directors (createunsw@gmail.com + unsw@studentenergy.org). Serial output of every sample is the always-legal fallback — never remove the `Serial.printf` sample lines.

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
