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
// CONFIGURATION
// ======================================================================

// ---- WiFi Access Point ----
const char* AP_SSID       = "HermitCrab";
const char* AP_PASS       = "12345678";   // NULL for open network
const bool  AP_HIDDEN     = false;
const uint8_t AP_MAX_CONN = 2;
IPAddress AP_IP(192, 168, 0, 1);
IPAddress AP_GATEWAY(192, 168, 0, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);
WebServer server(80); // Default newtork port for regular, unencrypted HTTP web traffic

// ---- Hardware ----
const uint8_t OPERATING_FREQ = 50;

const uint16_t OLED_WIDTH   = 128;
const uint16_t OLED_HEIGHT  = 64;
const uint8_t OLED_I2C_ADDR = 0x3C;
const int8_t OLED_RESET     = -1;  // -1 = share Arduino reset pin

const uint8_t I2C_SDA = 21;
const uint8_t I2C_SCL = 22;

// Index:                     0   1   2   3   4   5   6   7
// Label:                     R1  R2  L1  L2  R4  R3  L3  L4
const uint8_t servoPins[8] = {13, 14, 23, 16, 17, 18, 19, 33};

// ---- MG90D Pulse Calibration ----
// Full travel: 500–2500 µs → 2000 µs span for 270°
// Trimming to center 180°: (45° / 270°) * 2000 µs ≈ 333 µs per side
const uint16_t MIN_PULSE = 833;   
const uint16_t MAX_PULSE = 2167;  

// ---- Hold Watchdog ----
// Web D-pad re-sends its gait every 200ms while held; if refreshes stop
// (browser closed, button released, WiFi dropped), auto-stop the robot.
const uint32_t HOLD_TIMEOUT_MS = 600;  // ms without refresh before auto-stop (3 missed refreshes)

// ---- Motion Interpolation ----
const int INTERP_TICK_MS = 20;   // ms per interpolator tick (50 Hz)
int MAX_SPEED            = 600;  // deg/s max angular velocity (lower = smoother, higher = snappier)

// ---- Subtrim ----
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// ======================================================================
// HARDWARE
// ======================================================================

Servo servos[8];
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

SemaphoreHandle_t displayMutex;

// ======================================================================
// SERVO INTERPOLATION STATE
// ======================================================================

float          servoPos[8];     // Where each servo is now (deg)
volatile float servoTarget[8];  // Where each servo is headed (deg)

// ======================================================================
// STATE MACHINE
// ======================================================================

volatile MovementState currentCommand = STATE_IDLE;
QueueHandle_t commandQueue;

// held=true means the sender will keep re-sending (web D-pad); watchdog
// stops the robot if refreshes lapse. Serial commands/poses send false.
struct Command {
  MovementState state;
  bool          held;
};

// Watchdog bookkeeping. Written ONLY from motionTask context (via
// pollCommandQueue — the single point where commands leave the queue)
volatile bool          holdWatchdog      = false;  // armed by a held command
volatile unsigned long lastHoldRefreshMs = 0;      // millis() of last held refresh

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
// FACE LOOKUP TABLE
// ======================================================================

volatile MovementState lastDisplayedState = STATE_IDLE;

struct FaceEntry {
  MovementState state;
  const unsigned char* bitmap;
};

