#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "movement-sequences.h"

// ======================================================================
// CONFIGURATION
// ======================================================================

// ---- WiFi Access Point ----
const char* AP_SSID      = "HermitCrab";
const char* AP_PASS      = "12345678";   // NULL for open network
const bool  AP_HIDDEN    = false;
const int   AP_MAX_CONN  = 2;
IPAddress   AP_IP(192, 168, 0, 1);
IPAddress   AP_GATEWAY(192, 168, 0, 1);
IPAddress   AP_SUBNET(255, 255, 255, 0);

// ---- Hardware ----
const int OPERATING_FREQ   = 50;

// Index:                   0   1   2   3   4   5   6   7
// Label:                   R1  R2  L1  L2  R4  R3  L3  L4
const int servoPins[8]    = {13, 14, 22, 16, 17, 18, 19, 21};

// ---- MG90D Pulse Calibration ----
// Full travel: 500–2500 µs → 2000 µs span for 270°
// Trimming to center 180°: (45° / 270°) * 2000 µs ≈ 333 µs per side
const int MIN_PULSE = 833;   
const int MAX_PULSE = 2167;  

// ---- Tuning ----
int frameDelay        = 100;  // ms between gait frames
int motorCurrentDelay = 5;    // ms between servo writes (prevents brownout)

// ---- Subtrim ----
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// ======================================================================
// STATE MACHINE
// ======================================================================

MovementState currentCommand = STATE_IDLE;

struct CommandEntry {
  const char*   str;
  MovementState state;
};

static const CommandEntry COMMAND_TABLE[] = {
  { "stop",   STATE_IDLE     },
  { "rest",   STATE_REST     },
  { "stand",  STATE_STAND    },
  { "walk",   STATE_FORWARD  },
  { "back",   STATE_BACKWARD },
  { "left",   STATE_LEFT     },
  { "right",  STATE_RIGHT    },
  { "wave",   STATE_WAVE     },
  { "dance",  STATE_DANCE    },
  { "swim",   STATE_SWIM     },
  { "point",  STATE_POINT    },
  { "pushup", STATE_PUSHUP   },
  { "bow",    STATE_BOW      },
  { "cute",   STATE_CUTE     },
  { "freaky", STATE_FREAKY   },
  { "worm",   STATE_WORM     },
  { "shake",  STATE_SHAKE    },
  { "shrug",  STATE_SHRUG    },
  { "dead",   STATE_DEAD     },
  { "crab",   STATE_CRAB     },
};
static const int COMMAND_TABLE_SIZE = sizeof(COMMAND_TABLE) / sizeof(COMMAND_TABLE[0]);

// ======================================================================
// HARDWARE
// ======================================================================

Servo     servos[8];
WebServer server(80);

// ======================================================================
// SERIAL CLI STATE
// ======================================================================

char commandBuffer[64];
byte bufferPos = 0;

