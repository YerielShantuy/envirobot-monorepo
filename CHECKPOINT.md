# EnviroBot — Build Checkpoint

Last updated: 2026-07-16 (autonomy hardening + run clock + live table)

## Status: firmware feature-complete for RC + autonomy stretch; next = flash + bench FSM test + calibration

---

## Autonomy + output session (2026-07-16)

**Autonomy hardening** (user session, CodeRabbit-reviewed, all inside `#if ENABLE_AUTONOMOUS`): armed-start gate (`START` cmd / BOOT press, LED blink while armed, backdated lockout), `SAMPLE_CREEP_MS` probe nudge (default 0, CALIBRATION.md §6), 8s leg timeout stuck-escape, avoid-backoffs no longer re-arm the sample lockout, water/soil caps 4/8, sector from dead-reckoned quadrant + `SECTOR_MAP`, `REZERO` cmd + `resetPose()`, confirm re-read kills phantom samples, no sampling in final 8s (`SAMPLE_TIME_BUDGET_MS`), heading deadband 0.20 rad, ±0.35 rad bounce jitter, TCS integration 50→24ms (recalibrate thresholds at 24ms!).

**Live table (§4.6 judged output)**: `arduino/tools/live_plot.py` — matplotlib live table + labelled NTU/moisture bar charts + `samples.csv`; `--port COM5` or `--url http://192.168.4.1/samples` (new lightweight endpoint). Webapp §4.6 approval email now optional.

**Reaudit fixes (this session)**:
- **Run clock**: `startRun()`/`runLive()`/`runElapsedS()` in `data_logger` — doc + clock reset on first RC drive/sample or autonomous START; path logging and totals frozen outside a live run (previously `t_s`/`total_duration_s`/path counted from power-on; path cap filled with pre-run idle points). END_RUN freezes; next drive opens a fresh doc
- **RC page**: ⏱ run timer in posbar from `/pos` `t` field (amber 7:00, red 8:00); no false "offline" during the ~6s blocking sample; `rc_page.h` regenerated (15.9KB)
- **Creds**: `sync-code.mjs` now redacts `WIFI_STA_SSID/PASS` before publishing to the public `/code` browser (live R2 copy verified pre-STA — no leak occurred); push path also fixed to byte-correct Content-Length
- **`UPLOAD_URL`**: set to `https://envirobot.yerielph.workers.dev/api/viz-runs` — worker verified live (deployed 2026-07-09), endpoint returns runs
- Docs: PLAN Phase 1 GPIO36→34 corrected (user confirmed 34), aesthetics task added (Cat 5), innovation pitch bullet refreshed (dual-I2C claim removed — hardware dropped 07-10), integration checklist gained run-clock + live_plot items; `sensors.h` 50ms comment → 24ms

**Next**: flash `ENABLE_AUTONOMOUS 1`, wheels off ground, bench-trigger the FSM (PLAN Phase 3) · Phase 2 calibration · re-seed `/code` via `sync-code.mjs --push`.

---

## Component revision (2026-07-10)

Hardware reduced to what's actually on hand. **Dropped: MPU6050 IMU, VL53L0X ToF.** **Added: HC-SR04 ultrasonic** (wall detection only). Motors (2× JGA25-370 + L298N), MG996R servo, SEN0189 turbidity, capacitive soil, TCS34725 all unchanged.

Firmware follow-through:
- `config.h` — dropped second I2C bus + VL53L0X `WALL_STOP` comment; added `PIN_ULTRASONIC_TRIG/ECHO` (16/17) + `ULTRASONIC_TIMEOUT_US`; added `WHEEL_BASE_MM` for the new heading model
- `sensors.cpp/.h` — removed `VL53L0X`; `readToFDistanceMM()` → `readWallDistanceMM()` (HC-SR04 `pulseIn`, 0.343 mm/µs, timeout → "no wall")
- `position.cpp/.h` — removed MPU6050 DMP; heading now open-loop from the differential-drive model `ω=(vR−vL)/WHEEL_BASE_MM`. Drifts (no gyro) — path log only. `headingValid()` always true
- `navigation.cpp/.h` — dead-IMU refuse block removed (nothing to fail); `readWallDistanceMM()`; FSM comment now "ultrasonic"
- `envirobot.ino` — `initPosition()` no longer waits ~2s for DMP
- tests — deleted `test_mpu6050`, `test_vl53l0x`; added `test_ultrasonic`

Impact: heading/position drift more than before (no IMU) and don't survive a reposition — RC-first strategy already treats the path log as best-effort, so scoring is unaffected. **Hardware TODO: rewire per PLAN §Phase 1 (HC-SR04 ECHO needs a 5V→3.3V divider), recalibrate `WHEEL_BASE_MM` via a 360° in-place spin.**

---

## Audit outcome (2026-07-07)

Strategy decisions (user-confirmed):
- **RC-first**, autonomy = stretch goal
- **No wheel encoders** on hardware — dead-reckoning removed, IMU heading + speed model instead
- Webapp admin/Supabase/R2 kept — PRD updated to match

