#pragma once

// Pure collision-guard decision, split out of wifi_server.cpp so it can be
// unit-tested on the host (tools/test_collision.cpp) — no Arduino headers,
// no globals, no I/O. wifi_server.cpp owns the ping timing and the motor cut;
// this owns only "given this reading, should forward drive be blocked?".
//
// Two behaviours that a bare `mm < threshold` gets wrong:
//   debounce  — one HC-SR04 glitch must not halt a run, so a trip needs
//               `tripHits` consecutive in-range reads.
//   hysteresis— a bare threshold chatters at the boundary, so clearing needs
//               a read `hystMm` FURTHER out than the trip threshold.
// Readings between the two thresholds change nothing: they neither add a hit
// nor clear one, which is what makes the band sticky.

struct ObstacleGuard {
  unsigned char hits = 0;
  bool blocked = false;
};

// mm: latest ping (the firmware's no-echo sentinel is a large value, which
// simply reads as "far" here). stopMm: config.obstacleStopMm.
// Returns the new blocked state; also updates g.
inline bool obstacleStep(ObstacleGuard& g, float mm, int stopMm,
                         unsigned char tripHits, int hystMm) {
  if (mm < (float)stopMm) {
    if (g.hits < tripHits) g.hits++;
  } else if (mm >= (float)(stopMm + hystMm)) {
    g.hits = 0;
  }
  g.blocked = g.hits >= tripHits;
  return g.blocked;
}
