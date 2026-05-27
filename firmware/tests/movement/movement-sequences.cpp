#include "movement-sequences.h"

static const String ServoNames[] = {"R1","R2","L1","L2","R4","R3","L3","L4"};

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

// ---------- POSES ----------
void runRestPose() { 
  Serial.println("REST"); 
  for (int i = 0; i < 8; i++) {
    setServoAngle(i, 90); 
  }
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
}

void runWavePose() { 
  Serial.println("WAVE"); 
  runStandPose(); 
  delay(200);
  setServoAngle(R4, 80); 
  setServoAngle(L3, 180); 
  setServoAngle(L2, 90); 
  setServoAngle(R1, 100); 
  delay(200);
  setServoAngle(L3, 180); 
  delay(300); 
  for (int i = 0; i < 4; i++) { 
    setServoAngle(L3, 180); 
    delay(300); 
    setServoAngle(L3, 100); 
    delay(300); 
  } 
  runStandPose(); 
  if (currentCommand == "wave") currentCommand = "";
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
  delay(300); 
  for (int i = 0; i < 5; i++) { 
    setServoAngle(R4, 115); 
    setServoAngle(R3, 115); 
    setServoAngle(L3, 10); 
    setServoAngle(L4, 10); 
    delay(300); 
    setServoAngle(R4, 160); 
    setServoAngle(R3, 160); 
    setServoAngle(L3, 65); 
    setServoAngle(L4, 65); 
    delay(300); 
  } 
  runStandPose(); 
  if (currentCommand == "dance") currentCommand = "";
}

void runSwimPose() { 
  Serial.println("SWIM"); 
  for (int i = 0; i < 8; i++) setServoAngle(i, 90); // should i replace this with calling runRestPose()?
  for (int i = 0; i < 4; i++) { 
    setServoAngle(R1, 135); 
    setServoAngle(R2, 45); 
    setServoAngle(L1, 45); 
    setServoAngle(L2, 135); 
    delay(400); 
    setServoAngle(R1, 90); 
    setServoAngle(R2, 90); 
    setServoAngle(L1, 90); 
    setServoAngle(L2, 90); 
    delay(400); 
  } 
  runStandPose(); 
  if (currentCommand == "swim") currentCommand = "";
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
  delay(2000); 
  runStandPose(); 
  if (currentCommand == "point") currentCommand = "";
}

void runPushupPose() {
  Serial.println("PUSHUP");
  runStandPose(); 
  delay(200);
  setServoAngle(L1, 0);
  setServoAngle(R1, 180);
  setServoAngle(L3, 90);
  setServoAngle(R3, 90);
  delay(500);
  for (int i = 0; i < 4; i++) {
    setServoAngle(L3, 0);
    setServoAngle(R3, 180);
    delay(600);
    setServoAngle(L3, 90);
    setServoAngle(R3, 90);
    delay(500);
  }
  runStandPose();
  if (currentCommand == "pushup") currentCommand = "";
}

void runBowPose() {
  Serial.println("BOW");
  runStandPose(); 
  delay(200);
  setServoAngle(L1, 0);
  setServoAngle(R1, 180);
  setServoAngle(L3, 0);
  setServoAngle(R3, 180);
  setServoAngle(L2, 180);
  setServoAngle(R2, 0);
  setServoAngle(R4, 0);
  setServoAngle(L4, 180);
  delay(600);
  setServoAngle(L3, 90);
  setServoAngle(R3, 90);
  delay(3000);
  runStandPose();
  if (currentCommand == "bow") currentCommand = "";
}

void runCutePose() {
  Serial.println(F("CUTE"));
  runStandPose(); 
  delay(200);
  setServoAngle(L2, 160);
  setServoAngle(R2, 20);
  setServoAngle(R4, 180);
  setServoAngle(L4, 0);
  setServoAngle(L1, 0);
  setServoAngle(R1, 180);
  setServoAngle(L3, 180);
  setServoAngle(R3, 0);
  delay(200);
  for (int i = 0; i < 5; i++) {
    setServoAngle(R4, 180);
    setServoAngle(L4, 45);
    delay(300);
    setServoAngle(R4, 135);
    setServoAngle(L4, 0);
    delay(300);
  }
  runStandPose();
  if (currentCommand == "cute") currentCommand = "";
}

