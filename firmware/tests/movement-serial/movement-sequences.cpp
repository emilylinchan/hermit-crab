#include "movement-sequences.h"

// ---------- SERVO LOCATIONS ----------
int servoNameToIndex(const String& servo) {
  if (servo == "L1") return L1;
  if (servo == "L2") return L2;
  if (servo == "L3") return L3;
  if (servo == "L4") return L4;
  if (servo == "R1") return R1;
  if (servo == "R2") return R2;
  if (servo == "R3") return R3;
  if (servo == "R4") return R4;
  return -1;
}

// ---------- STATIC POSES ----------
// These run once and self-transition to STATE_IDLE so they don't
// get called repeatedly by loop()'s switch dispatch.

void runRestPose() {
  Serial.println("REST");
  for (int i = 0; i < 8; i++) setServoAngle(i, 90);
  currentCommand = STATE_IDLE;
}

void runStandPose() {
  Serial.println("STAND");
  setServoAngle(R1, 135);
  setServoAngle(R2, 45);
  setServoAngle(L1, 45);
  setServoAngle(L2, 135);
  setServoAngle(R4, 0);
  setServoAngle(R3, 180);
  setServoAngle(L3, 0);
  setServoAngle(L4, 180);
  currentCommand = STATE_IDLE;
}

// ---------- ANIMATED POSES ----------
// Each pose runs its full sequence, then returns to stand and 
// sets STATE_IDLE so loop() stops re-invoking it.

