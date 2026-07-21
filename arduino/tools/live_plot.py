#!/usr/bin/env python3
"""EnviroBot — live sample table + plot on a connected computer.

Rulebook 4.6 approved output format: "Live table plotted on a connected
computer (e.g. via Arduino Serial Plotter, Python matplotlib, or similar)".
This is that, via matplotlib. Every sample is also appended to samples.csv
(4.6: "Printed data log or CSV file output").

Two sources, same display:
  python live_plot.py --url http://192.168.4.1/samples   # laptop joined to RobotAP (during run)
  python live_plot.py --port COM5                        # USB serial (inspection / bench)

Deps: matplotlib always; requests for --url; pyserial for --port.
"""
import argparse
import csv
import math
import os
import re
import sys

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Matches both RC and autonomous firmware lines:
#   [SAMPLE] S1 WATER 42.3 NTU
#   [SAMPLE] S2 SOIL 68.5%  (5/12)
SAMPLE_RE = re.compile(r"\[SAMPLE\] S(\d+) (WATER|SOIL)\s+([\d.]+)")

CSV_PATH = "samples.csv"
WATER_COLOR = "#0277BD"
SOIL_COLOR = "#8D4E12"

samples = []   # {"id", "type", "sector", "value", "t_s"}
seen_ids = set()


def add_sample(s):
    if s["id"] in seen_ids:
        return
    seen_ids.add(s["id"])
    samples.append(s)
    new_file = not os.path.exists(CSV_PATH)
    with open(CSV_PATH, "a", newline="") as f:
        w = csv.writer(f)
        if new_file:
            w.writerow(["id", "type", "sector", "value", "unit", "t_s"])
        unit = "NTU" if s["type"] == "WATER" else "%"
        w.writerow([s["id"], s["type"], s["sector"], s["value"], unit, s["t_s"]])
    print(f"  + {s['id']}  S{s['sector']} {s['type']} {s['value']}")


# ── Sources ──────────────────────────────────────────────────────────
def make_serial_reader(port, baud):
    import serial  # pyserial
    ser = serial.Serial(port, baud, timeout=0)
    counts = {"WATER": 0, "SOIL": 0}
    buf = b""

    def poll():
        nonlocal buf
        buf += ser.read(4096)
        *lines, buf_tail = buf.split(b"\n")
        buf = buf_tail
        for raw in lines:
            m = SAMPLE_RE.search(raw.decode(errors="replace"))
            if not m:
                continue
            typ = m.group(2)
            counts[typ] += 1
            add_sample({
                "id": ("W" if typ == "WATER" else "S") + str(counts[typ]),
                "type": typ,
                "sector": int(m.group(1)),
                "value": float(m.group(3)),
                "t_s": None,
            })
    return poll


def make_url_reader(url):
    import requests

    def poll():
        try:
            d = requests.get(url, timeout=1.5).json()
        except Exception:
            return  # robot unreachable this tick — keep showing what we have
        for s in d.get("samples", []):
            val = s["value_ntu"] if s["type"] == "WATER" else s["value_pct"]
            if val is None:
                continue
            add_sample({
                "id": s["id"],
                "type": s["type"],
                "sector": s["sector"],
                "value": val,
                "t_s": s.get("t_s"),
            })
    return poll


def make_pos_reader(pos_url):
    import requests

    def poll():
        global last_pos
        try:
            d = requests.get(pos_url, timeout=1.0).json()
        except Exception:
            return  # robot unreachable this tick — keep the trail we have
        last_pos = d
        if not path or path[-1] != (d["x"], d["y"]):   # skip stationary duplicates
            path.append((d["x"], d["y"]))
    return poll


# ── Live position (URL mode only — /pos has no serial equivalent) ────
ARENA_RADIUS_MM = 750   # rulebook §5.1: ~1.5 m diameter
path = []               # [(x_mm, y_mm)] dead-reckoned trail
last_pos = None         # latest /pos dict {"x","y","h","t"}

# ── Display: table + live arena + two labelled bar charts (rulebook 6.2.2) ──
fig = plt.figure("EnviroBot Live Data", figsize=(12, 8))
fig.suptitle("EnviroBot — Live Sample Data", fontsize=14, fontweight="bold")
ax_table = fig.add_subplot(2, 2, 1)
ax_arena = fig.add_subplot(2, 2, 2)
ax_water = fig.add_subplot(2, 2, 3)
ax_soil = fig.add_subplot(2, 2, 4)


