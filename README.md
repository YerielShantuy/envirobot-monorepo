# EnviroBot

ESP32 environmental survey robot + web platform, built for the EnviroBot Environmental Robotics Challenge.

- **Mission**: 12 measurements (4 water turbidity + 8 soil moisture) in ≤8 minutes, scored across 6 categories.
- **Strategy**: RC-first — remote control is fully rulebook-legal; autonomy is a stretch goal for the top accuracy band + innovation points.

This is a monorepo: ESP32 firmware + a `webapp` git submodule (separate repo, deployed independently).

## Repo Structure

```
Enviorbot/
├── PRD.md / PLAN.md / DESIGN.md / CHECKPOINT.md   → planning artifacts
├── webapp/                    → git submodule → github.com/YerielShantuy/Enviorbot
│                                 Next.js 15 + TS + Tailwind 4, deployed to Cloudflare Workers
├── arduino/
│   ├── envirobot/             → main sketch (firmware source)
│   ├── rc_page/                → RC control page source (→ rc_page.h, PROGMEM)
│   ├── data_page/               → live-data page source (→ data_page.h)
│   ├── test_page/               → bench test-harness page source (→ test_page.h)
│   ├── tests/                  → standalone per-component bring-up sketches
│   │                             (i2c_scan, motors, servo, rc, soil, turbidity, ultrasonic, tcs34725)
│   ├── tools/                   → gen-pages.cjs (regen *_page.h from HTML), live_plot.py (judged output)
│   ├── CALIBRATION.md
│   └── CONFIG_PLAN.md
├── resources/                  → rulebook, arena spec, reference visualiser
└── graphify-out/                → generated code knowledge graph (gitignored content, tracked structure)
```

## Firmware (`arduino/envirobot/`)

ESP32, no wheel encoders, no IMU — position and heading are open-loop dead-reckoned from commanded wheel
speeds (`position.cpp`), path log only, never used for safety decisions.

Key files:
- `config.h` — hardware pins split into `hardware_pins.h` (physical, never runtime) + `robot_config.h`
  (runtime-tunable, overridden by `/config.json` at boot via `config_store.cpp`)
- `sensors.cpp/.h` — turbidity (SEN0189, voltage divider), capacitive soil, TCS34725 colour, HC-SR04
  ultrasonic (wall detection only)
- `servo_arm.cpp/.h` — single servo drives two arms: 0°=water sensor down, 90°=neutral/travel, 180°=soil
  sensor down
- `navigation.cpp/.h` — FSM: wander + zone detect + pond backoff + 8-minute hard cap
- `data_logger.cpp/.h` — frozen-schema JSON output → SPIFFS + `/data`
- `wifi_server.cpp/.h` — AP HTTP server: `/rc`, `/config`, `/view`, `/samples`, `/data`

See `arduino/CALIBRATION.md` for on-site sensor calibration and `arduino/CONFIG_PLAN.md` for the runtime
config architecture.

## webapp (submodule)

Next.js 15 platform: docs, Arduino code browser, CAD viewer, run visualiser, content admin. Deployed to
Cloudflare Workers via Wrangler/OpenNext — **never Vercel**. See `webapp/README.md` and `webapp/AGENTS.md`
for setup and deploy details.

Clone with submodules:
```
git clone --recurse-submodules https://github.com/YerielShantuy/envirobot-monorepo.git
```
Already cloned without it:
```
git submodule update --init --recursive
```

## Data Output Compliance (rulebook §4.6)

Serial output of every sample (`[SAMPLE] S<n> <TYPE> <value>`) is the always-legal fallback and must never
be removed. `arduino/tools/live_plot.py` is the primary judged output — a matplotlib live table + labelled
bar charts, reading either serial or `/samples` over the robot's WiFi AP. The webapp is an alternative
method requiring prior written approval from the competition's Project Directors.

## Rules

1. Rulebook is law — check `resources/EnviroBot Rulebook.md` before changing anything that affects scoring.
2. JSON output schema is frozen — the visualiser depends on exact field names.
3. One servo only — never add a second servo or actuator.
4. Never hardcode sensor data.
