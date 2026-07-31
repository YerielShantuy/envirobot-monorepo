# EnviroBot

ESP32 environmental survey robot + web platform, built for the EnviroBot Environmental Robotics Challenge.

- **Mission**: 12 measurements (4 water turbidity + 8 soil moisture) in ≤8 minutes, scored across 6 categories.
- **Arena**: 1500 mm square, four 750 mm terrain quadrants — S1 Urban (top-left), S2 Field (top-right),
  S3 Desert (bottom-left), S4 Stone (bottom-right). One water zone + two soil zones per quadrant.
- **Strategy**: Remote controlled — the operator drives, picks each sample's sector on the RC page, and
  keeps the run inside 8 minutes using the page's run clock. There is no autonomous mode in the firmware.

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
│   │                             (motors, servo, rc, soil, turbidity, ultrasonic)
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
- `sensors.cpp/.h` — turbidity (SEN0189, 220 Ω/470 Ω divider, ratio 1.4681), capacitive soil, HC-SR04
  collision guard (the only obstacle sensor: closer than `obstacleStopMm` cuts the motors and refuses
  forward drive, while reverse and turns stay available). No I2C bus, no colour sensor.
- `servo_arm.cpp/.h` — single servo drives two arms: 0°=water sensor down, 90°=neutral/travel, 180°=soil
  sensor down
- `navigation.cpp/.h` — motor layer only: drive, turn, brake, and the collision-guard veto. No FSM.
- `data_logger.cpp/.h` — frozen-schema JSON output → SPIFFS `results.json`, served at `/data`
- `wifi_server.cpp/.h` — AP-only HTTP server: `/rc`, `/pos`, `/config`, `/view`, `/samples`, `/data`.
  The robot is the network (SSID `RobotAP` at `http://192.168.4.1`) — no STA mode, no internet, no upload.

See `arduino/CALIBRATION.md` for on-site sensor calibration and `arduino/CONFIG_PLAN.md` for the runtime
config architecture.

## webapp (submodule)

Next.js 15 platform: docs, Arduino code browser, CAD viewer, run visualiser, content admin. The visualiser
plays back a `results.json` the operator downloads from the robot's RC page — runs are never uploaded.
Deployed to Cloudflare Workers via Wrangler/OpenNext — **never Vercel**. See `webapp/README.md` and
`webapp/AGENTS.md` for setup and deploy details.

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
