#include "movement-sequences.h"

// Derives frameCount from the array itself
#define FRAME_COUNT(f) ((uint8_t)(sizeof(f) / sizeof(Keyframe)))

// ======================================================================
// STATIC POSES
// ======================================================================

static const Keyframe STAND_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180}, 400 },
};
const Sequence SEQ_STAND = { STAND_FRAMES, FRAME_COUNT(STAND_FRAMES), 0, 1, false, false, true };

static const Keyframe REST_FRAMES[] = {
  { { 90,  90,  90,  90,  90,  90,  90,  90}, 400 },
};
const Sequence SEQ_REST = { REST_FRAMES, FRAME_COUNT(REST_FRAMES), 0, 1, false, false, true };

static const Keyframe DEAD_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180}, 200 },  // stand
  { { NC,  NC,  NC,  NC,  90,  90,  90,  90},   0 },  // collapse
};
const Sequence SEQ_DEAD = { DEAD_FRAMES, FRAME_COUNT(DEAD_FRAMES), 0, 1, false, false, true };

// ======================================================================
// ANIMATED POSES
// ======================================================================

static const Keyframe WAVE_FRAMES[] = {
  //  R1   R2   L1   L2   R4   R3   L3   L4    ms
  { {135,  45,  45, 135,   0, 180,   0, 180}, 200 },  // stand
  { {170,  NC,  NC, 165,  NC,  NC,  NC,  NC}, 250 },  // shift weight back-right (feet stay planted)
  { { NC,  NC,  NC,  NC,  NC,  NC, 180,  NC}, 200 },  // raise arm
  { { NC,  NC,  NC,  NC,  NC,  NC, 180,  NC}, 300 },  // -- loop: arm up
  { { NC,  NC,  NC,  NC,  NC,  NC, 100,  NC}, 300 },  // -- loop: arm down
};
const Sequence SEQ_WAVE = { WAVE_FRAMES, FRAME_COUNT(WAVE_FRAMES), 1, 4, false, true, false };

static const Keyframe DANCE_FRAMES[] = {
  { { 90,  90,  90,  90, 160, 160,  10,  10}, 300 },  // crouch, shells out
  { { NC,  NC,  NC,  NC, 115, 115,  10,  10}, 300 },  // -- loop: right side dips
  { { NC,  NC,  NC,  NC, 160, 160,  65,  65}, 300 },  // -- loop: left side dips
};
const Sequence SEQ_DANCE = { DANCE_FRAMES, FRAME_COUNT(DANCE_FRAMES), 1, 5, false, true, false };

static const Keyframe SWIM_FRAMES[] = {
  { { 90,  90,  90,  90,  90,  90,  90,  90},   0 },  // rest posture
  { {135,  45,  45, 135,  NC,  NC,  NC,  NC}, 400 },  // -- loop: legs sweep out
  { { 90,  90,  90,  90,  NC,  NC,  NC,  NC}, 400 },  // -- loop: legs pull in
};
const Sequence SEQ_SWIM = { SWIM_FRAMES, FRAME_COUNT(SWIM_FRAMES), 1, 4, false, true, false };

static const Keyframe POINT_FRAMES[] = {
  { {135, 100,  25,  90,  80, 170, 145, 180}, 2000 }, // hold the point
};
const Sequence SEQ_POINT = { POINT_FRAMES, FRAME_COUNT(POINT_FRAMES), 0, 1, false, true, false };

static const Keyframe PUSHUP_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180}, 200 },  // stand
  { {180,  NC,   0,  NC,  NC,  90,  90,  NC}, 500 },  // front legs brace
  { { NC,  NC,  NC,  NC,  NC, 180,   0,  NC}, 600 },  // -- loop: down
  { { NC,  NC,  NC,  NC,  NC,  90,  90,  NC}, 500 },  // -- loop: up
};
const Sequence SEQ_PUSHUP = { PUSHUP_FRAMES, FRAME_COUNT(PUSHUP_FRAMES), 2, 4, false, true, false };

static const Keyframe BOW_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180},  200 },  // stand
  { {180,   0,   0, 180,   0, 180,   0, 180},  600 },  // fold legs under
  { { NC,  NC,  NC,  NC,  NC,  90,  90,  NC}, 3000 },  // dip head, hold bow
};
const Sequence SEQ_BOW = { BOW_FRAMES, FRAME_COUNT(BOW_FRAMES), 0, 1, false, true, false };

static const Keyframe CUTE_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180}, 200 },  // stand
  { {180,  20,   0, 160, 180,   0, 180,   0}, 200 },  // tuck in, shells up
  { { NC,  NC,  NC,  NC, 180,  NC,  NC,  45}, 300 },  // -- loop: wiggle right
  { { NC,  NC,  NC,  NC, 135,  NC,  NC,   0}, 300 },  // -- loop: wiggle left
};
const Sequence SEQ_CUTE = { CUTE_FRAMES, FRAME_COUNT(CUTE_FRAMES), 2, 5, false, true, false };