void runFreakyPose() {
  Serial.println("FREAKY");
  runStandPose(); 
  delay(200);
  setServoAngle(L1, 0);
  setServoAngle(R1, 180);
  setServoAngle(L2, 180);
  setServoAngle(R2, 0);
  setServoAngle(R4, 90);
  setServoAngle(R3, 0);
  delay(200);
  for (int i = 0; i < 3; i++) {
    setServoAngle(R3, 25);
    delay(400);
    setServoAngle(R3, 0);
    delay(400);
  }
  runStandPose();
  if (currentCommand == "freaky") currentCommand = "";
}

void runWormPose() {
  Serial.println("WORM");
  runStandPose();
  delay(200);
  setServoAngle(R1, 180); 
  setServoAngle(R2, 0); 
  setServoAngle(L1, 0); 
  setServoAngle(L2, 180);
  setServoAngle(R4, 90); 
  setServoAngle(R3, 90); 
  setServoAngle(L3, 90); 
  setServoAngle(L4, 90);
  delay(200);
  for(int i = 0; i < 5; i++) {
    setServoAngle(R3, 45); 
    setServoAngle(L3, 135); 
    setServoAngle(R4, 45); 
    setServoAngle(L4, 135);
    delay(300);
    setServoAngle(R3, 135); 
    setServoAngle(L3, 45); 
    setServoAngle(R4, 135); 
    setServoAngle(L4, 45);
    delay(300);
  }
  runStandPose();
  if (currentCommand == "worm") currentCommand = "";
}

void runShakePose() {
  Serial.println("SHAKE");
  runStandPose();
  delay(200);
  setServoAngle(R1, 135); 
  setServoAngle(L1, 45); 
  setServoAngle(L3, 90); 
  setServoAngle(R3, 90);
  setServoAngle(L2, 90); 
  setServoAngle(R2, 90);
  delay(200);
  for(int i = 0; i < 5; i++) {
    setServoAngle(R4, 45); 
    setServoAngle(L4, 135);
    delay(300);
    setServoAngle(R4, 0); 
    setServoAngle(L4, 180);
    delay(300);
  }
  runStandPose();
  if (currentCommand == "shake") currentCommand = "";
}

inline void runShrugPose() {
  Serial.println("SHRUG");
  runStandPose();
  delay(200);
  setServoAngle(R3, 90); 
  setServoAngle(R4, 90); 
  setServoAngle(L3, 90); 
  setServoAngle(L4, 90);
  delay(1000);
  setServoAngle(R3, 0); 
  setServoAngle(R4, 180); 
  setServoAngle(L3, 180); 
  setServoAngle(L4, 0);
  delay(1500);
  runStandPose();
  if (currentCommand == "shrug") currentCommand = "";
}

void runDeadPose() {
  Serial.println("DEAD");
  runStandPose();
  delay(200);
  setServoAngle(R3, 90); 
  setServoAngle(R4, 90); 
  setServoAngle(L3, 90); 
  setServoAngle(L4, 90);
  if (currentCommand == "dead") currentCommand = "";
}

void runCrabPose() {
  Serial.println("CRAB");
  runStandPose();
  delay(200);
  setServoAngle(R1, 90); 
  setServoAngle(R2, 90); 
  setServoAngle(L1, 90); 
  setServoAngle(L2, 90);
  setServoAngle(R4, 0);
  setServoAngle(R3, 180); 
  setServoAngle(L3, 45); 
  setServoAngle(L4, 135);
  for(int i=0; i<5; i++) {
    setServoAngle(R4, 45); 
    setServoAngle(R3, 135); 
    setServoAngle(L3, 0); 
    setServoAngle(L4, 180);
    delay(300);
    setServoAngle(R4, 0); 
    setServoAngle(R3, 180); 
    setServoAngle(L3, 45); 
    setServoAngle(L4, 135);
    delay(300);
  }
  runStandPose();
  if (currentCommand == "crab") currentCommand = "";
}

