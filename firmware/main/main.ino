#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "movement-sequences.h"
#include "face-bitmaps.h"
#include "web.h"

// ======================================================================
// TYPES
// ======================================================================

// A command in flight through the queue: the requested state plus whether
// the caller intends to hold it (gaits re-send; the watchdog watches these).
struct Command {
  MovementState state;
  bool          held;
};

// One row of the string -> state lookup used by the serial CLI and web /cmd.
struct CommandEntry {
  const char*   str;
  MovementState state;
};

// One row of the state -> face bitmap lookup used on state transitions.
struct FaceEntry {
  MovementState        state;
  const unsigned char* bitmap;
};

// ======================================================================
// CONFIGURATION
// ======================================================================
// Compile-time constants only. Anything mutated at runtime lives in STATE.

// ---- WiFi Access Point ----
const char*    AP_SSID         = "HermitCrab";
const char*    AP_PASS         = "12345678";
const bool     AP_HIDDEN       = false;
const uint8_t  AP_MAX_CONN     = 2;
const uint32_t HOLD_TIMEOUT_MS = 600;  // Held-gait watchdog window (ms)

// ---- Servos ----
const uint8_t  OPERATING_FREQ = 50;    // Servo PWM frequency (Hz)
const uint16_t MIN_PULSE      = 833;   // 0deg   pulse width (us) - trimmed from 500
const uint16_t MAX_PULSE      = 2167;  // 180deg pulse width (us) - trimmed from 2500

const int INTERP_TICK_MS = 20;         // 50 Hz interpolator tick (1000/OPERATING_FREQ)
const int MAX_SPEED      = 600;        // Max angular velocity (deg/s)
                                       // (lower = smoother, higher = snappier)

// Label:                     R1  R2  L1  L2  R4  R3  L3  L4
const uint8_t servoPins[8] = {13, 14, 23, 16, 17, 18, 19, 33};

// ---- OLED Face Display ----
const uint16_t OLED_WIDTH    = 128;
const uint16_t OLED_HEIGHT   = 64;
const uint8_t  OLED_I2C_ADDR = 0x3C;
const int8_t   OLED_RESET    = -1;

const uint8_t I2C_SDA = 21;
const uint8_t I2C_SCL = 22;

// ======================================================================
// LOOKUP TABLES
// ======================================================================

// String -> state. Drives both the serial CLI and the web /cmd endpoint.
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

// State -> face bitmap. nullptr entries draw no face (e.g. IDLE).
static const FaceEntry FACE_TABLE[] = {
  { STATE_IDLE,     nullptr           },
  { STATE_STAND,    epd_bitmap_stand  },
  { STATE_REST,     epd_bitmap_rest   },
  { STATE_FORWARD,  epd_bitmap_walk   },
  { STATE_BACKWARD, epd_bitmap_walk   },
  { STATE_LEFT,     epd_bitmap_walk   },
  { STATE_RIGHT,    epd_bitmap_walk   },
  { STATE_WAVE,     epd_bitmap_wave   },
  { STATE_DANCE,    epd_bitmap_dance  },
  { STATE_SWIM,     epd_bitmap_swim   },
  { STATE_POINT,    epd_bitmap_point  },
  { STATE_PUSHUP,   epd_bitmap_pushup },
  { STATE_BOW,      epd_bitmap_bow    },
  { STATE_CUTE,     epd_bitmap_cute   },
  { STATE_FREAKY,   epd_bitmap_freaky },
  { STATE_WORM,     epd_bitmap_worm   },
  { STATE_SHAKE,    epd_bitmap_shake  },
  { STATE_SHRUG,    epd_bitmap_shrug  },
  { STATE_DEAD,     epd_bitmap_dead   },
  { STATE_CRAB,     epd_bitmap_crab   },
};
static const int FACE_TABLE_SIZE = sizeof(FACE_TABLE) / sizeof(FACE_TABLE[0]);

// ======================================================================
// STATE
// ======================================================================

