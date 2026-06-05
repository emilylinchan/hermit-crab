#include <Wifi.h>
#include <ESP32Servo.h>
#include "movement-sequences.h"

// ======================================================================
// CONFIGURATION
// ======================================================================

// ---- Wifi Access Point ----
constexpr char SSID[]     = "HermitCrab";
constexpr char PASSWORD[] = "12345678";

// ---- Hardware ----
constexpr uint8_t OPERATING_FREQ = 50;      
// Index:                         0   1   2   3   4   5   6   7
// Label:                         R1  R2  L1  L2  R4  R3  L3  L4
constexpr uint8_t servoPins[8] = {13, 14, 22, 16, 17, 18, 19, 21};       

// ---- MG90D Pulse Calibration ----
// Full travel: 500–2500 µs → 2000 µs span for 270°
// Trimming to center 180°: (45° / 270°) * 2000 µs ≈ 333 µs per side
constexpr uint16_t MIN_PULSE = 833;      
constexpr uint16_t MAX_PULSE = 2167;    

// ---- Tuning ----
int frameDelay         = 100;      // ms between gait frames
int motorCurrentDelay  = 5;        // ms between servo writes (prevents brownout)

// ---- Subtrim ----
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// ======================================================================
// STATE MACHINE
// ======================================================================

MovementState currentCommand = STATE_IDLE;

// ---- Command Lookup Table ----
// Maps serial input strings to their corresponding MovementState.
// Subtrim and direct motor commands are NOT in this table — they are
// handled separately below since they require argument parsing.
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

Servo servos[8];

// ======================================================================
// SERIAL CLI STATE
// ======================================================================

char commandBuffer[64];
byte bufferPos = 0;

// ======================================================================
// PROTOTYPES
// ======================================================================

void setServoAngle(uint8_t channel, int angle);
bool pressingCheck(MovementState expectedState, int ms);
bool parseMovementCommand(const char* str, MovementState& out);
void handleSubtrimCommand(const char* buf);
void checkSerial();
bool interruptibleDelay(int ms);

// ======================================================================
// SETUP
// ======================================================================

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Hermit Crab booting up...");

  // Reserve all 4 LEDC hardware timers so that the Servo library can guarantee stable microsecond pulses for PWM
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Servo init with 270° pulse calibration
  for (int i = 0; i < 8; i++) {
    servos[i].setPeriodHertz(OPERATING_FREQ);
    servos[i].attach(servoPins[i], MIN_PULSE, MAX_PULSE);
  }
  delay(10);

  Serial.println("-----------------------------------");
  Serial.println("    HERMIT CRAB SERIAL INTERFACE   ");
  Serial.println("-----------------------------------");
  Serial.println("Static Poses:");
  Serial.println("  rest, stand");
  Serial.println();
  Serial.println("Animated Poses:");
  Serial.println("  wave, dance, swim, point, pushup");
  Serial.println("  bow, cute, freaky, worm, shake");
  Serial.println("  shrug, dead, crab");
  Serial.println();
  Serial.println("Movement:");
  Serial.println("  walk, back, left, right");
  Serial.println();
  Serial.println("Subtrim:");
  Serial.println("  subtrim               (show all values)");
  Serial.println("  subtrim <m> <v>       (set motor m to offset v)");
  Serial.println("  subtrim reset         (reset all to 0)");
  Serial.println("  subtrim save          (print array for code)");
  Serial.println();
  Serial.println("Direct Motor Control:");
  Serial.println("  <m> <angle>           (set motor m)");
  Serial.println("  all <angle>           (set all motors)");
  Serial.println();
  Serial.println("Other:");
  Serial.println("  stop");
  Serial.println("-----------------------------------");
}

// ======================================================================
// LOOP
// ======================================================================

void loop() {

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
  if (Serial.available()) {
    char c = Serial.read();

    // Set state if end of command enetered
    if (c == '\n' || c == '\r') {

      // Reset command buffer
      if (bufferPos > 0) {
        commandBuffer[bufferPos] = '\0';
        bufferPos = 0;

        MovementState parsed;

        // --- Movement / Pose Commands (via lookup table) ---
        if (parseMovementCommand(commandBuffer, parsed)) {
          currentCommand = parsed;
          Serial.print("Command: ");
          Serial.println(commandBuffer);
        }

        // --- Subtrim Commands ---
        else if (strncmp(commandBuffer, "subtrim", 7) == 0) {
          handleSubtrimCommand(commandBuffer);
        }

        // --- Direct Motor Control: all <angle> ---
        else if (strncmp(commandBuffer, "all ", 4) == 0) {
          int angle;
          if (sscanf(commandBuffer + 4, "%d", &angle) == 1) {
            for (int i = 0; i < 8; i++) setServoAngle(i, angle);
            Serial.print("All servos -> ");
            Serial.println(angle);
          }
        }

        // --- Direct Motor Control: <motor> <angle> ---
        else {
          int motorNum, angle;
          if (sscanf(commandBuffer, "%d %d", &motorNum, &angle) == 2) {
            if (motorNum >= 0 && motorNum < 8) {
              setServoAngle(motorNum, angle);
              Serial.print("Servo ");
              Serial.print(motorNum);
              Serial.print(" -> ");
              Serial.println(angle);
            } else {
              Serial.println("Invalid motor number (0-7)");
            }
          } else {
            Serial.print("Unknown command: ");
            Serial.println(commandBuffer);
          }
        }
      }
    }

    // Build command buffer
    else if (bufferPos < sizeof(commandBuffer) - 1) {
      commandBuffer[bufferPos++] = c;
    }
  }
}