Firmware fixes applied:
- `config.h` — motors moved off I2C pins 21/22 (was hard conflict); encoder pins removed; calibration knobs added (`TURBIDITY_DIVIDER_RATIO`, `MM_PER_SEC_AT_DRIVE`, zone thresholds, `RUN_TIME_LIMIT_MS`, `SAMPLE_LOCKOUT_MS`, `POND_BACKOFF_MS`); VL53L0X on second I2C bus 16/17 (0x29 clash with TCS34725 — TCA9548A dropped)
- `position.cpp` — rewritten: MPU6050 DMP absolute yaw (zeroed at init) + commanded-speed position estimate; broken 70/30 absolute-heading fusion removed
- `navigation.cpp` — 8-min hard cap; re-sample time lockout (was: infinite loop sampling first zone 12×); `POND_BACKOFF` state (stop → sample → REVERSE — wheels never cross recessed water slot); ToF wall bounce (was: never called); non-blocking COMPLETE (server stays alive)
- `data_logger.cpp` — frozen schema now actually met: `"W1"`/`"S1"` ids, `sector` + `terrain`, explicit nulls; `sectors_completed` from distinct sectors; `/data` falls back to SPIFFS `results.json` after reboot; doc 48KB
- `sensors.cpp` — turbidity divider ratio (SEN0189 outputs 4.5V > 3.3V ADC max — hardware divider REQUIRED); NTU clamped; zone thresholds from config; VL53L0X on Wire1
- `wifi_server.cpp` — `sector` param on samples, `END_RUN` cmd, optional `speed`, serial print per sample (rulebook-approved output fallback), RC page served from PROGMEM
- `rc_page/index.html` — sector selector S1–S4 + End Run button; `rc_page.h` auto-generated (regen command in PLAN.md Phase 2)
- `envirobot.ino` — WiFi AP + server in BOTH modes (judges pull /data right after autonomous run); encoder init removed
- `servo_arm.cpp` — ESP32Servo pinned to LEDC timers 2/3 (motors own timer 0)

Docs: PRD → v1.1 (real scoring table, RC-first, compliance section) · PLAN → v2.0 (phases 0–5) · CLAUDE.md (hardware truths, new RC protocol, compliance rules)

## Webapp session (2026-07-08)

- **Migrated to OpenNext** — `@cloudflare/next-on-pages` was deprecated for Next 15 AND unbuildable on Windows (`spawn npx ENOENT` inside its `vercel build` wrapper). Now: `@opennextjs/cloudflare` 1.20 + next 15.5.20 + wrangler 4.108, `wrangler.jsonc` + `open-next.config.ts`, all `runtime = 'edge'` exports removed (OpenNext requirement). Build + `wrangler deploy --dry-run` both pass (6MB worker / 1.26MB gzip). Deploy = `npm run deploy`.
- **Docs rewritten to real robot** — `lib/docs-content.ts` (render source, all 11 pages) no longer describes the never-built "Rev 5" hardware (3× TCS, encoders, NeoPixel, WebSocket). `scripts/sync-docs.mjs` mirrors it into `content/docs/*.mdx` (11 files).
- **/code browser re-synced** — `scripts/sync-code.mjs` regenerates `seed-code/route.ts` from the real firmware (15 files). Run after any firmware change.
- **Hero asset verified** — `public/hero-envirobot.webp` exists (Jun 27), on-brief, wired in HeroSection. CHECKPOINT's old "pending" entry was stale.

## Pending — resume here (PLAN.md v2.0 is the source of truth)

1. **Phase 0**: organiser email (scoring weightings + RC-band question; webapp approval now optional) — user's call, deliberately deferred 2026-07-16
2. **Phase 1 hardware: COMPLETE** (2026-07-08) — divider on GPIO34 ✓, colour sensor ahead of wheels ✓, rewired to new pin map ✓. Divider assumed ratio 2.0 — update `TURBIDITY_DIVIDER_RATIO` if resistors differ. Remaining: dims/weight check + wiring dress + aesthetics (Cat 5)
3. **Phase 2**: bring-up + sensor calibration (±10% target) — user-deferred, highest-value work left
4. **Phase 3**: bench FSM test (`ENABLE_AUTONOMOUS 1`, wheels off ground, hand-trigger sensors)
5. **Phase 5**: webapp LIVE at `envirobot.yerielph.workers.dev` (2026-07-09). Remaining: `sync-code.mjs --push` re-seed (redacted), clear stale `docs_pages` rows, retire old Pages project
6. `/viz` against a real robot-produced `results.json`; Emil design review; full code review; graphify update

## Key facts (context restoration)

- Frozen JSON schema: `run_id, total_duration_s, sectors_completed, path[]{t_s,x_mm,y_mm}, samples[]{id,type,sector,terrain,value_ntu,value_pct,t_s}` — nulls explicit; clock starts at `startRun()`, not boot
- Servo: 0° water · 90° neutral · 180° soil
- RC protocol: see project CLAUDE.md §RC Commands (FWD/BWD/LEFT/RIGHT/STOP, SAMPLE_*, CAL_*, START, REZERO, END_RUN) · `GET /data` · `GET /samples` (live_plot feed) · `GET /pos` (x/y/h/t/ip)
- Deploy: `cd webapp; npm run deploy` — OpenNext + Wrangler to Workers, live at `envirobot.yerielph.workers.dev` (never Vercel, old Pages command dead since 2026-07-08)
- Judged data output: `arduino/tools/live_plot.py` (matplotlib table + labelled charts + samples.csv); serial `[SAMPLE] S<n> <TYPE> <value>` lines are the parse format — keep shape
- Scoring: Cat 6 presentation 40 · Cat 1 measurement 30 · rest 30 — rulebook contradicts itself, confirmation pending
- Fonts: Syne / IBM Plex Sans / JetBrains Mono