static const Keyframe FREAKY_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180}, 200 },  // stand
  { {180,   0,   0, 180,  90,   0,  NC,  NC}, 200 },  // splay, cock the arm
  { { NC,  NC,  NC,  NC,  NC,  25,  NC,  NC}, 400 },  // -- loop: twitch out
  { { NC,  NC,  NC,  NC,  NC,   0,  NC,  NC}, 400 },  // -- loop: twitch back
};
const Sequence SEQ_FREAKY = { FREAKY_FRAMES, FRAME_COUNT(FREAKY_FRAMES), 2, 3, false, true, false };

static const Keyframe WORM_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180}, 200 },  // stand
  { {180,   0,   0, 180,  90,  90,  90,  90}, 200 },  // flatten out
  { { NC,  NC,  NC,  NC,  45,  45, 135, 135}, 300 },  // -- loop: ripple one way
  { { NC,  NC,  NC,  NC, 135, 135,  45,  45}, 300 },  // -- loop: ripple back
};
const Sequence SEQ_WORM = { WORM_FRAMES, FRAME_COUNT(WORM_FRAMES), 2, 5, false, true, false };

static const Keyframe SHAKE_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180}, 200 },  // stand
  { {135,  90,  45,  90,  NC,  90,  90,  NC}, 200 },  // square up
  { { NC,  NC,  NC,  NC,  45,  NC,  NC, 135}, 300 },  // -- loop: shake right
  { { NC,  NC,  NC,  NC,   0,  NC,  NC, 180}, 300 },  // -- loop: shake left
};
const Sequence SEQ_SHAKE = { SHAKE_FRAMES, FRAME_COUNT(SHAKE_FRAMES), 2, 5, false, true, false };

static const Keyframe SHRUG_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180},  200 },  // stand
  { { NC,  NC,  NC,  NC,  90,  90,  90,  90}, 1000 },  // shells to midpoint
  { { NC,  NC,  NC,  NC, 180,   0, 180,   0}, 1500 },  // full shrug, hold
};
const Sequence SEQ_SHRUG = { SHRUG_FRAMES, FRAME_COUNT(SHRUG_FRAMES), 0, 1, false, true, false };

static const Keyframe CRAB_FRAMES[] = {
  { {135,  45,  45, 135,   0, 180,   0, 180}, 200 },  // stand
  { { 90,  90,  90,  90,   0, 180,  45, 135},   0 },  // square legs, ready claws
  { { NC,  NC,  NC,  NC,  45, 135,   0, 180}, 300 },  // -- loop: snip
  { { NC,  NC,  NC,  NC,   0, 180,  45, 135}, 300 },  // -- loop: snap
};
const Sequence SEQ_CRAB = { CRAB_FRAMES, FRAME_COUNT(CRAB_FRAMES), 2, 5, false, true, false };

// ======================================================================
// GAITS
// ======================================================================

static const Keyframe WALK_FWD_FRAMES[] = {
  { { NC, 100,  25,  NC,  NC, 135,  45,  NC}, 100 },
  { { NC,  NC,  NC,  NC,  NC, 135,   0,  NC}, 100 },
  { {180,  NC,  NC,  90,   0,  NC,  NC, 135}, 100 },
  { { NC,  45,  90,  NC,  NC,  NC,  NC,  NC}, 100 },
  { { NC,  NC,  NC,  NC,  45,  NC,  NC, 180}, 100 },
  { { NC,  90,   0,  NC,  NC, 180,  45,  NC}, 100 },
  { { 90,  NC,  NC, 135,  NC,  NC,  NC,  NC}, 100 },
};
const Sequence SEQ_WALK_FWD = { WALK_FWD_FRAMES, FRAME_COUNT(WALK_FWD_FRAMES), 0, 1, true, false, false };

static const Keyframe WALK_BACK_FRAMES[] = {
  { { NC,  NC,  NC,  NC,  NC, 135,   0,  NC}, 100 },
  { { 90,  NC,  NC, 135,   0,  NC,  NC, 135}, 100 },
  { { NC,  90,   0,  NC,  NC,  NC,  NC,  NC}, 100 },
  { { NC,  NC,  NC,  NC,  45,  NC,  NC, 180}, 100 },
  { { NC,  45,  90,  NC,  NC, 180,  45,  NC}, 100 },
  { {180,  NC,  NC,  90,  NC,  NC,  NC,  NC}, 100 },
};
const Sequence SEQ_WALK_BACK = { WALK_BACK_FRAMES, FRAME_COUNT(WALK_BACK_FRAMES), 0, 1, true, false, false };

static const Keyframe TURN_LEFT_FRAMES[] = {
  // Legset 1 (R1 L2)
  { { NC,  NC,  NC,  NC,  NC, 135,  NC, 135}, 100 },
  { {180,  NC,  NC, 180,  NC,  NC,  NC,  NC}, 100 },
  { { NC,  NC,  NC,  NC,  NC, 180,  NC, 180}, 100 },
  { {135,  NC,  NC, 135,  NC,  NC,  NC,  NC}, 100 },
  // Legset 2 (R2 L1)
  { { NC,  NC,  NC,  NC,  45,  NC,  45,  NC}, 100 },
  { { NC,  90,  90,  NC,  NC,  NC,  NC,  NC}, 100 },
  { { NC,  NC,  NC,  NC,   0,  NC,   0,  NC}, 100 },
  { { NC,  45,  45,  NC,  NC,  NC,  NC,  NC}, 100 },
};
const Sequence SEQ_TURN_LEFT = { TURN_LEFT_FRAMES, FRAME_COUNT(TURN_LEFT_FRAMES), 0, 1, true, false, false };

