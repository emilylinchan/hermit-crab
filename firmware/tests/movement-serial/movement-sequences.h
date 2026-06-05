#pragma once

#include <Arduino.h>

// ======================================================================
// SERVO LAYOUT
// ======================================================================

// Physical location of each servo on Hermit
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

// ======================================================================
// STATE MACHINE TYPE
// ======================================================================

enum MovementState : uint8_t {
  STATE_IDLE,
  STATE_STAND,
  STATE_REST,
  STATE_FORWARD,
  STATE_BACKWARD,
  STATE_LEFT,
  STATE_RIGHT,
  STATE_WAVE,
  STATE_DANCE,
  STATE_SWIM,
  STATE_POINT,
  STATE_PUSHUP,
  STATE_BOW,
  STATE_CUTE,
  STATE_FREAKY,
  STATE_WORM,
  STATE_SHAKE,
  STATE_SHRUG,
  STATE_DEAD,
  STATE_CRAB
};

// ======================================================================
// EXTERNAL GLOBALS (defined in main.ino)
// ======================================================================

extern int           frameDelay;
extern MovementState currentCommand;

extern void setServoAngle(uint8_t channel, int angle);
extern bool pressingCheck(MovementState expectedState, int ms);
extern void checkSerial();
extern bool interruptibleDelay(int ms);

// ======================================================================
// POSE & MOVEMENT SEQUENCE PROTOTYPES
// ======================================================================

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