void runWavePose() {
  Serial.println("WAVE");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(R4, 80);
  setServoAngle(L3, 180);
  setServoAngle(L2, 90);
  setServoAngle(R1, 100);
  if (!interruptibleDelay(200)) return;
  setServoAngle(L3, 180);
  if (!interruptibleDelay(300)) return;
  for (int i = 0; i < 4; i++) {
    setServoAngle(L3, 180);
    if (!interruptibleDelay(300)) return;
    setServoAngle(L3, 100);
    if (!interruptibleDelay(300)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runDancePose() {
  Serial.println("DANCE");
  setServoAngle(R1, 90);
  setServoAngle(R2, 90);
  setServoAngle(L1, 90);
  setServoAngle(L2, 90);
  setServoAngle(R4, 160);
  setServoAngle(R3, 160);
  setServoAngle(L3, 10);
  setServoAngle(L4, 10);
  if (!interruptibleDelay(300)) return;
  for (int i = 0; i < 5; i++) {
    setServoAngle(R4, 115);
    setServoAngle(R3, 115);
    setServoAngle(L3, 10);
    setServoAngle(L4, 10);
    if (!interruptibleDelay(300)) return;
    setServoAngle(R4, 160);
    setServoAngle(R3, 160);
    setServoAngle(L3, 65);
    setServoAngle(L4, 65);
    if (!interruptibleDelay(300)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runSwimPose() {
  Serial.println("SWIM");
  runRestPose();
  for (int i = 0; i < 4; i++) {
    setServoAngle(R1, 135);
    setServoAngle(R2, 45);
    setServoAngle(L1, 45);
    setServoAngle(L2, 135);
    if (!interruptibleDelay(400)) return;
    setServoAngle(R1, 90);
    setServoAngle(R2, 90);
    setServoAngle(L1, 90);
    setServoAngle(L2, 90);
    if (!interruptibleDelay(400)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runPointPose() {
  Serial.println("POINT");
  setServoAngle(L2, 90);
  setServoAngle(R1, 135);
  setServoAngle(R2, 100);
  setServoAngle(L4, 180);
  setServoAngle(L1, 25);
  setServoAngle(L3, 145);
  setServoAngle(R4, 80);
  setServoAngle(R3, 170);
  if (!interruptibleDelay(2000)) return;
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runPushupPose() {
  Serial.println("PUSHUP");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(L1, 0);
  setServoAngle(R1, 180);
  setServoAngle(L3, 90);
  setServoAngle(R3, 90);
  if (!interruptibleDelay(500)) return;
  for (int i = 0; i < 4; i++) {
    setServoAngle(L3, 0);
    setServoAngle(R3, 180);
    if (!interruptibleDelay(600)) return;
    setServoAngle(L3, 90);
    setServoAngle(R3, 90);
    if (!interruptibleDelay(500)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runBowPose() {
  Serial.println("BOW");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(L1, 0);
  setServoAngle(R1, 180);
  setServoAngle(L3, 0);
  setServoAngle(R3, 180);
  setServoAngle(L2, 180);
  setServoAngle(R2, 0);
  setServoAngle(R4, 0);
  setServoAngle(L4, 180);
  if (!interruptibleDelay(600)) return;
  setServoAngle(L3, 90);
  setServoAngle(R3, 90);
  if (!interruptibleDelay(3000)) return;
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runCutePose() {
  Serial.println("CUTE");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(L2, 160);
  setServoAngle(R2, 20);
  setServoAngle(R4, 180);
  setServoAngle(L4, 0);
  setServoAngle(L1, 0);
  setServoAngle(R1, 180);
  setServoAngle(L3, 180);
  setServoAngle(R3, 0);
  if (!interruptibleDelay(200)) return;
  for (int i = 0; i < 5; i++) {
    setServoAngle(R4, 180);
    setServoAngle(L4, 45);
    if (!interruptibleDelay(300)) return;
    setServoAngle(R4, 135);
    setServoAngle(L4, 0);
    if (!interruptibleDelay(300)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runFreakyPose() {
  Serial.println("FREAKY");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(L1, 0);
  setServoAngle(R1, 180);
  setServoAngle(L2, 180);
  setServoAngle(R2, 0);
  setServoAngle(R4, 90);
  setServoAngle(R3, 0);
  if (!interruptibleDelay(200)) return;
  for (int i = 0; i < 3; i++) {
    setServoAngle(R3, 25);
    if (!interruptibleDelay(400)) return;
    setServoAngle(R3, 0);
    if (!interruptibleDelay(400)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runWormPose() {
  Serial.println("WORM");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(R1, 180);
  setServoAngle(R2, 0);
  setServoAngle(L1, 0);
  setServoAngle(L2, 180);
  setServoAngle(R4, 90);
  setServoAngle(R3, 90);
  setServoAngle(L3, 90);
  setServoAngle(L4, 90);
  if (!interruptibleDelay(200)) return;
  for (int i = 0; i < 5; i++) {
    setServoAngle(R3, 45);
    setServoAngle(L3, 135);
    setServoAngle(R4, 45);
    setServoAngle(L4, 135);
    if (!interruptibleDelay(300)) return;
    setServoAngle(R3, 135);
    setServoAngle(L3, 45);
    setServoAngle(R4, 135);
    setServoAngle(L4, 45);
    if (!interruptibleDelay(300)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runShakePose() {
  Serial.println("SHAKE");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(R1, 135);
  setServoAngle(L1, 45);
  setServoAngle(L3, 90);
  setServoAngle(R3, 90);
  setServoAngle(L2, 90);
  setServoAngle(R2, 90);
  if (!interruptibleDelay(200)) return;
  for (int i = 0; i < 5; i++) {
    setServoAngle(R4, 45);
    setServoAngle(L4, 135);
    if (!interruptibleDelay(300)) return;
    setServoAngle(R4, 0);
    setServoAngle(L4, 180);
    if (!interruptibleDelay(300)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runShrugPose() {
  Serial.println("SHRUG");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(R3, 90);
  setServoAngle(R4, 90);
  setServoAngle(L3, 90);
  setServoAngle(L4, 90);
  if (!interruptibleDelay(1000)) return;
  setServoAngle(R3, 0);
  setServoAngle(R4, 180);
  setServoAngle(L3, 180);
  setServoAngle(L4, 0);
  if (!interruptibleDelay(1500)) return;
  runStandPose();
  currentCommand = STATE_IDLE;
}

void runDeadPose() {
  Serial.println("DEAD");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(R3, 90);
  setServoAngle(R4, 90);
  setServoAngle(L3, 90);
  setServoAngle(L4, 90);
  currentCommand = STATE_IDLE;
}

void runCrabPose() {
  Serial.println("CRAB");
  runStandPose();
  if (!interruptibleDelay(200)) return;
  setServoAngle(R1, 90);
  setServoAngle(R2, 90);
  setServoAngle(L1, 90);
  setServoAngle(L2, 90);
  setServoAngle(R4, 0);
  setServoAngle(R3, 180);
  setServoAngle(L3, 45);
  setServoAngle(L4, 135);
  for (int i = 0; i < 5; i++) {
    setServoAngle(R4, 45);
    setServoAngle(R3, 135);
    setServoAngle(L3, 0);
    setServoAngle(L4, 180);
    if (!interruptibleDelay(300)) return;
    setServoAngle(R4, 0);
    setServoAngle(R3, 180);
    setServoAngle(L3, 45);
    setServoAngle(L4, 135);
    if (!interruptibleDelay(300)) return;
  }
  runStandPose();
  currentCommand = STATE_IDLE;
}

// ---------- MOVEMENT ANIMATIONS ----------
// Continuous gaits — loop() keeps calling these as long as the state holds. 
// pressingCheck() gates each frame and aborts if state changes.

void runWalkPose() {
  Serial.println("WALK FWD");

  setServoAngle(R3, 135);
  setServoAngle(L3, 45);
  setServoAngle(R2, 100);
  setServoAngle(L1, 25);

  if (!pressingCheck(STATE_FORWARD, frameDelay)) return;
  setServoAngle(R3, 135); setServoAngle(L3, 0);
  if (!pressingCheck(STATE_FORWARD, frameDelay)) return;
  setServoAngle(L4, 135); setServoAngle(L2, 90);
  setServoAngle(R4, 0);   setServoAngle(R1, 180);
  if (!pressingCheck(STATE_FORWARD, frameDelay)) return;
  setServoAngle(R2, 45);  setServoAngle(L1, 90);
  if (!pressingCheck(STATE_FORWARD, frameDelay)) return;
  setServoAngle(R4, 45);  setServoAngle(L4, 180);
  if (!pressingCheck(STATE_FORWARD, frameDelay)) return;
  setServoAngle(R3, 180); setServoAngle(L3, 45);
  setServoAngle(R2, 90);  setServoAngle(L1, 0);
  if (!pressingCheck(STATE_FORWARD, frameDelay)) return;
  setServoAngle(L2, 135); setServoAngle(R1, 90);
  if (!pressingCheck(STATE_FORWARD, frameDelay)) return;
}

void runWalkBackward() {
  Serial.println("WALK BACK");

  if (!pressingCheck(STATE_BACKWARD, frameDelay)) return;
  setServoAngle(R3, 135); setServoAngle(L3, 0);
  if (!pressingCheck(STATE_BACKWARD, frameDelay)) return;
  setServoAngle(L4, 135); setServoAngle(L2, 135);
  setServoAngle(R4, 0);   setServoAngle(R1, 90);
  if (!pressingCheck(STATE_BACKWARD, frameDelay)) return;
  setServoAngle(R2, 90);  setServoAngle(L1, 0);
  if (!pressingCheck(STATE_BACKWARD, frameDelay)) return;
  setServoAngle(R4, 45);  setServoAngle(L4, 180);
  if (!pressingCheck(STATE_BACKWARD, frameDelay)) return;
  setServoAngle(R3, 180); setServoAngle(L3, 45);
  setServoAngle(R2, 45);  setServoAngle(L1, 90);
  if (!pressingCheck(STATE_BACKWARD, frameDelay)) return;
  setServoAngle(L2, 90);  setServoAngle(R1, 180);
  if (!pressingCheck(STATE_BACKWARD, frameDelay)) return;
}

void runTurnLeft() {
  Serial.println("TURN LEFT");

  // Legset 1 (R1 L2)
  setServoAngle(R3, 135); setServoAngle(L4, 135);
  if (!pressingCheck(STATE_LEFT, frameDelay)) return;
  setServoAngle(R1, 180); setServoAngle(L2, 180);
  if (!pressingCheck(STATE_LEFT, frameDelay)) return;
  setServoAngle(R3, 180); setServoAngle(L4, 180);
  if (!pressingCheck(STATE_LEFT, frameDelay)) return;
  setServoAngle(R1, 135); setServoAngle(L2, 135);
  if (!pressingCheck(STATE_LEFT, frameDelay)) return;

  // Legset 2 (R2 L1)
  setServoAngle(R4, 45);  setServoAngle(L3, 45);
  if (!pressingCheck(STATE_LEFT, frameDelay)) return;
  setServoAngle(R2, 90);  setServoAngle(L1, 90);
  if (!pressingCheck(STATE_LEFT, frameDelay)) return;
  setServoAngle(R4, 0);   setServoAngle(L3, 0);
  if (!pressingCheck(STATE_LEFT, frameDelay)) return;
  setServoAngle(R2, 45);  setServoAngle(L1, 45);
  if (!pressingCheck(STATE_LEFT, frameDelay)) return;
}

void runTurnRight() {
  Serial.println("TURN RIGHT");

  // Legset 2 (R2 L1)
  setServoAngle(R4, 45);  setServoAngle(L3, 45);
  if (!pressingCheck(STATE_RIGHT, frameDelay)) return;
  setServoAngle(R2, 0);   setServoAngle(L1, 0);
  if (!pressingCheck(STATE_RIGHT, frameDelay)) return;
  setServoAngle(R4, 0);   setServoAngle(L3, 0);
  if (!pressingCheck(STATE_RIGHT, frameDelay)) return;
  setServoAngle(R2, 45);  setServoAngle(L1, 45);
  if (!pressingCheck(STATE_RIGHT, frameDelay)) return;

  // Legset 1 (R1 L2)
  setServoAngle(R3, 135); setServoAngle(L4, 135);
  if (!pressingCheck(STATE_RIGHT, frameDelay)) return;
  setServoAngle(R1, 90);  setServoAngle(L2, 90);
  if (!pressingCheck(STATE_RIGHT, frameDelay)) return;
  setServoAngle(R3, 180); setServoAngle(L4, 180);
  if (!pressingCheck(STATE_RIGHT, frameDelay)) return;
  setServoAngle(R1, 135); setServoAngle(L2, 135);
  if (!pressingCheck(STATE_RIGHT, frameDelay)) return;
}