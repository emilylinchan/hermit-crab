#include <Arduino.h>
#include <ESP32Servo.h>

// ======================================================================
// --- CONFIGURATION ---
// ======================================================================

Servo servos[8];

// Motor Pin Mapping
// Index:                 0   1   2   3   4    5   6   7
// Label:                 R1  R2  L1  L2  R4  R3  L3  L4
const int servoPins[8] = {13, 14, 15, 16, 17, 18, 19, 21};

// ---- MG90D 270° SERVO CALIBRATION ----
// Full travel: 500–2500 µs  → 2000 µs span for 270°
// Using only the center 180° → trim 45° from each end
// Trim per side: (45° / 270°) * 2000 µs ≈ 333 µs
// Final usable pulse range: 833–2167 µs

const int MIN_PULSE = 833;   // 500  + 333
const int MAX_PULSE = 2167;  // 2500 - 333

// ======================================================================
// --- SETUP ---
// ======================================================================

void setup() {
  Serial.begin(115200); // Ensure the baud rate matches in the serial terminal
  while (!Serial);

  Serial.println("-----------------------------------");
  Serial.println("   Servo Motor Tester Interface    ");
  Serial.println("-----------------------------------");
  Serial.println("Commands:");
  Serial.println("1. id,angle   -> e.g. '0,90'");
  Serial.println("2. all,angle  -> e.g. 'all,90'");
  Serial.println("3. stop       -> Detaches/Powers down motors");
  Serial.println("-----------------------------------");
  Serial.print  ("Pulse range mapped to 180°: ");
  Serial.print  (MIN_PULSE);
  Serial.print  (" - ");
  Serial.print  (MAX_PULSE);
  Serial.println(" µs");
  Serial.println("Status: Motors are currently OFF (Limp).");

  // Reserve all 4 LEDC hardware timers so that the Servo library can guarantee stable microsecond pulses for PWM
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
}

// ======================================================================
// --- MAIN LOOP ---
// ======================================================================

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() == 0) return;

    if (input.equalsIgnoreCase("stop")) {
      stopMotors();
      return;
    }

    // Parse input
    int commaIndex = input.indexOf(',');
    if (commaIndex != -1) {
      String cmd    = input.substring(0, commaIndex);
      String valStr = input.substring(commaIndex + 1);
      int angle     = valStr.toInt();

      // Limit servo angle range to just 0-180°
      if (angle < 0)   angle = 0;
      if (angle > 180) angle = 180;

      if (cmd.equalsIgnoreCase("all")) {
        moveAll(angle);
      } 
      else {
        int motorId = cmd.toInt();
        // Handle ambiguity of String.toInt() in Arduino IDE to distinguish if actually a valid motor command
        // ***NOTE*** toInt() returns 0 in two different situations:
        //      1. The string actually contains "0" (a valid motor ID)
        //      2. The string is non-numeric characters like "abc" (which also returns 0 as a failure/default)
        if (motorId == 0 && cmd.charAt(0) != '0') {
          Serial.println("Error: Invalid Motor ID");
        } 
        else {
          moveMotor(motorId, angle);
        }
      }
    } 
    else {
      Serial.println("Error: Invalid format. Use 'id,angle', 'all,angle', or 'stop'.");
    }
  }
}

// ======================================================================
// --- HELPER FUNCTIONS ---
// ======================================================================

int angleToPulse(int angle) {
  // Map a 0-180° angle to the correct pulse width for the 270° servo
  return map(angle, 0, 180, MIN_PULSE, MAX_PULSE);
}

void moveMotor(int id, int angle) {
  // Validate ID
  if (id < 0 || id > 7) {
    Serial.println("Error: Motor ID must be 0-7");
    return;
  }
  // Lazy attach motors
  if (!servos[id].attached()) {
    servos[id].setPeriodHertz(50);
    servos[id].attach(servoPins[id], MIN_PULSE, MAX_PULSE);
  }

  // writeMicroseconds() is used instead of write() for precise pulse control.
  // ***NOTE*** write() relies on the library's internal angle-to-pulse mapping which
  //            is calibrated for 180° servos — bypassing it avoids any drift at the endpoints.
  servos[id].writeMicroseconds(angleToPulse(angle));
  
  Serial.print("OK: Motor ");
  Serial.print(id);
  Serial.print(" -> ");
  Serial.print(angle);
  Serial.print("° (");
  Serial.print(angleToPulse(angle));
  Serial.println(" µs)");
}

void moveAll(int angle) {
  Serial.print("Moving ALL to ");
  Serial.println(angle);
  for (int i = 0; i < 8; i++) {
    moveMotor(i, angle);
  }
}

void stopMotors() {
  Serial.println("Stopping (Detaching) all motors...");
  for (int i = 0; i < 8; i++) {
    if (servos[i].attached()) {
      servos[i].detach();
    }
  }
  Serial.println("Motors are now OFF.");
}