def draw_arena():
    ax_arena.clear()
    ax_arena.set_aspect("equal")
    ax_arena.set_title("Live position", fontsize=10)
    lim = ARENA_RADIUS_MM * 1.1
    ax_arena.set_xlim(-lim, lim)
    ax_arena.set_ylim(-lim, lim)
    ax_arena.set_xlabel("x (mm)")
    ax_arena.set_ylabel("y (mm)")
    # Octagon boundary — matches the outline drawn on the /view page
    oct_pts = [(ARENA_RADIUS_MM * math.cos(math.pi / 4 * i - math.pi / 8),
                ARENA_RADIUS_MM * math.sin(math.pi / 4 * i - math.pi / 8)) for i in range(8)]
    oct_pts.append(oct_pts[0])
    ax_arena.plot([p[0] for p in oct_pts], [p[1] for p in oct_pts], color="#2A3E5E", lw=1)
    ax_arena.axhline(0, color="#8899BB", lw=0.6, ls=":")
    ax_arena.axvline(0, color="#8899BB", lw=0.6, ls=":")
    if path:
        ax_arena.plot([p[0] for p in path], [p[1] for p in path], color="#00D4AA", lw=1.5)
        x, y = path[-1]
        ax_arena.plot([x], [y], "o", color="#00D4AA", ms=8)
        if last_pos is not None:
            h = math.radians(last_pos.get("h", 0))   # +x = 0°, matches arena convention
            ax_arena.arrow(x, y, 90 * math.cos(h), 90 * math.sin(h), head_width=40, color="#00D4AA")
    else:
        ax_arena.text(0, 0, "waiting for position…", ha="center", va="center", color="gray")


def redraw(_frame, poll, pos_poll):
    poll()
    if pos_poll:
        pos_poll()
        draw_arena()
    ax_table.clear()
    ax_table.axis("off")
    rows = [[s["id"], s["type"], f"S{s['sector']}",
             f"{s['value']:.1f} {'NTU' if s['type'] == 'WATER' else '%'}",
             f"{s['t_s']:.0f}s" if s["t_s"] is not None else "-"]
            for s in samples]
    if rows:
        tbl = ax_table.table(
            cellText=rows,
            colLabels=["ID", "Type", "Sector", "Value", "Time"],
            loc="center", cellLoc="center")
        tbl.auto_set_font_size(False)
        tbl.set_fontsize(9)
        tbl.scale(1, 1.25)
    else:
        ax_table.text(0.5, 0.5, "waiting for samples…",
                      ha="center", va="center", color="gray")
    ax_table.set_title(f"Samples ({len(samples)}/12)", fontsize=10)

    water = [s for s in samples if s["type"] == "WATER"]
    soil = [s for s in samples if s["type"] == "SOIL"]

    ax_water.clear()
    ax_water.bar([s["id"] for s in water], [s["value"] for s in water], color=WATER_COLOR)
    ax_water.set_title("Water turbidity")
    ax_water.set_ylabel("Turbidity (NTU)")
    ax_water.set_xlabel("Sample")

    ax_soil.clear()
    ax_soil.bar([s["id"] for s in soil], [s["value"] for s in soil], color=SOIL_COLOR)
    ax_soil.set_title("Soil moisture")
    ax_soil.set_ylabel("Moisture (%)")
    ax_soil.set_xlabel("Sample")
    ax_soil.set_ylim(0, 100)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument("--port", help="serial port, e.g. COM5")
    src.add_argument("--url", help="robot samples endpoint, e.g. http://192.168.4.1/samples")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--interval", type=float, default=2.0, help="refresh seconds")
    args = p.parse_args()

    if args.port:
        poll = make_serial_reader(args.port, args.baud)
        pos_poll = None   # serial stream has no /pos — arena panel stays blank
        ax_arena.text(0, 0, "position unavailable\n(serial mode)", ha="center", va="center", color="gray")
        ax_arena.set_title("Live position", fontsize=10)
    else:
        poll = make_url_reader(args.url)
        pos_poll = make_pos_reader(args.url.replace("/samples", "/pos"))
    anim = FuncAnimation(fig, redraw, fargs=(poll, pos_poll),
                         interval=int(args.interval * 1000), cache_frame_data=False)
    plt.tight_layout(rect=(0, 0, 1, 0.95))
    plt.show()
    return anim


if __name__ == "__main__":
    sys.exit(0 if main() else 0)
