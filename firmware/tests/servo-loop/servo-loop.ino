#include <Arduino.h>

// ======================================================================
// CONFIGURATION
// ======================================================================

// Index:                 0   1   2   3   4    5   6   7
// Label:                 R1  R2  L1  L2  R4  R3  L3  L4
// Connector:             J4  J5  J6  J7  J8  J9  J10 J11
const int servoPins[8] = {4, 5, 6, 7, 15, 16, 17, 18};

// MG90D 270deg servo calibration
const int MIN_PULSE = 833;   // 500  + 333
const int MAX_PULSE = 2167;  // 2500 - 333

// ---- LEDC setup ----
const int LEDC_FREQ = 50; // Hz, 20 ms frame
const int LEDC_RES  = 14; // bits of duty resolution
                                            
const uint32_t FRAME_US = 1000000UL / LEDC_FREQ; // 20000 us

// ---- Sweep tuning ----
const int SWEEP_MIN   = 30;  // deg
const int SWEEP_MAX   = 150; // deg
const int SWEEP_STEP  = 2;   // deg per update
const int SWEEP_DELAY = 20;  // ms between updates (1 servo frame)

bool attachOk[8] = {false, false, false, false, false, false, false, false};

// ======================================================================
// PROTOTYPES
// ======================================================================

int      angleToPulse(int angle);
uint32_t usToDuty(uint32_t us);
void     writeAngle(int id, int angle);

// ======================================================================
// SETUP
// ======================================================================

void setup() {
  for (int i = 0; i < 8; i++) {
    attachOk[i] = ledcAttach(servoPins[i], LEDC_FREQ, LEDC_RES);
  }
}

// ======================================================================
// MAIN LOOP
// ======================================================================

void loop() {
  // Sweep all servos up then back down to view on oscilloscope for debugging
  for (int angle = SWEEP_MIN; angle <= SWEEP_MAX; angle += SWEEP_STEP) {
    for (int i = 0; i < 8; i++) writeAngle(i, angle);
    delay(SWEEP_DELAY);
  }
  for (int angle = SWEEP_MAX; angle >= SWEEP_MIN; angle -= SWEEP_STEP) {
    for (int i = 0; i < 8; i++) writeAngle(i, angle);
    delay(SWEEP_DELAY);
  }
}

// ======================================================================
// HELPERS
// ======================================================================

// Map a 0-180deg angle to the correct pulse width for the 270deg servo
int angleToPulse(int angle) {
  if (angle < 0)   angle = 0;
  if (angle > 180) angle = 180;
  return map(angle, 0, 180, MIN_PULSE, MAX_PULSE);
}

// Convert a pulse width in microseconds to an LEDC duty count.
// 64-bit intermediate so the shift cannot overflow.
uint32_t usToDuty(uint32_t us) {
  return (uint32_t)(((uint64_t)us << LEDC_RES) / FRAME_US);
}

void writeAngle(int id, int angle) {
  if (id < 0 || id > 7) return;
  if (!attachOk[id])    return;
  ledcWrite(servoPins[id], usToDuty(angleToPulse(angle)));
}
