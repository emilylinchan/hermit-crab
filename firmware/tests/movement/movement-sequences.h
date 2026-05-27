#pragma once

#include <Arduino.h>

// Location of servo on Hermit
enum ServoName : uint8_t {
  R1 = 0, 
  R2 = 1,
  L1 = 2,
  L2 = 3,
  R4 = 4,
  R3 = 5,
  L3 = 6,
  L4 = 7
};

// External globals and helpers by pose and movement sequence functions
extern int frameDelay;
extern String currentCommand;

extern void setServoAngle(uint8_t channel, int angle);
extern bool pressingCheck(String cmd, int ms);

// Function prototypes for pose and movement sequences
void runRestPose();
void runStandPose();
void runWavePose();
void runDancePose();
void runSwimPose();
void runPointPose();
void runPushupPose();
void runBowPose();
void runCutePose();
void runFreakyPose();
void runWormPose();
void runShakePose();
void runShrugPose();
void runDeadPose();
void runCrabPose();
void runWalkPose();
void runWalkBackward();
void runTurnLeft();
void runTurnRight();