static const FaceEntry FACE_TABLE[] = {
  { STATE_IDLE,     nullptr              },
  { STATE_STAND,    epd_bitmap_stand     },
  { STATE_REST,     epd_bitmap_rest      },
  { STATE_FORWARD,  epd_bitmap_walk      },
  { STATE_BACKWARD, epd_bitmap_walk      },
  { STATE_LEFT,     epd_bitmap_walk      },
  { STATE_RIGHT,    epd_bitmap_walk      },
  { STATE_WAVE,     epd_bitmap_wave      },
  { STATE_DANCE,    epd_bitmap_dance     },
  { STATE_SWIM,     epd_bitmap_swim      },
  { STATE_POINT,    epd_bitmap_point     },
  { STATE_PUSHUP,   epd_bitmap_pushup    },
  { STATE_BOW,      epd_bitmap_bow       },
  { STATE_CUTE,     epd_bitmap_cute      },
  { STATE_FREAKY,   epd_bitmap_freaky    },
  { STATE_WORM,     epd_bitmap_worm      },
  { STATE_SHAKE,    epd_bitmap_shake     },
  { STATE_SHRUG,    epd_bitmap_shrug     },
  { STATE_DEAD,     epd_bitmap_dead      },
  { STATE_CRAB,     epd_bitmap_crab      },
};
static const int FACE_TABLE_SIZE = sizeof(FACE_TABLE) / sizeof(FACE_TABLE[0]);

// ======================================================================
// SERIAL CLI STATE
// ======================================================================

char commandBuffer[64];
byte bufferPos = 0;

// ======================================================================
// PROTOTYPES
// ======================================================================

void webServerTask(void* param);
void serialTask(void* param);
void motionTask(void* param);
void interpolatorTask(void* param);

void setServoTarget(uint8_t channel, int angle);
void setServoImmediate(uint8_t channel, int angle);
void writeServoHardware(uint8_t channel, float pos);
void submitCommand(MovementState state, bool held);
bool pollCommandQueue();
bool pressingCheck(MovementState expectedState, int ms);
bool parseMovementCommand(const char* str, MovementState& out);
const char* commandName(MovementState state);
void handleSubtrimCommand(const char* buf);
void checkSerial();
bool interruptibleDelay(int ms);

void setupWiFi();
void setupWebServer();
void handleRoot();
void handleCmd();
void handleMotor();
void handleStatus();

void setupOLED();
void drawFace(const unsigned char* bitmap);
void showFace(MovementState state);

// ======================================================================
// SETUP
// ======================================================================

void setup() {

  // FreeRTOS setup
  displayMutex = xSemaphoreCreateMutex();
  commandQueue = xQueueCreate(1, sizeof(Command));

  // Initialize serial communication
  Serial.begin(115200);
  delay(100);

  Serial.println("Hermit Crab booting up...");

  // Initialize I2C and OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  setupOLED();
  drawFace(epd_bitmap_idle);

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

  // Seed the interpolator: snap every servo to rest (90°) once
  for (int i = 0; i < 8; i++) {
    setServoImmediate(i, 90);
    delay(5);
  }

  // Initialize wifi access point
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
  vTaskDelete(NULL); // Arduino loop not needed with FreeRTOS implementation
}

// ======================================================================
// TASKS
// ======================================================================

void webServerTask(void* param) {
  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5)); // Yield for 5ms between polls
  }
}

void serialTask(void* param) {
  for (;;) {
    checkSerial();
    vTaskDelay(pdMS_TO_TICKS(10)); // Yield for 10ms between polls
  }
}

void interpolatorTask(void* param) {
  for (;;) {
    float maxStep = (float)MAX_SPEED * INTERP_TICK_MS / 1000.0f; // Degrees

    for (int i = 0; i < 8; i++) {
      float target = servoTarget[i];
      float diff   = target - servoPos[i];

      if (diff != 0.0f) {
        if      (diff >  maxStep) servoPos[i] += maxStep;
        else if (diff < -maxStep) servoPos[i] -= maxStep;
        else                      servoPos[i]  = target;  // arrived
        writeServoHardware(i, servoPos[i]);
      }
      // No write when already at target — servo holds position on its own
    }

    vTaskDelay(pdMS_TO_TICKS(INTERP_TICK_MS));
  }
}

void motionTask(void* param) {
  for (;;) {
    
    pollCommandQueue();

    // On state transition: update face + announce
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
        // One gait cycle; replays if still held. Abort-to-idle recovers to
        // stand posture, so match the face (abort into another command is
        // already handled by the transition block above).
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
          // Pose glided back to stand (standAtEnd) — match the face
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
    // Robot continues to work without the display
    return;
  }
  display.clearDisplay();
  display.display();
  Serial.println("OLED initialized");
}