// ======================================================================
// HELPER FUNCTIONS
// ======================================================================

// Walks the lookup table and maps an input string to a MovementState.
// Returns true on a match, false if no match (subtrim/motor commands).
bool parseMovementCommand(const char* str, MovementState& out) {
  for (int i = 0; i < COMMAND_TABLE_SIZE; i++) {
    if (strcmp(str, COMMAND_TABLE[i].str) == 0) {
      out = COMMAND_TABLE[i].state;
      return true;
    }
  }
  return false;
}

// Handles all "subtrim ..." variants. Expects the full commandBuffer.
void handleSubtrimCommand(const char* buf) {

  // subtrim save
  if (strcmp(buf, "subtrim save") == 0) {
    Serial.println("Copy this into the code:");
    Serial.print("int8_t servoSubtrim[8] = {");
    for (int i = 0; i < 8; i++) {
      Serial.print(servoSubtrim[i]);
      if (i < 7) Serial.print(", ");
    }
    Serial.println("};");
  }

  // subtrim reset
  else if (strcmp(buf, "subtrim reset") == 0) {
    for (int i = 0; i < 8; i++) servoSubtrim[i] = 0;
    Serial.println("All subtrim values reset to 0");
  }

  // subtrim <motor> <value>
  else if (strncmp(buf, "subtrim ", 8) == 0) {
    int trimMotor, trimValue;
    if (sscanf(buf + 8, "%d %d", &trimMotor, &trimValue) == 2) {
      if (trimMotor >= 0 && trimMotor < 8 && trimValue >= -90 && trimValue <= 90) {
        servoSubtrim[trimMotor] = (int8_t)trimValue;
        Serial.print("Motor ");
        Serial.print(trimMotor);
        Serial.print(" subtrim set to ");
        if (trimValue >= 0) Serial.print("+");
        Serial.println(trimValue);
      } else {
        Serial.println("Invalid subtrim values — motor: 0-7, value: -90 to +90");
      }
    }
  }

  // subtrim (show all)
  else {
    Serial.println("Subtrim values:");
    for (int i = 0; i < 8; i++) {
      Serial.print("  Motor ");
      Serial.print(i);
      Serial.print(": ");
      if (servoSubtrim[i] >= 0) Serial.print("+");
      Serial.println(servoSubtrim[i]);
    }
  }
}

// Writes a servo angle, applying subtrim offset and clamping to 0-180°.
// Uses writeMicroseconds() to bypass the library's internal 180° mapping.
void setServoAngle(uint8_t channel, int angle) {
  if (channel >= 8) return;
  int adjusted = constrain(angle + servoSubtrim[channel], 0, 180);
  int pulseUs  = map(adjusted, 0, 180, MIN_PULSE, MAX_PULSE);
  servos[channel].writeMicroseconds(pulseUs);
  delay(motorCurrentDelay); // Prevents brownout / motor stall
}

// Holds execution for `ms` milliseconds while the robot is still in
// `expectedState`. Returns false immediately if the state changes,
// triggering the caller to abort its movement sequence.
bool pressingCheck(MovementState expectedState, int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (currentCommand != expectedState) {
      runStandPose();
      return false;
    }
    yield(); // Cooperative non-blocking
  }
  return true;
}

// Reads and processes any pending serial bytes mid-animation so that
// commands like "stop" are acted on immediately rather than queued.
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
          Serial.print("Command: ");
          Serial.println(commandBuffer);
        }
        // Subtrim and direct motor commands are intentionally not handled
        // here — they require argument parsing that is only done in loop().
      }
    } else if (bufferPos < sizeof(commandBuffer) - 1) {
      commandBuffer[bufferPos++] = c;
    }
  }
}

// Drop-in replacement for delay() inside animated poses.
// Returns false if currentCommand changed mid-wait so the caller can bail out of the rest of the animation.
bool interruptibleDelay(int ms) {
  MovementState stateAtStart = currentCommand;
  unsigned long start = millis();
  while (millis() - start < ms) {
    checkSerial();
    if (currentCommand != stateAtStart) return false;
    yield();
  }
  return true;
}