// ---- WiFi ----
IPAddress AP_IP(192, 168, 0, 1);
IPAddress AP_GATEWAY(192, 168, 0, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);
WebServer server(80);

// ---- Servos ----
Servo          servos[8];
float          servoPos[8];              // Where each servo is now (deg)
volatile float servoTarget[8];           // Where each servo is headed (deg)
int8_t         servoSubtrim[8] = {0};    // Per-servo trim (deg), runtime-tunable
                                         // via the "subtrim" serial command

// ---- Movement ----
volatile MovementState currentCommand = STATE_IDLE;
QueueHandle_t          commandQueue;

volatile bool          holdWatchdog      = false;  // Armed by a held command
volatile unsigned long lastHoldRefreshMs = 0;

// ---- OLED Face Display ----
Adafruit_SSD1306  display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
SemaphoreHandle_t displayMutex;

volatile MovementState lastDisplayedState = STATE_IDLE;

// ---- Serial CLI ----
char commandBuffer[64];
byte bufferPos = 0;

// ======================================================================
// PROTOTYPES
// ======================================================================

// ---- Tasks ----
void webServerTask(void* param);
void serialTask(void* param);
void motionTask(void* param);
void interpolatorTask(void* param);

// ---- Servo Write Path ----
void setServoTarget(uint8_t channel, int angle);
void setServoImmediate(uint8_t channel, int angle);
void writeServoHardware(uint8_t channel, float pos);

// ---- Command Queue + Input ----
void        submitCommand(MovementState state, bool held);
bool        pollCommandQueue();
bool        pressingCheck(MovementState expectedState, int ms);
bool        interruptibleDelay(int ms);
bool        parseMovementCommand(const char* str, MovementState& out);
const char* commandName(MovementState state);
void        handleSubtrimCommand(const char* buf);
void        checkSerial();

// ---- Network + Web ----
void setupWiFi();
void setupWebServer();
void handleRoot();
void handleCmd();
void handleMotor();
void handleStatus();

// ---- OLED ----
void setupOLED();
void drawFace(const unsigned char* bitmap);
void showFace(MovementState state);

// ======================================================================
// SETUP
// ======================================================================

void setup() {

  // FreeRTOS primitives
  displayMutex = xSemaphoreCreateMutex();
  commandQueue = xQueueCreate(1, sizeof(Command));

  // Serial communication
  Serial.begin(115200);
  delay(100);

  Serial.println("Hermit Crab booting up...");

  // I2C + OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  setupOLED();
  drawFace(epd_bitmap_idle);

  // Reserve all 4 LEDC hardware timers for stable PWM
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Servo init with the 180deg window inside the 270deg servo pulse range
  for (int i = 0; i < 8; i++) {
    servos[i].setPeriodHertz(OPERATING_FREQ);
    servos[i].attach(servoPins[i], MIN_PULSE, MAX_PULSE);
  }
  delay(10);

  // Seed the interpolator: snap every servo to rest (90°) once
  for (int i = 0; i < 8; i++) {
    setServoImmediate(i, 90);
    delay(5);
  }

  // WiFi access point + HTTP server
  setupWiFi();
  setupWebServer();

  // Core 0 - I/O tasks
  xTaskCreatePinnedToCore(webServerTask, "WebServer", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(serialTask, "Serial", 2048, NULL, 1, NULL, 0);

  // Core 1 - movement tasks
  xTaskCreatePinnedToCore(interpolatorTask, "Interp", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(motionTask, "Motion", 8192, NULL, 2, NULL, 1);

  // Startup message
  Serial.println("-----------------------------------");
  Serial.println("    HERMIT CRAB WIFI + SERIAL      ");
  Serial.println("-----------------------------------");
  Serial.print  (" Web UI: http://");
  Serial.println(AP_IP);
  Serial.println();
  Serial.print  ("Interpolator: 50 Hz tick, max ");
  Serial.print  (MAX_SPEED);
  Serial.println(" deg/s");
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
  vTaskDelete(NULL); // Arduino loop unused; all work runs in FreeRTOS tasks
}

// ======================================================================
// TASKS
// ======================================================================

// Core 0: services the web server client poll loop.
void webServerTask(void* param) {
  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5)); // Yield for 5ms between polls
  }
}

