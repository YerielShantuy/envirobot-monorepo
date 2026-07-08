# EnviroBot — Build Checkpoint

Last updated: 2026-07-07 (post-audit)

## Status: AUDITED — firmware rewritten, plan v2.0, RC-first strategy

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

1. **Phase 0**: organiser email (scoring contradiction + webapp output approval) — blocking, human task
2. **Phase 1 hardware: COMPLETE** (2026-07-08) — divider ✓, colour sensor ahead of wheels ✓, rewired to new pin map ✓. Divider assumed ratio 2.0 — update `TURBIDITY_DIVIDER_RATIO` if resistors differ. Next: Phase 2 bring-up + calibration
3. **Phase 2**: bring-up + sensor calibration (±10% target)
4. **Phase 5 deploy**: `wrangler secret put` for `R2_*` / `SUPABASE_*` / admin → `npm run deploy` → hit "Seed code" in `/admin` → clear stale `docs_pages` rows in Supabase (they override baked-in docs) → retire old Pages project
5. `/viz` against a real robot-produced `results.json`; Emil design review; full code review; graphify update

## Key facts (context restoration)

- Frozen JSON schema: `run_id, total_duration_s, sectors_completed, path[]{t_s,x_mm,y_mm}, samples[]{id,type,sector,terrain,value_ntu,value_pct,t_s}` — nulls explicit
- Servo: 0° water · 90° neutral · 180° soil
- RC: `POST /rc {cmd: FWD|BWD|LEFT|RIGHT|STOP[,speed]} | {cmd: SAMPLE_WATER|SAMPLE_SOIL, sector:1-4} | {cmd: END_RUN}` · `GET /data`
- Deploy: `npx @cloudflare/next-on-pages` → `wrangler pages deploy .vercel/output/static --project-name envirobot` (never Vercel)
- Scoring: Cat 6 presentation 40 · Cat 1 measurement 30 · rest 30 — rulebook contradicts itself, confirmation pending
- Fonts: Syne / IBM Plex Sans / JetBrains Mono