// ======================================================================
// WEB INTERFACE (served from flash)
// ======================================================================

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Hermit Crab</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #111;
      color: #eee;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 20px 12px;
      gap: 18px;
      min-height: 100vh;
    }
    h1 { font-size: 22px; color: #fff; letter-spacing: 1px; }

    .status-bar {
      font-size: 13px;
      color: #aaa;
      background: #1e1e1e;
      padding: 6px 16px;
      border-radius: 20px;
      border: 1px solid #333;
    }
    .status-bar span { color: #4fc; font-weight: 600; }

    .card {
      background: #1a1a1a;
      border: 1px solid #2e2e2e;
      border-radius: 14px;
      padding: 16px;
      width: 100%;
      max-width: 420px;
    }
    .card-title {
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 1.5px;
      color: #666;
      margin-bottom: 12px;
    }

    /* D-pad */
    .dpad {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      grid-template-rows: repeat(2, 1fr);
      gap: 10px;
      max-width: 240px;
      margin: 0 auto 12px;
    }
    .dpad button {
      font-size: 26px;
      aspect-ratio: 1;
      background: #2a2a2a;
      border: 1px solid #3a3a3a;
      border-radius: 10px;
      color: #fff;
      cursor: pointer;
      transition: background 0.1s, transform 0.1s;
      touch-action: manipulation;
    }
    .dpad button:active { background: #3a5a3a; transform: scale(0.94); }
    .spacer { visibility: hidden; }

    /* Stop button */
    .btn-stop {
      width: 100%;
      padding: 14px;
      font-size: 16px;
      font-weight: 700;
      letter-spacing: 2px;
      background: #5a1a1a;
      border: 1px solid #a33;
      border-radius: 10px;
      color: #fff;
      cursor: pointer;
      transition: background 0.1s;
      touch-action: manipulation;
    }
    .btn-stop:active { background: #7a2a2a; }

    /* Pose grid */
    .pose-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
    }
    .btn-pose {
      padding: 10px 6px;
      font-size: 13px;
      background: #1f3a2a;
      border: 1px solid #2a5a3a;
      border-radius: 9px;
      color: #cec;
      cursor: pointer;
      transition: background 0.1s;
      touch-action: manipulation;
    }
    .btn-pose:active { background: #2a5a3a; }

    /* Direct motor */
    .motor-row {
      display: flex;
      align-items: center;
      gap: 10px;
      margin-top: 8px;
    }
    .motor-row label { font-size: 12px; color: #888; width: 60px; flex-shrink: 0; }
    .motor-row input[type=range] {
      flex: 1;
      accent-color: #4fc;
    }
    .motor-row .motor-val { font-size: 12px; color: #aaa; width: 36px; text-align: right; }

    footer { font-size: 11px; color: #444; margin-top: 4px; }
  </style>
</head>
<body>

<h1>🦀 Hermit Crab</h1>

<div class="status-bar">Command: <span id="status">idle</span></div>

<!-- Movement -->
<div class="card">
  <div class="card-title">Movement</div>
  <div class="dpad">
    <div class="spacer"></div>
    <button onmousedown="move('walk')"  onmouseup="sendCmd('stop')"
            ontouchstart="move('walk')" ontouchend="sendCmd('stop')">▲</button>
    <div class="spacer"></div>
    <button onmousedown="move('left')"  onmouseup="sendCmd('stop')"
            ontouchstart="move('left')" ontouchend="sendCmd('stop')">◀</button>
    <button onmousedown="move('back')"  onmouseup="sendCmd('stop')"
            ontouchstart="move('back')" ontouchend="sendCmd('stop')">▼</button>
    <button onmousedown="move('right')" onmouseup="sendCmd('stop')"
            ontouchstart="move('right')" ontouchend="sendCmd('stop')">▶</button>
  </div>
  <button class="btn-stop" onclick="sendCmd('stop')">■ STOP</button>
</div>

<!-- Poses -->
<div class="card">
  <div class="card-title">Poses &amp; Animations</div>
  <div class="pose-grid">
    <button class="btn-pose" onclick="sendCmd('rest')">Rest</button>
    <button class="btn-pose" onclick="sendCmd('stand')">Stand</button>
    <button class="btn-pose" onclick="sendCmd('wave')">Wave</button>
    <button class="btn-pose" onclick="sendCmd('dance')">Dance</button>
    <button class="btn-pose" onclick="sendCmd('swim')">Swim</button>
    <button class="btn-pose" onclick="sendCmd('point')">Point</button>
    <button class="btn-pose" onclick="sendCmd('pushup')">Pushup</button>
    <button class="btn-pose" onclick="sendCmd('bow')">Bow</button>
    <button class="btn-pose" onclick="sendCmd('cute')">Cute</button>
    <button class="btn-pose" onclick="sendCmd('freaky')">Freaky</button>
    <button class="btn-pose" onclick="sendCmd('worm')">Worm</button>
    <button class="btn-pose" onclick="sendCmd('shake')">Shake</button>
    <button class="btn-pose" onclick="sendCmd('shrug')">Shrug</button>
    <button class="btn-pose" onclick="sendCmd('dead')">Dead</button>
    <button class="btn-pose" onclick="sendCmd('crab')">Crab</button>
  </div>
</div>

<!-- Direct Motor Control -->
<div class="card">
  <div class="card-title">Direct Motor Control</div>
  <div id="motorSliders"></div>
</div>

<footer>192.168.0.1 · Serial CLI also active</footer>

<script>
const LABELS = ['R1','R2','L1','L2','R4','R3','L3','L4'];

// Build motor sliders
const container = document.getElementById('motorSliders');
LABELS.forEach((label, i) => {
  const row = document.createElement('div');
  row.className = 'motor-row';
  row.innerHTML = `
    <label>S${i} ${label}</label>
    <input type="range" min="0" max="180" value="90"
           oninput="motorMove(${i}, this.value, this.nextElementSibling)">
    <span class="motor-val">90°</span>`;
  container.appendChild(row);
});

let moveInterval = null;

function move(cmd) {
  sendCmd(cmd);
  // Re-send every 200 ms while held — keeps state alive
  // if a future watchdog is added
  moveInterval = setInterval(() => sendCmd(cmd), 200);
}

function sendCmd(cmd) {
  if (moveInterval && cmd === 'stop') {
    clearInterval(moveInterval);
    moveInterval = null;
  }
  fetch('/cmd?c=' + cmd)
    .then(r => r.text())
    .then(t => document.getElementById('status').textContent = t)
    .catch(() => document.getElementById('status').textContent = 'error');
}

function motorMove(idx, val, display) {
  display.textContent = val + '°';
  fetch(`/motor?i=${idx}&a=${val}`).catch(()=>{});
}
</script>
</body>
</html>
)rawliteral";

// ======================================================================
// PROTOTYPES
// ======================================================================

void setServoAngle(uint8_t channel, int angle);
bool pressingCheck(MovementState expectedState, int ms);
bool parseMovementCommand(const char* str, MovementState& out);
void handleSubtrimCommand(const char* buf);
void checkSerial();
bool interruptibleDelay(int ms);
void handleClient();   

void setupWiFi();
void setupWebServer();
void handleRoot();
void handleCmd();
void handleMotor();
void handleStatus();

// ======================================================================
// SETUP
// ======================================================================

void setup() {
  Serial.begin(115200);
  delay(100); // ESP32 WROOM: skip while(!Serial), UART bridge doesn't assert

  Serial.println("Hermit Crab booting up...");

  // Reserve all 4 LEDC hardware timers for stable PWM
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Servo init with 180° window inside 270° servo pulse range
  for (int i = 0; i < 8; i++) {
    servos[i].setPeriodHertz(OPERATING_FREQ);
    servos[i].attach(servoPins[i], MIN_PULSE, MAX_PULSE);
  }
  delay(10);

  setupWiFi();
  setupWebServer();

  Serial.println("-----------------------------------");
  Serial.println("    HERMIT CRAB WIFI + SERIAL      ");
  Serial.println("-----------------------------------");
  Serial.print  ("  Web UI: http://");
  Serial.println(AP_IP);
  Serial.println();
  Serial.println("Serial Commands:");
  Serial.println("  rest, stand, walk, back, left, right");
  Serial.println("  wave, dance, swim, point, pushup");
  Serial.println("  bow, cute, freaky, worm, shake");
  Serial.println("  shrug, dead, crab, stop");
  Serial.println();
  Serial.println("  subtrim               (show values)");
  Serial.println("  subtrim <m> <v>       (set motor m)");
  Serial.println("  subtrim reset / save");
  Serial.println();
  Serial.println("  <m> <angle>           (direct motor)");
  Serial.println("  all <angle>           (all motors)");
  Serial.println("-----------------------------------");
}

// ======================================================================
// LOOP
// ======================================================================

void loop() {

  server.handleClient();

  // ---- State Machine Dispatch ----
  switch (currentCommand) {
    case STATE_IDLE:                         break;
    case STATE_STAND:    runStandPose();     break;
    case STATE_REST:     runRestPose();      break;
    case STATE_FORWARD:  runWalkPose();      break;
    case STATE_BACKWARD: runWalkBackward();  break;
    case STATE_LEFT:     runTurnLeft();      break;
    case STATE_RIGHT:    runTurnRight();     break;
    case STATE_WAVE:     runWavePose();      break;
    case STATE_DANCE:    runDancePose();     break;
    case STATE_SWIM:     runSwimPose();      break;
    case STATE_POINT:    runPointPose();     break;
    case STATE_PUSHUP:   runPushupPose();    break;
    case STATE_BOW:      runBowPose();       break;
    case STATE_CUTE:     runCutePose();      break;
    case STATE_FREAKY:   runFreakyPose();    break;
    case STATE_WORM:     runWormPose();      break;
    case STATE_SHAKE:    runShakePose();     break;
    case STATE_SHRUG:    runShrugPose();     break;
    case STATE_DEAD:     runDeadPose();      break;
    case STATE_CRAB:     runCrabPose();      break;
    default:                                 break;
  }

  // ---- Serial CLI ----
  checkSerial();
}

// ======================================================================
// WIFI SETUP
// ======================================================================

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_SUBNET);
  WiFi.softAP(AP_SSID, AP_PASS, 1, AP_HIDDEN, AP_MAX_CONN);
  Serial.print("AP started — SSID: ");
  Serial.print(AP_SSID);
  Serial.print("  IP: ");
  Serial.println(WiFi.softAPIP());
}

// ======================================================================
// WEB SERVER SETUP & HANDLERS
// ======================================================================

void setupWebServer() {
  server.on("/",       handleRoot);
  server.on("/cmd",    handleCmd);
  server.on("/motor",  handleMotor);
  server.on("/status", handleStatus);
  server.onNotFound(handleRoot);
  server.begin();
  Serial.println("HTTP server started on port 80");
}

// GET /
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

// GET /cmd?c=<command>
// Accepts any string from COMMAND_TABLE.
// Movement states (walk/back/left/right) are held — caller re-sends while held.
// Pose states run once and self-transition to STATE_IDLE via the .cpp functions.
void handleCmd() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "missing arg");
    return;
  }
  String cmdStr = server.arg("c");
  MovementState parsed;
  if (parseMovementCommand(cmdStr.c_str(), parsed)) {
    currentCommand = parsed;
    server.send(200, "text/plain", cmdStr);
  } else {
    server.send(400, "text/plain", "unknown");
  }
}

// GET /motor?i=<0-7>&a=<0-180>
void handleMotor() {
  if (!server.hasArg("i") || !server.hasArg("a")) {
    server.send(400, "text/plain", "missing args");
    return;
  }
  int idx   = server.arg("i").toInt();
  int angle = server.arg("a").toInt();
  if (idx < 0 || idx > 7 || angle < 0 || angle > 180) {
    server.send(400, "text/plain", "out of range");
    return;
  }
  setServoAngle(idx, angle);
  server.send(200, "text/plain", "ok");
}

// GET /status — lightweight JSON for future scripting / Python use
void handleStatus() {
  String json = "{\"state\":";
  json += String((int)currentCommand);
  json += ",\"frameDelay\":";
  json += String(frameDelay);
  json += "}";
  server.send(200, "application/json", json);
}

// ======================================================================
// HELPER FUNCTIONS
// ======================================================================

// Walks the lookup table and maps an input string to a MovementState.
bool parseMovementCommand(const char* str, MovementState& out) {
  for (int i = 0; i < COMMAND_TABLE_SIZE; i++) {
    if (strcmp(str, COMMAND_TABLE[i].str) == 0) {
      out = COMMAND_TABLE[i].state;
      return true;
    }
  }
  return false;
}

// Handles all "subtrim ..." variants.
void handleSubtrimCommand(const char* buf) {
  if (strcmp(buf, "subtrim save") == 0) {
    Serial.println("Copy this into the code:");
    Serial.print("int8_t servoSubtrim[8] = {");
    for (int i = 0; i < 8; i++) {
      Serial.print(servoSubtrim[i]);
      if (i < 7) Serial.print(", ");
    }
    Serial.println("};");
  } else if (strcmp(buf, "subtrim reset") == 0) {
    for (int i = 0; i < 8; i++) servoSubtrim[i] = 0;
    Serial.println("All subtrim values reset to 0");
  } else if (strncmp(buf, "subtrim ", 8) == 0) {
    int trimMotor, trimValue;
    if (sscanf(buf + 8, "%d %d", &trimMotor, &trimValue) == 2) {
      if (trimMotor >= 0 && trimMotor < 8 && trimValue >= -90 && trimValue <= 90) {
        servoSubtrim[trimMotor] = (int8_t)trimValue;
        Serial.print("Motor "); Serial.print(trimMotor);
        Serial.print(" subtrim -> ");
        if (trimValue >= 0) Serial.print("+");
        Serial.println(trimValue);
      } else {
        Serial.println("Invalid — motor: 0-7, value: -90 to +90");
      }
    }
  } else {
    Serial.println("Subtrim values:");
    for (int i = 0; i < 8; i++) {
      Serial.print("  Motor "); Serial.print(i); Serial.print(": ");
      if (servoSubtrim[i] >= 0) Serial.print("+");
      Serial.println(servoSubtrim[i]);
    }
  }
}

// Writes a servo angle, applying subtrim offset and clamping.
void setServoAngle(uint8_t channel, int angle) {
  if (channel >= 8) return;
  int adjusted = constrain(angle + servoSubtrim[channel], 0, 180);
  int pulseUs  = map(adjusted, 0, 180, MIN_PULSE, MAX_PULSE);
  servos[channel].writeMicroseconds(pulseUs);
  delay(motorCurrentDelay);
}

// Holds execution for `ms` ms while still in expectedState.
// Pumps the web server and serial each iteration.
bool pressingCheck(MovementState expectedState, int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();    
    checkSerial();            
    if (currentCommand != expectedState) {
      runStandPose();
      return false;         
    }
    yield(); // Momentarily handle background system tasks before returning to loop
             // NOTE: need to feed the system wastchdog to keep the MCU from constantly resetting
  }
  return true;
}