// --- MOVEMENT ANIMATIONS ---
void runWalkPose() {
  Serial.println("WALK FWD");
  
  setServoAngle(R3, 135); 
  setServoAngle(L3, 45);
  setServoAngle(R2, 100); 
  setServoAngle(L1, 25);
 
  if (!pressingCheck("forward", frameDelay)) return;
  setServoAngle(R3, 135); setServoAngle(L3, 0);
  if (!pressingCheck("forward", frameDelay)) return;
  setServoAngle(L4, 135); setServoAngle(L2, 90);
  setServoAngle(R4, 0); setServoAngle(R1, 180);
  if (!pressingCheck("forward", frameDelay)) return;    
  setServoAngle(R2, 45); setServoAngle(L1, 90);
  if (!pressingCheck("forward", frameDelay)) return;
  setServoAngle(R4, 45); setServoAngle(L4, 180);
  if (!pressingCheck("forward", frameDelay)) return;
  setServoAngle(R3, 180); setServoAngle(L3, 45);
  setServoAngle(R2, 90); setServoAngle(L1, 0);
  if (!pressingCheck("forward", frameDelay)) return;  
  setServoAngle(L2, 135); setServoAngle(R1, 90);
  if (!pressingCheck("forward", frameDelay)) return;
}

void runWalkBackward() {
  Serial.println("WALK BACK");

  if (!pressingCheck("backward", frameDelay)) return;
  setServoAngle(R3, 135); setServoAngle(L3, 0);
  if (!pressingCheck("backward", frameDelay)) return;
  setServoAngle(L4, 135); setServoAngle(L2, 135);
  setServoAngle(R4, 0); setServoAngle(R1, 90);
  if (!pressingCheck("backward", frameDelay)) return;    
  setServoAngle(R2, 90); setServoAngle(L1, 0);
  if (!pressingCheck("backward", frameDelay)) return;
  setServoAngle(R4, 45); setServoAngle(L4, 180);
  if (!pressingCheck("backward", frameDelay)) return;
  setServoAngle(R3, 180); setServoAngle(L3, 45);
  setServoAngle(R2, 45); setServoAngle(L1, 90);
  if (!pressingCheck("backward", frameDelay)) return;  
  setServoAngle(L2, 90); setServoAngle(R1, 180);
  if (!pressingCheck("backward", frameDelay)) return;
}

void runTurnLeft() {
  Serial.println("TURN LEFT");

  // Legset 1 (R1 L2)
  setServoAngle(R3, 135); setServoAngle(L4, 135); 
  if (!pressingCheck("left", frameDelay)) return;
  setServoAngle(R1, 180); setServoAngle(L2, 180); 
  if (!pressingCheck("left", frameDelay)) return;
  setServoAngle(R3, 180); setServoAngle(L4, 180); 
  if (!pressingCheck("left", frameDelay)) return;
  setServoAngle(R1, 135); setServoAngle(L2, 135);
  if (!pressingCheck("left", frameDelay)) return;

  // Legset 2 (R2 L1)
  setServoAngle(R4, 45); setServoAngle(L3, 45); 
  if (!pressingCheck("left", frameDelay)) return;
  setServoAngle(R2, 90); setServoAngle(L1, 90); 
  if (!pressingCheck("left", frameDelay)) return;
  setServoAngle(R4, 0); setServoAngle(L3, 0); 
  if (!pressingCheck("left", frameDelay)) return;
  setServoAngle(R2, 45); setServoAngle(L1, 45);
  if (!pressingCheck("left", frameDelay)) return;  
}

void runTurnRight() {
  Serial.println("TURN RIGHT");

  // Legset 2 (R2 L1)
  setServoAngle(R4, 45); setServoAngle(L3, 45); 
  if (!pressingCheck("right", frameDelay)) return;
  setServoAngle(R2, 0); setServoAngle(L1, 0); 
  if (!pressingCheck("right", frameDelay)) return;
  setServoAngle(R4, 0); setServoAngle(L3, 0); 
  if (!pressingCheck("right", frameDelay)) return;
  setServoAngle(R2, 45); setServoAngle(L1, 45);
  if (!pressingCheck("right", frameDelay)) return;  

  // Legset 1 (R1 L2)
  setServoAngle(R3, 135); setServoAngle(L4, 135); 
  if (!pressingCheck("right", frameDelay)) return;
  setServoAngle(R1, 90); setServoAngle(L2, 90); 
  if (!pressingCheck("right", frameDelay)) return;
  setServoAngle(R3, 180); setServoAngle(L4, 180); 
  if (!pressingCheck("right", frameDelay)) return;
  setServoAngle(R1, 135); setServoAngle(L2, 135);
  if (!pressingCheck("right", frameDelay)) return;
}