// Core 0: drains the serial CLI.
void serialTask(void* param) {
  for (;;) {
    checkSerial();
    vTaskDelay(pdMS_TO_TICKS(10)); // Yield for 10ms between polls
  }
}

// Core 1: ramps every servo toward its target at MAX_SPEED deg/s instead of
// snapping — the only task allowed to write servo hardware
void interpolatorTask(void* param) {
  for (;;) {
    float maxStep = (float)MAX_SPEED * INTERP_TICK_MS / 1000.0f;  // Max degrees movable per tick

    // Advance all 8 servos towards target
    for (int i = 0; i < 8; i++) {
      float target = servoTarget[i];
      float diff   = target - servoPos[i];
      if (diff != 0.0f) {
        if      (diff >  maxStep) servoPos[i] += maxStep;
        else if (diff < -maxStep) servoPos[i] -= maxStep;
        else                      servoPos[i]  = target;  // Arrived
        writeServoHardware(i, servoPos[i]);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(INTERP_TICK_MS));
  }
}

// Core 1: the movement brain. Pulls commands off the queue, drives the face,
// and dispatches the current state to its pose/gait sequence.
void motionTask(void* param) {
  for (;;) {
    
    pollCommandQueue();

    // On state transition: update face + announce over serial
    if (currentCommand != lastDisplayedState) {
      showFace(currentCommand);
      Serial.print("State: ");
      Serial.println(commandName(currentCommand));
      lastDisplayedState = currentCommand;
    }

    // Dispatch: state -> sequence table lookup
    const Sequence* seq = lookupSequence(currentCommand);
    if (seq != nullptr) {
      if (seq->isGait) {
        // Abort-to-idle from gait recovers to stand posture
        if (!playGaitCycle(*seq, currentCommand) && currentCommand == STATE_IDLE) {
          drawFace(epd_bitmap_stand);
        }
      } 
      else if (playPose(*seq)) {
        if (seq->sticky) {
          // Sticky pose (STAND/REST/DEAD): hold state
          while (!pollCommandQueue()) vTaskDelay(pdMS_TO_TICKS(10));
        } 
        else {
          // Self-transition to IDLE only on full completion
          currentCommand = STATE_IDLE;
          if (seq->standAtEnd) drawFace(epd_bitmap_stand);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));  // Yield briefly when idle
  }
}

// ======================================================================
// OLED FACE DISPLAY
// ======================================================================

void setupOLED() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println("SSD1306 not found — check wiring and I2C address");
    return; // Robot still runs without the display
  }
  display.clearDisplay();
  display.display();
  Serial.println("OLED initialized");
}

// Looks up the face for a state and draws it. No-op if the state has no face.
void showFace(MovementState state) {
  const unsigned char* bitmap = nullptr;
  for (int i = 0; i < FACE_TABLE_SIZE; i++) {
    if (FACE_TABLE[i].state == state) {
      bitmap = FACE_TABLE[i].bitmap;
      break;
    }
  }
  if (bitmap == nullptr) return;
  drawFace(bitmap);
}

// Display a face bitmap (mutex-guarded).
void drawFace(const unsigned char* bitmap) {
  xSemaphoreTake(displayMutex, portMAX_DELAY);
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmap, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
  display.display();
  xSemaphoreGive(displayMutex);
}

// ======================================================================
// WIFI SETUP
// ======================================================================

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
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

// GET /cmd?c=<command> — accepts any COMMAND_TABLE string. Gaits are marked
// held (caller re-sends); poses run once and self-transition to STATE_IDLE.
void handleCmd() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "missing arg");
    return;
  }
  String cmdStr = server.arg("c");
  MovementState parsed;
  if (parseMovementCommand(cmdStr.c_str(), parsed)) {
    const Sequence* seq = lookupSequence(parsed);
    submitCommand(parsed, seq != nullptr && seq->isGait);
    server.send(200, "text/plain", cmdStr);
  }
  else {
    server.send(400, "text/plain", "unknown");
  }
}

