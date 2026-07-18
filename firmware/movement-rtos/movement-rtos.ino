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

// ---- Tuning ----
int frameDelay        = 100;  // ms between gait frames
int motorCurrentDelay = 5;    // ms between servo writes (prevents brownout)

// ---- Subtrim ----
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// ======================================================================
// HARDWARE
// ======================================================================

Servo servos[8];
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

SemaphoreHandle_t servoMutex;
SemaphoreHandle_t displayMutex;

// ======================================================================
// STATE MACHINE
// ======================================================================

volatile MovementState currentCommand = STATE_IDLE;
QueueHandle_t commandQueue;

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

// Track last state to only redraw face bitmap on a transition
volatile MovementState lastDisplayedState = STATE_IDLE; 

struct FaceEntry {
  MovementState state;
  const unsigned char* bitmap;
};

// nullptr = leave the current face on screen (no update)
static const FaceEntry FACE_TABLE[] = {
  { STATE_IDLE,     epd_bitmap_idle      },
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

void setServoAngle(uint8_t channel, int angle);
bool pressingCheck(MovementState expectedState, int ms);
bool parseMovementCommand(const char* str, MovementState& out);
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
void showFace(MovementState state);

// ======================================================================
// SETUP
// ======================================================================

void setup() {

  // FreeRTOS
  servoMutex = xSemaphoreCreateMutex();
  displayMutex = xSemaphoreCreateMutex();
  commandQueue = xQueueCreate(1, sizeof(MovementState));

  // Initialize serial communication
  Serial.begin(115200);
  delay(100);

  Serial.println("Hermit Crab booting up...");

  // Initialize I2C and OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  setupOLED();

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

  // Initialize wifi access point
  setupWiFi();
  setupWebServer();

  // Idle face on boot
  showFace(STATE_IDLE);

  // Core 0 - I/0 tasks
  xTaskCreatePinnedToCore(webServerTask, "WebServer", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(serialTask, "Serial", 2048, NULL, 1, NULL, 0);

  // Core 1 - motion (higher priority)
  xTaskCreatePinnedToCore(motionTask, "Motion", 8192, NULL, 2, NULL, 1);

  // Startup message
  Serial.println("-----------------------------------");
  Serial.println("    HERMIT CRAB WIFI + SERIAL      ");
  Serial.println("-----------------------------------");
  Serial.print  (" Web UI: http://");
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
  vTaskDelete(NULL); // no longer needed with FreeRTOS implementation
}

// ======================================================================
// TASKS
// ======================================================================

void webServerTask(void* param) {
  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void serialTask(void* param) {
  for (;;) {
    checkSerial();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void motionTask(void* param) {
  for (;;) {
    // Pull latest command from queue if one is waiting
    MovementState incoming;
    if (xQueueReceive(commandQueue, &incoming, 0) == pdTRUE) {
      currentCommand = incoming;        
    }
    
    // Update face on state change
    if (currentCommand != lastDisplayedState) {
      showFace(currentCommand);
      lastDisplayedState = currentCommand;
    }

    // Dispatch state machine
    MovementState running = currentCommand; 

    switch (running) {
      case STATE_IDLE:                          break;

      // Gaits: re-dispatched every loop while the state holds — no IDLE transition
      case STATE_FORWARD:  runWalkPose();       break;
      case STATE_BACKWARD: runWalkBackward();   break;
      case STATE_LEFT:     runTurnLeft();       break;
      case STATE_RIGHT:    runTurnRight();      break;

      // One-shot poses
      case STATE_STAND:    runStandPose();      break;
      case STATE_REST:     runRestPose();       break;
      case STATE_WAVE:     runWavePose();       break;
      case STATE_DANCE:    runDancePose();      break;
      case STATE_SWIM:     runSwimPose();       break;
      case STATE_POINT:    runPointPose();      break;
      case STATE_PUSHUP:   runPushupPose();     break;
      case STATE_BOW:      runBowPose();        break;
      case STATE_CUTE:     runCutePose();       break;
      case STATE_FREAKY:   runFreakyPose();     break;
      case STATE_WORM:     runWormPose();       break;
      case STATE_SHAKE:    runShakePose();      break;
      case STATE_SHRUG:    runShrugPose();      break;
      case STATE_DEAD:     runDeadPose();       break;
      case STATE_CRAB:     runCrabPose();       break;
      default:                                  break;
    }

    bool isGait = (running == STATE_FORWARD || running == STATE_BACKWARD ||
                  running == STATE_LEFT    || running == STATE_RIGHT);

    // Only one-shot poses return to idle, and only if nothing new arrived while they ran
    if (!isGait && running != STATE_IDLE && currentCommand == running) {
      currentCommand = STATE_IDLE;
    }
    
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}


// ======================================================================
// OLED SETUP & FACE DISPLAY
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

  // If no bitmap is mapped for this state (or bitmap is null), do nothing
  if (bitmap == nullptr) return;

  // Thread-safe writing to OLED
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

// GET /cmd?c=<command>
// Accepts any string from COMMAND_TABLE
// Movement states (walk/back/left/right) are held — caller re-sends while held
// Pose states run once and self-transition to STATE_IDLE once done
void handleCmd() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "missing arg");
    return;
  }
  String cmdStr = server.arg("c");
  MovementState parsed;
  if (parseMovementCommand(cmdStr.c_str(), parsed)) {
    xQueueOverwrite(commandQueue, &parsed);
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
  MovementState idle = STATE_IDLE; // stop any running animation first
  xQueueOverwrite(commandQueue, &idle);
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

// Handles all "subtrim ..." variants
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

// Writes a servo angle, applying subtrim offset and clamping
void setServoAngle(uint8_t channel, int angle) {
  if (channel >= 8) return;
  int adjusted = constrain(angle + servoSubtrim[channel], 0, 180);
  int pulseUs  = map(adjusted, 0, 180, MIN_PULSE, MAX_PULSE);

  // Thread-safe writing to servos
  xSemaphoreTake(servoMutex, portMAX_DELAY);
  servos[channel].writeMicroseconds(pulseUs);
  xSemaphoreGive(servoMutex);

  delay(motorCurrentDelay); // Current limiter
}

// Reads and processes any pending serial bytes mid-animation
void checkSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufferPos > 0) {
        commandBuffer[bufferPos] = '\0';
        bufferPos = 0;

        MovementState parsed;
        if (parseMovementCommand(commandBuffer, parsed)) {
          xQueueOverwrite(commandQueue, &parsed);
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

// Used in animated poses (wave, dance, etc.)
// Bails early if currentCommand changes at all
bool interruptibleDelay(int ms) {
  MovementState stateAtStart = currentCommand;
  unsigned long start = millis();
  while (millis() - start < ms) {
    MovementState incoming;
    if (xQueueReceive(commandQueue, &incoming, 0) == pdTRUE) {
      currentCommand = incoming;
    }
    if (currentCommand != stateAtStart) return false;
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  return true;
}

// Used in continuous gaits (walk, turn, etc.)
// Bails early if currentCommand does not remain in the specific expected state
bool pressingCheck(MovementState expectedState, int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    // Pull any incoming command
    MovementState incoming;
    if (xQueueReceive(commandQueue, &incoming, 0) == pdTRUE) {
      currentCommand = incoming;
    }
    if (currentCommand != expectedState) {
      applyStandPose(); // Explicit reset to neutral before the next state takes over
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // Yield to the scheduler instead of burning CPU
  }
  return true;
}