// Reads and processes any pending serial bytes mid-animation.
void checkSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufferPos > 0) {
        commandBuffer[bufferPos] = '\0';
        bufferPos = 0;

        MovementState parsed;
        if (parseMovementCommand(commandBuffer, parsed)) {
          currentCommand = parsed;
          Serial.print("Command: "); Serial.println(commandBuffer);
        } else if (strncmp(commandBuffer, "subtrim", 7) == 0) {
          handleSubtrimCommand(commandBuffer);
        } else if (strncmp(commandBuffer, "all ", 4) == 0) {
          int angle;
          if (sscanf(commandBuffer + 4, "%d", &angle) == 1) {
            for (int i = 0; i < 8; i++) setServoAngle(i, angle);
            Serial.print("All servos -> "); Serial.println(angle);
          }
        } else {
          int motorNum, angle;
          if (sscanf(commandBuffer, "%d %d", &motorNum, &angle) == 2) {
            if (motorNum >= 0 && motorNum < 8) {
              setServoAngle(motorNum, angle);
              Serial.print("Servo "); Serial.print(motorNum);
              Serial.print(" -> "); Serial.println(angle);
            } else {
              Serial.println("Invalid motor number (0-7)");
            }
          } else {
            Serial.print("Unknown command: "); Serial.println(commandBuffer);
          }
        }
      }
    } else if (bufferPos < sizeof(commandBuffer) - 1) {
      commandBuffer[bufferPos++] = c;
    }
  }
}

// Drop-in replacement for delay() inside animated poses.
// Returns false if currentCommand changed mid-wait.
bool interruptibleDelay(int ms) {
  MovementState stateAtStart = currentCommand;
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    checkSerial();
    if (currentCommand != stateAtStart) return false;
    yield();
  }
  return true;
}
