// Host unit test for the collision guard's debounce + hysteresis rules.
// Includes the REAL header the firmware uses — no copy, so it cannot drift.
// Run it from PowerShell (Git Bash on this machine refuses to exec the .exe):
//   g++ -std=c++17 -Wall -Wextra arduino/tools/test_collision.cpp -o tc.exe; ./tc.exe
//
// Covers only the pure decision. The on/off switch (config.collisionGuardOn)
// gates this whole function from wifi_server.cpp and is not modelled here.
#include "../envirobot/collision.h"
#include <cassert>
#include <cstdio>

// Firmware values (config.h). Duplicated here only because config.h pulls in
// Arduino headers; keep them in sync if those defines change.
static const unsigned char TRIP = 2;
static const int HYST = 40;
static const int STOP = 120;

// Feed n identical pings, return the final blocked state.
static bool feed(ObstacleGuard& g, float mm, int n) {
  bool b = g.blocked;
  for (int i = 0; i < n; i++) b = obstacleStep(g, mm, STOP, TRIP, HYST);
  return b;
}

int main() {
  // 1. One in-range ping must NOT trip — that is the glitch-rejection.
  {
    ObstacleGuard g;
    assert(!feed(g, 100.0f, 1));
    assert(feed(g, 100.0f, 1));   // second consecutive one does
  }

  // 2. A glitch followed by clear air never trips.
  {
    ObstacleGuard g;
    feed(g, 100.0f, 1);           // one bad read
    feed(g, 500.0f, 1);           // well clear → hits reset
    assert(!feed(g, 100.0f, 1));  // so this is again only the first hit
  }

  // 3. Blocked stays blocked inside the hysteresis band (120..160).
  {
    ObstacleGuard g;
    assert(feed(g, 100.0f, TRIP));
    assert(feed(g, 130.0f, 5));   // past the threshold but inside the band
    assert(feed(g, 159.0f, 5));   // still inside
    assert(!feed(g, 160.0f, 1));  // exactly stop+hyst clears
  }

  // 4. Never blocks on the no-echo sentinel (nothing in range).
  {
    ObstacleGuard g;
    assert(!feed(g, 8190.0f, 20));
  }

  // 5. Boundary: a read exactly AT the threshold is not "closer than", so it
  //    must not accumulate a hit.
  {
    ObstacleGuard g;
    assert(!feed(g, (float)STOP, 10));
  }

  // 6. Hits saturate — a long block then one clear read releases immediately,
  //    it must not need TRIP clear reads to unwind.
  {
    ObstacleGuard g;
    feed(g, 50.0f, 100);
    assert(g.blocked);
    assert(!feed(g, 300.0f, 1));
  }

  printf("PASS: collision guard - debounce, hysteresis band, sentinel, boundary (6 cases)\n");
  return 0;
}
