#include <ESP32Servo.h>
#include "movement-sequences.h"

// ======================================================================
// CONFIGURATION
// ======================================================================

// ---- Hardware ----
const int OPERATING_FREQ = 50;                                // Hz
const int servoPins[8]  = {13, 14, 15, 16, 17, 18, 19, 21};   // Index: 0  1  2  3   4   5   6   7

// ---- MG90D Pulse Calibration ----
// Full travel: 500–2500 µs → 2000 µs span for 270°
// Trimming to center 180°: (45° / 270°) * 2000 µs ≈ 333 µs per side
const int MIN_PULSE = 833;    // 500  + 333
const int MAX_PULSE = 2167;   // 2500 - 333

// ---- Tuning ----
int frameDelay        = 100;  // ms between gait frames
int motorCurrentDelay = 5;    // ms between servo writes (prevents brownout)

// ---- Subtrim ----
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// ======================================================================
// STATE
// ======================================================================

Servo servos[8];
String currentCommand = "";
char   command_buffer[64];
byte   buffer_pos = 0;

// ======================================================================
// PROTOTYPES
// ======================================================================

void setServoAngle(uint8_t channel, int angle);
bool pressingCheck(String cmd, int ms);

// ======================================================================
// Setup
// ======================================================================
void setup() {
  Serial.begin(115200); // Baud rate for Serial Monitor
  while (!Serial);

  Serial.println("Hermit Crab booting up...");

  // Reserve all 4 LEDC hardware timers so that the Servo library can guarantee stable pulses for PWM
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Servo init with 270° calibration
  for (int i = 0; i < 8; i++) {
    servos[i].setPeriodHertz(OPERATING_FREQ);
    servos[i].attach(servoPins[i], MIN_PULSE, MAX_PULSE);
  }
  delay(10);

  Serial.println("-----------------------------------");
  Serial.println("    HERMIT CRAB SERIAL INTERFACE   ");
  Serial.println("-----------------------------------");
  Serial.println("Static Pose Commands:");
  Serial.println("  rest, stand");
  Serial.println();
  Serial.println("Animated Pose Commands:");
  Serial.println("  wave, dance, swim, point, pushup");
  Serial.println("  bow, cute, freaky, worm, shake");
  Serial.println("  shrug, dead, crab");
  Serial.println();
  Serial.println("Movement Commands:");
  Serial.println("  walk, back, left, right");
  Serial.println();
  Serial.println("Subtrim Commands:");
  Serial.println("  subtrim               (show values)");
  Serial.println("  subtrim <m> <v>       (set motor m to value v)");
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
// Loop
// ======================================================================
void loop() {
  
// State executor
  if (currentCommand != "") {
    String cmd = currentCommand;
    if      (cmd == "forward")  runWalkPose();
    else if (cmd == "backward") runWalkBackward();
    else if (cmd == "left")     runTurnLeft();
    else if (cmd == "right")    runTurnRight();
    else if (cmd == "rest")     runRestPose();
    else if (cmd == "stand")    runStandPose();
    else if (cmd == "wave")     runWavePose();
    else if (cmd == "dance")    runDancePose();
    else if (cmd == "swim")     runSwimPose();
    else if (cmd == "point")    runPointPose();
    else if (cmd == "pushup")   runPushupPose();
    else if (cmd == "bow")      runBowPose();
    else if (cmd == "cute")     runCutePose();
    else if (cmd == "freaky")   runFreakyPose();
    else if (cmd == "worm")     runWormPose();
    else if (cmd == "shake")    runShakePose();
    else if (cmd == "shrug")    runShrugPose();
    else if (cmd == "dead")     runDeadPose();
    else if (cmd == "crab")     runCrabPose();
  }

  // ----- Serial CLI -----
  if (Serial.available()) {
    char c = Serial.read();

    // Set state if end of command entered
    if (c == '\n' || c == '\r') {

      // Reset command buffer
      if (buffer_pos > 0) {
        command_buffer[buffer_pos] = '\0';
        buffer_pos = 0;
        int motorNum, angle;

        // Stop command
        if (strcmp(command_buffer, "stop") == 0) {
          currentCommand = "";
          runStandPose();
          Serial.println("STOPPED");
        }

        // Static poses
        else if (strcmp(command_buffer, "rest")  == 0) {
          currentCommand = "";
          runRestPose();
        }
        else if (strcmp(command_buffer, "stand") == 0) {
          currentCommand = "";
          runStandPose();
        }

        // Animated poses
        else if (strcmp(command_buffer, "wave") == 0) { currentCommand = "wave";   runWavePose(); }
        else if (strcmp(command_buffer, "dance") == 0) { currentCommand = "dance";  runDancePose(); }
        else if (strcmp(command_buffer, "swim") == 0) { currentCommand = "swim";   runSwimPose(); }
        else if (strcmp(command_buffer, "point") == 0) { currentCommand = "point";  runPointPose(); }
        else if (strcmp(command_buffer, "pushup") == 0) { currentCommand = "pushup"; runPushupPose(); }
        else if (strcmp(command_buffer, "bow") == 0) { currentCommand = "bow";    runBowPose(); }
        else if (strcmp(command_buffer, "cute") == 0) { currentCommand = "cute";   runCutePose(); }
        else if (strcmp(command_buffer, "freaky") == 0) { currentCommand = "freaky"; runFreakyPose(); }
        else if (strcmp(command_buffer, "worm") == 0) { currentCommand = "worm";   runWormPose(); }
        else if (strcmp(command_buffer, "shake") == 0) { currentCommand = "shake";  runShakePose(); }
        else if (strcmp(command_buffer, "shrug") == 0) { currentCommand = "shrug";  runShrugPose(); }
        else if (strcmp(command_buffer, "dead") == 0) { currentCommand = "dead";   runDeadPose(); }
        else if (strcmp(command_buffer, "crab") == 0) { currentCommand = "crab";   runCrabPose(); }

        // Movement commands
        else if (strcmp(command_buffer, "walk") == 0) { currentCommand = "forward"; runWalkPose(); }
        else if (strcmp(command_buffer, "back") == 0) { currentCommand = "backward"; runWalkBackward(); }
        else if (strcmp(command_buffer, "left") == 0) { currentCommand = "left"; runTurnLeft(); }
        else if (strcmp(command_buffer, "right") == 0) { currentCommand = "right"; runTurnRight(); }

        // --- Subtrim Commands ---

        // Print all subtrim values
        else if (strcmp(command_buffer, "subtrim") == 0) {
          Serial.println("Subtrim values:");
          for (int i = 0; i < 8; i++) {
            Serial.print("Motor "); 
            Serial.print(i); 
            Serial.print(": ");
            if (servoSubtrim[i] >= 0) Serial.print("+");
            Serial.println(servoSubtrim[i]);
          }
        }

        // Save subtrim array for code
        else if (strcmp(command_buffer, "subtrim save") == 0) {
          Serial.println("Copy this into the code:");
          Serial.print("int8_t servoSubtrim[8] = {");
          for (int i = 0; i < 8; i++) {
            Serial.print(servoSubtrim[i]);
            if (i < 7) Serial.print(", ");
          }
          Serial.println("};");
        }

        // Reset all subtrims
        else if (strcmp(command_buffer, "subtrim reset") == 0) {
          for (int i = 0; i < 8; i++) servoSubtrim[i] = 0;
          Serial.println("All subtrim values reset to 0");
        }

        // Set subtrim for one motor
        else if (strncmp(command_buffer, "subtrim ", 8) == 0) {
          int trimMotor, trimValue;
          if (sscanf(command_buffer + 8, "%d %d", &trimMotor, &trimValue) == 2) {
            if (trimMotor >= 0 && trimMotor < 8 && trimValue >= -90 && trimValue <= 90) {
              servoSubtrim[trimMotor] = (int8_t)trimValue;
              Serial.print("Motor "); 
              Serial.print(trimMotor);
              Serial.print(" subtrim set to ");
              if (trimValue >= 0) Serial.print("+");
              Serial.println(trimValue);
            }
            else {
              Serial.println("Invalid subtrim values entered");
            }
          }
        }

        // --- Direct Motor Control ---

        // all <angle>
        else if (strncmp(command_buffer, "all ", 4) == 0) {
          if (sscanf(command_buffer + 4, "%d", &angle) == 1) {
            for (int i = 0; i < 8; i++) setServoAngle(i, angle);
            Serial.print("All servos -> "); 
            Serial.println(angle);
          }
        }

        // <motor> <angle> 
        else if (sscanf(command_buffer, "%d %d", &motorNum, &angle) == 2) {
          if (motorNum >= 0 && motorNum < 8) {
            setServoAngle(motorNum, angle);
            Serial.print("Servo "); 
            Serial.print(motorNum);
            Serial.print(" -> "); 
            Serial.println(angle);
          } 
          else {
            Serial.println("Invalid motor number (0-7)");
          }
        }
        else {
          Serial.print("Unknown command: ");
          Serial.println(command_buffer);
        }
      }
    } 
    
    // Build command buffer 
    else if (buffer_pos < sizeof(command_buffer) - 1) {
      command_buffer[buffer_pos++] = c;
    }
  }
}

// ======================================================================
// Helper Functions
// ======================================================================

void setServoAngle(uint8_t channel, int angle) {
    if (channel >= 8) return;

    int adjusted = constrain(angle + servoSubtrim[channel], 0, 180); // Apply subtrim and clamp
    int pulseUs = map(adjusted, 0, 180, MIN_PULSE, MAX_PULSE);
    servos[channel].writeMicroseconds(pulseUs);

    delay(motorCurrentDelay); // Needed to prevent brownout and/or motor stalls
}


bool pressingCheck(String cmd, int ms) {
  unsigned long start = millis();
  
  while (millis() - start < ms) {
    // Only continue a movement if the user is still commanding it, and stop instantly if not
    if (currentCommand != cmd) {
      runStandPose();
      return false;
    }
    yield(); // Cooperative non‑blocking
  }
  return true;
}