void showFace(MovementState state) {
  const unsigned char* bitmap = nullptr;
  // Match face bitmap to movement state
  for (int i = 0; i < FACE_TABLE_SIZE; i++) {
    if (FACE_TABLE[i].state == state) {
      bitmap = FACE_TABLE[i].bitmap;
      break;
    }
  }
  if (bitmap == nullptr) return;
  drawFace(bitmap);
}

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

// GET /cmd?c=<command> — accepts any COMMAND_TABLE string. Movement states
// are held (caller re-sends); poses run once and self-transition to STATE_IDLE.
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
  submitCommand(STATE_IDLE, false);
  setServoTarget(idx, angle);
  server.send(200, "text/plain", "ok");
}

// GET /status — lightweight JSON for future scripting / Python use
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

// Walks the lookup table and maps an input string to a MovementState
bool parseMovementCommand(const char* str, MovementState& out) {
  for (int i = 0; i < COMMAND_TABLE_SIZE; i++) {
    if (strcmp(str, COMMAND_TABLE[i].str) == 0) {
      out = COMMAND_TABLE[i].state;
      return true;
    }
  }
  return false;
}

// Reverse lookup: state → command string, for transition announcements
const char* commandName(MovementState state) {
  for (int i = 0; i < COMMAND_TABLE_SIZE; i++) {
    if (COMMAND_TABLE[i].state == state) return COMMAND_TABLE[i].str;
  }
  return "?";
}

// Handles all "subtrim ..." command variants
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

void setServoTarget(uint8_t channel, int angle) {
  if (channel >= 8) return;
  servoTarget[channel] = (float)constrain(angle, 0, 180);
}

void setServoImmediate(uint8_t channel, int angle) {
  if (channel >= 8) return;
  float a = (float)constrain(angle, 0, 180);
  servoPos[channel]    = a;
  servoTarget[channel] = a;
  writeServoHardware(channel, a);
}

void writeServoHardware(uint8_t channel, float pos) {
  int adjusted = constrain((int)(pos + 0.5f) + servoSubtrim[channel], 0, 180);
  int pulseUs  = map(adjusted, 0, 180, MIN_PULSE, MAX_PULSE);
  servos[channel].writeMicroseconds(pulseUs);
}

// Reads and processes any pending serial bytes mid-animation
void checkSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    // Check for termination character
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
    // Fill buffer if space remains
    else if (bufferPos < sizeof(commandBuffer) - 1) {
      commandBuffer[bufferPos++] = c;
    }
  }
}

// ---- Command queue helpers ----
// submitCommand(): commands enter here, safe from any task.
// pollCommandQueue(): commands leave here, called only from motionTask context
// (loop, interruptibleDelay, pressingCheck) — the single writer of currentCommand
// and the watchdog state, so no locks needed.

void submitCommand(MovementState state, bool held) {
  Command cmd = { state, held };
  xQueueOverwrite(commandQueue, &cmd);
}

bool pollCommandQueue() {
  Command incoming;
  if (xQueueReceive(commandQueue, &incoming, 0) != pdTRUE) return false;
  currentCommand = incoming.state;
  holdWatchdog   = incoming.held;
  if (incoming.held) lastHoldRefreshMs = millis();
  return true;
}

// Used in animated poses (wave, dance, etc.)
bool interruptibleDelay(int ms) {
  MovementState stateAtStart = currentCommand;
  unsigned long start = millis();
  while (millis() - start < ms) {
    pollCommandQueue();
    // Bail early if currentCommand changes at all
    if (currentCommand != stateAtStart) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  return true;
}

// Used in continuous gaits (walk, turn, etc.)
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
    // Bail early if currentCommand does not remain in the expected state
    if (currentCommand != expectedState) return false;

    vTaskDelay(pdMS_TO_TICKS(1));
  }
  return true;
}
