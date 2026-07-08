# EnviroBot — Implementation Plan

**Version**: 2.0 | **Date**: 2026-07-07 | Supersedes v1.0 (audit revision)

Strategy: **RC-first** (see PRD §2). Timeline anchors: Workshop help sessions Fri 10/17/24 July · **Testing Day Thu 30 July** · **Closing Night Fri 31 July**.

---

## Phase 0 — Compliance actions (this week, blocking)

- [ ] **Email organisers** (createunsw@gmail.com AND unsw@studentenergy.org):
  1. Confirm scoring weightings (rulebook table vs headings contradict — 30/40 Cat 1, 40/25 Cat 6, 5/10 Cat 3)
  2. Request written approval for WiFi webapp as data output method (rulebook §4.6)
- [ ] Confirm at a workshop: actual arena terrains, water slot dimensions/depth, soil disc size, whether reference values are announced before runs

## Phase 1 — Hardware (physical work, firmware is ready)

- [x] **Voltage divider** on SEN0189 output → GPIO36 — done 2026-07-08. Firmware assumes ratio **2.0** (equal resistors); if different values were used, update `TURBIDITY_DIVIDER_RATIO` in `config.h` before calibrating
- [x] Wired per `config.h` — done 2026-07-08: motors 13/14/4 + 18/19/23 · servo 26 · I2C bus 0 SDA 21 / SCL 22 (MPU6050 + TCS34725) · I2C bus 1 SDA 16 / SCL 17 (VL53L0X) · turbidity 36 · soil 39
- [x] **TCS34725 mounted ahead of front wheels, facing down** — done 2026-07-08. On arena day, verify the stop margin: drive toward a water slot at `DRIVE_SPEED`, confirm wheels halt clear of the recess (lookahead distance must exceed stopping distance)
- [ ] Verify dims ≤400×300×400mm, weight ≤1.5kg with battery; servo at 90° for measurement
- [ ] Wiring neatness + insulation — Build Quality is judged; battery secured and switchable (instant power-down on official instruction)

## Phase 2 — Firmware bring-up + calibration (highest-value engineering time)

Firmware state: RC + autonomous FSM code complete after audit fixes. Bring-up order:

1. [ ] Flash, join `RobotAP`, open `192.168.4.1` — drive all directions, verify motor polarity
2. [ ] Calibrate `MM_PER_SEC_AT_DRIVE`: drive a measured 1m on arena-like surface, time it
3. [ ] Servo arm: verify 0°/90°/180° reach water slot depth and soil disc contact without hitting chassis
4. [ ] **Turbidity calibration**: prepare reference samples (clear water, diluted milk steps), record ADC voltage vs known NTU, refit polynomial in `sensors.cpp` — DFRobot curve is only a starting point and the divider changes everything. Target ±10%.
5. [ ] **Soil calibration**: `SOIL_DRY_VAL` in air-dry soil, `SOIL_WET_VAL` in saturated soil (the actual competition-style pots if possible)
6. [ ] Zone-detection thresholds (`WATER_*`, `SOIL_*` in `config.h`) under venue-like lighting
7. [ ] Full 8-min RC rehearsal: 12 samples with sector tagging → End Run → `GET /data` → drop JSON into `/viz` — the complete judge-facing flow

RC page: source is `arduino/rc_page/index.html`. After editing, regenerate the PROGMEM header:
```powershell
cd arduino
node -e "const fs=require('fs');const html=fs.readFileSync('rc_page/index.html','utf8');fs.writeFileSync('envirobot/rc_page.h','#pragma once\n#include <Arduino.h>\n\n// AUTO-GENERATED from arduino/rc_page/index.html — do not edit by hand.\nstatic const char RC_PAGE_HTML[] PROGMEM = R\"rawliteral(\n'+html+')rawliteral\";\n')"
```

## Phase 3 — Autonomy (stretch — only after Phase 2 is solid)

- [ ] Bench-test FSM with wheels off ground: state transitions on hand-triggered sensors
- [ ] Arena test: wander + wall bounce (VL53L0X), pond stop → sample → backoff, soil detect
- [ ] Tune `SAMPLE_LOCKOUT_MS`, `POND_BACKOFF_MS`, `WALL_STOP_MM` on the real arena
- [ ] Go/no-go rule: if autonomy isn't reliably finding ≥6 zones by Fri 24 July workshop, demo RC and mention autonomy in the innovation pitch only

## Phase 4 — Closing Night prep (40 pts — biggest category, don't starve it)

- [ ] Team roles for the 3-min presentation + Q&A (multiple members must contribute — it's scored)
- [ ] Practice case: take a real run's JSON, build a data-driven argument (e.g. "which sector is suitable for crop X"), acknowledge sensor uncertainty and tradeoffs
- [ ] Prepare limitation talking points: ±10% sensor band, single-sample-per-pot vs averaging, position estimate accuracy
- [ ] 3-min innovation pitch for Testing Day robot assessment: double-arm single-servo story, dual I2C bus fix, iteration evidence
- [ ] Visualiser rehearsal: JSON → charts in under a minute (judges review data immediately after run)

## Phase 5 — Webapp (showcase; robot takes priority)

- [x] **Build verify** — done 2026-07-08: migrated to OpenNext (`@opennextjs/cloudflare`); `@cloudflare/next-on-pages` was deprecated for Next 15 and unbuildable on Windows. Deploy is now `npm run deploy` (Workers, Wrangler under the hood). Config: `wrangler.jsonc` + `open-next.config.ts`. No `runtime = 'edge'` exports allowed.
- [x] Hero asset — `public/hero-envirobot.webp` exists and is wired into HeroSection
- [x] `/code` browser re-sync — `node scripts/sync-code.mjs` regenerates seed-code from real firmware (run again after any firmware change, then re-seed from `/admin`)
- [x] Docs updated — `lib/docs-content.ts` rewritten to real robot; `node scripts/sync-docs.mjs` mirrors it into `content/docs/*.mdx`
- [ ] **Deploy**: set secrets once (`wrangler secret put` for `R2_*`, `SUPABASE_*`, admin), then `npm run deploy`; hit "Seed code" in `/admin` to push re-synced sources; clear any stale `docs_pages` rows in Supabase (they override the baked-in docs)
- [ ] Verify `/viz` renders a real robot-produced `results.json` (not just hand-written fixtures) — nulls in `value_ntu`/`value_pct`, sector 0 ("Unknown") handling
- [ ] Retire the old Cloudflare Pages project `envirobot` once the Worker is live
- [ ] `/code-review` before the deploy commit

---

## Integration checklist (before Testing Day)

- [ ] `GET /data` matches frozen schema exactly (validate in `/viz`)
- [ ] Serial output shows every sample with units (compliance fallback)
- [ ] `/data` still serves last run after ESP32 reboot (SPIFFS fallback)
- [ ] RC page loads with no SPIFFS upload (PROGMEM) and no internet (no CDN deps)
- [ ] 8-min cap fires in autonomous mode; End Run works in RC mode
- [ ] Phone + laptop both tested against `RobotAP`
- [ ] Spare battery charged; inspection checklist walked through (dims, sensors, one live reading)