static const Keyframe TURN_RIGHT_FRAMES[] = {
  // Legset 2 (R2 L1)
  { { NC,  NC,  NC,  NC,  45,  NC,  45,  NC}, 100 },
  { { NC,   0,   0,  NC,  NC,  NC,  NC,  NC}, 100 },
  { { NC,  NC,  NC,  NC,   0,  NC,   0,  NC}, 100 },
  { { NC,  45,  45,  NC,  NC,  NC,  NC,  NC}, 100 },
  // Legset 1 (R1 L2)
  { { NC,  NC,  NC,  NC,  NC, 135,  NC, 135}, 100 },
  { { 90,  NC,  NC,  90,  NC,  NC,  NC,  NC}, 100 },
  { { NC,  NC,  NC,  NC,  NC, 180,  NC, 180}, 100 },
  { {135,  NC,  NC, 135,  NC,  NC,  NC,  NC}, 100 },
};
const Sequence SEQ_TURN_RIGHT = { TURN_RIGHT_FRAMES, FRAME_COUNT(TURN_RIGHT_FRAMES), 0, 1, true, false, false };

// ======================================================================
// STATE -> SEQUENCE DISPATCH
// ======================================================================

struct SequenceEntry {
  MovementState   state;
  const Sequence* seq;
};

static const SequenceEntry SEQUENCE_TABLE[] = {
  { STATE_STAND,    &SEQ_STAND      },
  { STATE_REST,     &SEQ_REST       },
  { STATE_FORWARD,  &SEQ_WALK_FWD   },
  { STATE_BACKWARD, &SEQ_WALK_BACK  },
  { STATE_LEFT,     &SEQ_TURN_LEFT  },
  { STATE_RIGHT,    &SEQ_TURN_RIGHT },
  { STATE_WAVE,     &SEQ_WAVE       },
  { STATE_DANCE,    &SEQ_DANCE      },
  { STATE_SWIM,     &SEQ_SWIM       },
  { STATE_POINT,    &SEQ_POINT      },
  { STATE_PUSHUP,   &SEQ_PUSHUP     },
  { STATE_BOW,      &SEQ_BOW        },
  { STATE_CUTE,     &SEQ_CUTE       },
  { STATE_FREAKY,   &SEQ_FREAKY     },
  { STATE_WORM,     &SEQ_WORM       },
  { STATE_SHAKE,    &SEQ_SHAKE      },
  { STATE_SHRUG,    &SEQ_SHRUG      },
  { STATE_DEAD,     &SEQ_DEAD       },
  { STATE_CRAB,     &SEQ_CRAB       },
};
static const int SEQUENCE_TABLE_SIZE = sizeof(SEQUENCE_TABLE) / sizeof(SEQUENCE_TABLE[0]);

const Sequence* lookupSequence(MovementState state) {
  for (int i = 0; i < SEQUENCE_TABLE_SIZE; i++) {
    if (SEQUENCE_TABLE[i].state == state) return SEQUENCE_TABLE[i].seq;
  }
  return nullptr;  // STATE_IDLE and anything unmapped
}

// ======================================================================
// PLAYBACK ENGINE
// ======================================================================

// Private helper to apply one keyframe's targets, skipping NC entries
static void applyKeyframe(const Keyframe& kf) {
  for (int i = 0; i < 8; i++) {
    if (kf.angles[i] != NC) setServoTarget(i, kf.angles[i]);
  }
}

bool playPose(const Sequence& seq) {
  // Intro: frames before the loop section, played once
  for (uint8_t f = 0; f < seq.loopStart; f++) {
    applyKeyframe(seq.frames[f]);
    if (!interruptibleDelay(seq.frames[f].durationMs)) return false;
  }
  // Loop body: remaining frames, played repeatCount times
  for (uint8_t r = 0; r < seq.repeatCount; r++) {
    for (uint8_t f = seq.loopStart; f < seq.frameCount; f++) {
      applyKeyframe(seq.frames[f]);
      if (!interruptibleDelay(seq.frames[f].durationMs)) return false;
    }
  }
  // Optional return back to stand position
  if (seq.standAtEnd) {
    applyKeyframe(STAND_FRAMES[0]);
    if (!interruptibleDelay(STAND_FRAMES[0].durationMs)) return false;
  }
  return true;
}

bool playGaitCycle(const Sequence& seq, MovementState holdState) {
  for (uint8_t f = 0; f < seq.frameCount; f++) {
    applyKeyframe(seq.frames[f]);
    // Apply stand targets if gait no longer held
    if (!pressingCheck(holdState, seq.frames[f].durationMs)) {
      applyKeyframe(STAND_FRAMES[0]);
      return false;
    }
  }
  return true;
}