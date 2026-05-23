#include <Arduino.h>
#include <ESP32Servo.h>

// ======================================================================
// --- CONFIGURATION ---
// ======================================================================

Servo servos[8];

// Motor Pin Mapping
// Index: 0  1  2  3  4   5   6   7
const int servoPins[8] = {13, 14, 15, 16, 17, 18, 19, 21};

// ---- 270° Servo Pulse Range ----
// These define the FULL 270° travel of your servo.
// Adjust if your servo's datasheet specifies different values.
const int PULSE_270_MIN = 500;
const int PULSE_270_MAX = 2500;

// ---- Mapped 180° Pulse Range ----
// We use only the CENTER 180° of the 270° sweep.
// Formula: total range * (180/270) = total range * (2/3)
// Each side is trimmed by: total range * (1/6)
const int TOTAL_PULSE_RANGE = PULSE_270_MAX - PULSE_270_MIN;            // 2000 µs
const int TRIM             = TOTAL_PULSE_RANGE / 6;                     // ~333 µs
const int MIN_PULSE        = PULSE_270_MIN + TRIM;                      // ~833 µs
const int MAX_PULSE        = PULSE_270_MAX - TRIM;                      // ~2167 µs

// ======================================================================
// --- SETUP ---
// ======================================================================

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("-----------------------------------");
  Serial.println("   Sesame Motor Tester Interface   ");
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

    int commaIndex = input.indexOf(',');
    
    if (commaIndex != -1) {
      String cmd    = input.substring(0, commaIndex);
      String valStr = input.substring(commaIndex + 1);
      int angle     = valStr.toInt();

      if (angle < 0)   angle = 0;
      if (angle > 180) angle = 180;

      if (cmd.equalsIgnoreCase("all")) {
        moveAll(angle);
      } else {
        int motorId = cmd.toInt();
        if (motorId == 0 && cmd.charAt(0) != '0') {
          Serial.println("Error: Invalid Motor ID");
        } else {
          moveMotor(motorId, angle);
        }
      }
      
    } else {
      Serial.println("Error: Invalid format. Use 'id,angle', 'all,angle', or 'stop'.");
    }
  }
}

// ======================================================================
// --- HELPER FUNCTIONS ---
// ======================================================================

// Maps a 0-180° angle to the correct pulse width for the 270° servo.
int angleToPulse(int angle) {
  return map(angle, 0, 180, MIN_PULSE, MAX_PULSE);
}

void moveMotor(int id, int angle) {
  if (id < 0 || id > 7) {
    Serial.println("Error: Motor ID must be 0-7");
    return;
  }

  if (!servos[id].attached()) {
    servos[id].setPeriodHertz(50);
    servos[id].attach(servoPins[id], MIN_PULSE, MAX_PULSE);
  }

  // Use writeMicroseconds() instead of write() for precise pulse control.
  // write() relies on the library's internal angle-to-pulse mapping which
  // is calibrated for 180° servos — bypassing it avoids any drift at the endpoints.
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