// GET /motor?i=<0-7>&a=<0-180> — direct single-servo control.
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
  submitCommand(STATE_IDLE, false);
  setServoTarget(idx, angle);
  server.send(200, "text/plain", "ok");
}

// GET /status — lightweight JSON snapshot (state + live servo positions).
// Intended as a hook for future Python scripting.
void handleStatus() {
  MovementState state = currentCommand;
  String json = "{\"state\":";
  json += String((int)state);
  json += ",\"name\":\"";
  json += commandName(state);
  json += "\",\"pos\":[";
  for (int i = 0; i < 8; i++) {
    json += String((int)(servoPos[i] + 0.5f));
    if (i < 7) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// ======================================================================
// HELPER FUNCTIONS
// ======================================================================

// String -> state via COMMAND_TABLE. Returns false on no match.
bool parseMovementCommand(const char* str, MovementState& out) {
  for (int i = 0; i < COMMAND_TABLE_SIZE; i++) {
    if (strcmp(str, COMMAND_TABLE[i].str) == 0) {
      out = COMMAND_TABLE[i].state;
      return true;
    }
  }
  return false;
}

// Reverse lookup: state → command string, for transition announcements.
const char* commandName(MovementState state) {
  for (int i = 0; i < COMMAND_TABLE_SIZE; i++) {
    if (COMMAND_TABLE[i].state == state) return COMMAND_TABLE[i].str;
  }
  return "?";
}

// Handles all "subtrim ..." command variants: save / reset / set / show.
void handleSubtrimCommand(const char* buf) {
  if (strcmp(buf, "subtrim save") == 0) {
    Serial.println("Copy this into the code:");
    Serial.print("int8_t servoSubtrim[8] = {");
    for (int i = 0; i < 8; i++) {
      Serial.print(servoSubtrim[i]);
      if (i < 7) Serial.print(", ");
    }
    Serial.println("};");
  } 
  else if (strcmp(buf, "subtrim reset") == 0) {
    for (int i = 0; i < 8; i++) servoSubtrim[i] = 0;
    Serial.println("All subtrim values reset to 0");
  } 
  else if (strncmp(buf, "subtrim ", 8) == 0) {
    int trimMotor, trimValue;
    if (sscanf(buf + 8, "%d %d", &trimMotor, &trimValue) == 2) {
      if (trimMotor >= 0 && trimMotor < 8 && trimValue >= -90 && trimValue <= 90) {
        servoSubtrim[trimMotor] = (int8_t)trimValue;
        Serial.print("Motor "); Serial.print(trimMotor);
        Serial.print(" subtrim -> ");
        if (trimValue >= 0) Serial.print("+");
        Serial.println(trimValue);
      } 
      else {
        Serial.println("Invalid — motor: 0-7, value: -90 to +90");
      }
    }
  } 
  else {
    Serial.println("Subtrim values:");
    for (int i = 0; i < 8; i++) {
      Serial.print("  Motor "); Serial.print(i); Serial.print(": ");
      if (servoSubtrim[i] >= 0) Serial.print("+");
      Serial.println(servoSubtrim[i]);
    }
  }
}

// ---- Servo write path ----
// setServoTarget(): normal path, safe from any task — interpolator moves it there.
// setServoImmediate(): boot-only snap bypassing interpolation (single-owner rule
// after tasks start — only interpolatorTask may touch servo hardware).
// writeServoHardware(): applies subtrim at write time so trim changes take
// effect immediately without disturbing targets.

// Sets where a servo should head (interpolator does the move). Safe from any task.
void setServoTarget(uint8_t channel, int angle) {
  if (channel >= 8) return;
  servoTarget[channel] = (float)constrain(angle, 0, 180);
}

// Boot-only snap that bypasses interpolation. Not safe once tasks are running.
void setServoImmediate(uint8_t channel, int angle) {
  if (channel >= 8) return;
  float a = (float)constrain(angle, 0, 180);
  servoPos[channel]    = a;
  servoTarget[channel] = a;
  writeServoHardware(channel, a);
}

// Maps angle + subtrim to a pulse and drives the servo.
void writeServoHardware(uint8_t channel, float pos) {
  int adjusted = constrain((int)(pos + 0.5f) + servoSubtrim[channel], 0, 180);
  int pulseUs  = map(adjusted, 0, 180, MIN_PULSE, MAX_PULSE);
  servos[channel].writeMicroseconds(pulseUs);
}

// Reads and processes any pending serial bytes mid-animation.
void checkSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    // Terminator: parse the accumulated line
    if (c == '\n' || c == '\r') {
      if (bufferPos > 0) {
        commandBuffer[bufferPos] = '\0';
        bufferPos = 0;

        // Parse command
        MovementState parsed;
        if (parseMovementCommand(commandBuffer, parsed)) {
          submitCommand(parsed, false);
          Serial.print("Command: "); Serial.println(commandBuffer);
        } 
        else if (strncmp(commandBuffer, "subtrim", 7) == 0) {
          handleSubtrimCommand(commandBuffer);
        } 
        else if (strncmp(commandBuffer, "all ", 4) == 0) {
          int angle;
          if (sscanf(commandBuffer + 4, "%d", &angle) == 1) {
            for (int i = 0; i < 8; i++) setServoTarget(i, angle);
            Serial.print("All servos -> "); Serial.println(angle);
          }
        } 
        else {
          int motorNum, angle;
          if (sscanf(commandBuffer, "%d %d", &motorNum, &angle) == 2) {
            if (motorNum >= 0 && motorNum < 8) {
              setServoTarget(motorNum, angle);
              Serial.print("Servo "); Serial.print(motorNum);
              Serial.print(" -> "); Serial.println(angle);
            } else {
              Serial.println("Invalid motor number (0-7)");
            }
          } 
          else {
            Serial.print("Unknown command: "); Serial.println(commandBuffer);
          }
        }
      }
    } 
    // Otherwise accumulate, if buffer space remains
    else if (bufferPos < sizeof(commandBuffer) - 1) {
      commandBuffer[bufferPos++] = c;
    }
  }
}

