// ── EnviroBot component test: RC control path (hardware only) ───────
// Stands up the WiFi AP + POST /rc endpoint and drives the REAL motors
// and servo from RC commands. No sensors, no data logger, no FSM —
// just confirms the remote-control hardware chain works end to end.
//
// Test: join WiFi "RobotAP" (pass enviro123), open http://192.168.4.1
// Buttons send the same JSON the real rc_page uses. Robot on blocks!
//   FWD/BWD/LEFT/RIGHT/STOP → wheels     SAMPLE_WATER/SOIL → servo sweep
// Every received cmd is echoed to Serial (rulebook-legal data display).
//
// Pins/LEDC/servo-timer split match test_motors + test_servo so the
// motor PWM (LEDC 0/1) never clashes with ESP32Servo (timers 2/3).
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#define PIN_MOTOR_L_FWD 13
#define PIN_MOTOR_L_BWD 14
#define PIN_MOTOR_L_PWM 4
#define PIN_MOTOR_R_FWD 18
#define PIN_MOTOR_R_BWD 19
#define PIN_MOTOR_R_PWM 23
#define PIN_SERVO       26
#define SERVO_NEUTRAL   90
#define SERVO_WATER      0
#define SERVO_SOIL     180

#define WIFI_SSID   "RobotAP"
#define WIFI_PASS   "enviro123"
#define DRIVE_SPEED 180
#define TURN_SPEED  140

WebServer server(80);
Servo srv;
static bool driving = false;
static unsigned long lastDriveMs = 0;

// ── Motor primitives (inline, standalone — matches test_motors) ──────
static void pwm(int l, int r) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_MOTOR_L_PWM, l); ledcWrite(PIN_MOTOR_R_PWM, r);
#else
  ledcWrite(0, l); ledcWrite(1, r);
#endif
}
static void left(bool fwd)  { digitalWrite(PIN_MOTOR_L_FWD, fwd); digitalWrite(PIN_MOTOR_L_BWD, !fwd); }
static void right(bool fwd) { digitalWrite(PIN_MOTOR_R_FWD, fwd); digitalWrite(PIN_MOTOR_R_BWD, !fwd); }

static void driveStop() {
  pwm(0, 0);
  digitalWrite(PIN_MOTOR_L_FWD, LOW); digitalWrite(PIN_MOTOR_L_BWD, LOW);
  digitalWrite(PIN_MOTOR_R_FWD, LOW); digitalWrite(PIN_MOTOR_R_BWD, LOW);
}
static void driveForward(int speed) {           // negative = reverse
  bool fwd = speed >= 0; int s = abs(speed);
  left(fwd); right(fwd); pwm(s, s);
}
static void turnLeft(int speed)  { left(false); right(true);  pwm(speed, speed); }
static void turnRight(int speed) { left(true);  right(false); pwm(speed, speed); }

// Hardware-only SAMPLE: sweep the servo to the probe position and back.
// Real firmware reads a sensor here — this just proves the arm actuates.
static void armSweep(const char* label, int deg) {
  driveStop(); driving = false;      // never move servo while driving
  Serial.printf("[SAMPLE] %s — servo %d deg\n", label, deg);
  srv.write(deg); delay(800);
  srv.write(SERVO_NEUTRAL); delay(600);
}

// ── /rc endpoint — same JSON schema as wifi_server.cpp ───────────────
static void sendJSON(int code, const String& body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "application/json", body);
}

static void handleRC() {
  if (!server.hasArg("plain")) { sendJSON(400, "{\"error\":\"no body\"}"); return; }
  StaticJsonDocument<128> req;
  if (deserializeJson(req, server.arg("plain"))) { sendJSON(400, "{\"error\":\"invalid json\"}"); return; }

  const char* cmd = req["cmd"] | "";
  int speed = constrain(req["speed"] | DRIVE_SPEED, 0, 255);
  Serial.printf("[RC] cmd=%s speed=%d\n", cmd, speed);

  if      (strcmp(cmd, "FWD")          == 0) { driveForward(speed);  driving = true; lastDriveMs = millis(); sendJSON(200, "{}"); }
  else if (strcmp(cmd, "BWD")          == 0) { driveForward(-speed); driving = true; lastDriveMs = millis(); sendJSON(200, "{}"); }
  else if (strcmp(cmd, "LEFT")         == 0) { turnLeft(TURN_SPEED);  driving = true; lastDriveMs = millis(); sendJSON(200, "{}"); }
  else if (strcmp(cmd, "RIGHT")        == 0) { turnRight(TURN_SPEED); driving = true; lastDriveMs = millis(); sendJSON(200, "{}"); }
  else if (strcmp(cmd, "STOP")         == 0) { driveStop(); driving = false; sendJSON(200, "{}"); }
  else if (strcmp(cmd, "SAMPLE_WATER") == 0) { armSweep("WATER", SERVO_WATER); sendJSON(200, "{\"ok\":true}"); }
  else if (strcmp(cmd, "SAMPLE_SOIL")  == 0) { armSweep("SOIL",  SERVO_SOIL);  sendJSON(200, "{\"ok\":true}"); }
  else sendJSON(400, "{\"error\":\"unknown cmd\"}");
}

// Minimal self-contained control page (real firmware serves rc_page.h).
static const char PAGE[] PROGMEM = R"HTML(<!doctype html><meta name=viewport content="width=device-width,initial-scale=1">
<style>button{font-size:1.4rem;padding:1rem 1.6rem;margin:.3rem}body{font-family:sans-serif;text-align:center}</style>
<h2>EnviroBot RC hardware test</h2>
<div><button onclick=s('FWD')>FWD</button></div>
<div><button onclick=s('LEFT')>LEFT</button><button onclick=s('STOP')>STOP</button><button onclick=s('RIGHT')>RIGHT</button></div>
<div><button onclick=s('BWD')>BWD</button></div>
<div><button onclick=s('SAMPLE_WATER')>WATER</button><button onclick=s('SAMPLE_SOIL')>SOIL</button></div>
<script>function s(c){fetch('/rc',{method:'POST',body:JSON.stringify({cmd:c})})}</script>)HTML";

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_MOTOR_L_FWD, OUTPUT); pinMode(PIN_MOTOR_L_BWD, OUTPUT);
  pinMode(PIN_MOTOR_R_FWD, OUTPUT); pinMode(PIN_MOTOR_R_BWD, OUTPUT);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_MOTOR_L_PWM, 5000, 8);
  ledcAttach(PIN_MOTOR_R_PWM, 5000, 8);
#else
  ledcSetup(0, 5000, 8); ledcAttachPin(PIN_MOTOR_L_PWM, 0);
  ledcSetup(1, 5000, 8); ledcAttachPin(PIN_MOTOR_R_PWM, 1);
#endif
  driveStop();

  ESP32PWM::allocateTimer(2); ESP32PWM::allocateTimer(3);
  srv.setPeriodHertz(50);
  srv.attach(PIN_SERVO, 500, 2400);
  srv.write(SERVO_NEUTRAL);

  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] AP IP: "); Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", PAGE); });
  server.on("/rc", HTTP_POST, handleRC);
  server.begin();
  Serial.println("RC test — join RobotAP, open http://192.168.4.1. Robot on blocks!");
}

void loop() {
  server.handleClient();
  // Dead-man: if drive cmds stop arriving (WiFi drop / phone sleep), halt.
  if (driving && millis() - lastDriveMs > 500) {
    driveStop(); driving = false;
    Serial.println("[RC] drive timeout — stopped");
  }
}
