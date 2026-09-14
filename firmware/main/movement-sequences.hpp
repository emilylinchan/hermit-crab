#pragma once

#include <Arduino.h>

// ======================================================================
// SERVO LAYOUT
// ======================================================================

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
// KEYFRAME DATA MODEL
// ======================================================================

const uint8_t NC = 255;  // No change

struct Keyframe {
  uint8_t  angles[8];   // 0–180 deg per servo (ServoName index order), or NC
  uint16_t durationMs;  // Time allowed for this frame (travel + hold)
};

struct Sequence {
  const Keyframe* frames;
  uint8_t         frameCount;
  uint8_t         loopStart;    // First frame of the repeated section
  uint8_t         repeatCount;  // Times the repeated section plays (>= 1)
  bool            isGait;       // true = continuous, held-state cycle
  bool            standAtEnd;   // true = return to stand when finished
  bool            sticky;       // true = hold state after completion 
};

// ======================================================================
// EXTERNAL GLOBALS
// ======================================================================

extern void setServoTarget(uint8_t channel, int angle);
extern bool interruptibleDelay(int ms);
extern bool pressingCheck(MovementState expectedState, int ms);

// ======================================================================
// PROTOTYPES
// ======================================================================

/**
 * @brief Maps a state to its sequence.
 * @param state Movement state to look up.
 * @return Pointer to the matching Sequence, or nullptr if none (e.g. STATE_IDLE).
 */
const Sequence* lookupSequence(MovementState state);

/**
 * @brief Plays a pose sequence once.
 * @param seq Sequence to play.
 * @return true if it completed; false if a new command interrupted it.
 */
bool playPose(const Sequence& seq);

/**
 * @brief Plays one full cycle of a gait while `holdState` remains active.
 * @param seq Sequence to play.
 * @param holdState State that must stay active for the cycle to continue.
 * @return true if the cycle completed; false if aborted, in which case
 *         stand targets are applied as graceful recovery.
 */
bool playGaitCycle(const Sequence& seq, MovementState holdState);