// Enqueues a command (overwrite: the newest command always wins). 
// Safe to call from any task.
void submitCommand(MovementState state, bool held) {
  Command cmd = { state, held };
  xQueueOverwrite(commandQueue, &cmd);
}

// Dequeues one command if present, updating currentCommand + watchdog state.
// Returns true if a command was consumed. Call only from motionTask context.
bool pollCommandQueue() {
  Command incoming;
  if (xQueueReceive(commandQueue, &incoming, 0) != pdTRUE) return false;
  currentCommand = incoming.state;
  holdWatchdog   = incoming.held;
  if (incoming.held) lastHoldRefreshMs = millis();
  return true;
}

// Interruptible sleep for animated poses (wave, dance, etc.). Returns true if
// the full duration elapsed, false if currentCommand changed (poll to abort).
bool interruptibleDelay(int ms) {
  MovementState stateAtStart = currentCommand;
  unsigned long start = millis();
  while (millis() - start < ms) {
    pollCommandQueue();
    if (currentCommand != stateAtStart) return false; // Any change aborts
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  return true;
}

// Hold check for continuous gaits (walk, turn, etc.). Returns true if the robot
// stayed in expectedState for the full duration; false if the state changed OR
// the held-gait watchdog expired.
bool pressingCheck(MovementState expectedState, int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    pollCommandQueue();
    // Watchdog: held gait, but no refresh within the timeout -> stop
    if (holdWatchdog && millis() - lastHoldRefreshMs > HOLD_TIMEOUT_MS) {
      holdWatchdog   = false;
      currentCommand = STATE_IDLE;
      Serial.println("Hold watchdog: refresh lost, stopping");
      return false;
    }
    if (currentCommand != expectedState) return false; // Left expected state

    vTaskDelay(pdMS_TO_TICKS(1));
  }
  return true;
}
