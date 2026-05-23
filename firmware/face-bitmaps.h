#pragma once

#include <Arduino.h>

// ======================================================================
// --- FACE BITMAPS ---
// Bitmaps for the faces are stored here, can be generated with image2cpp
// Optional animated frames can be added by defining bitmaps with suffixes
// _1, _2, _3, ... (example: epd_bitmap_walk_1).
// ======================================================================

// X-Macro Pattern - Add or remove faces here (single source of truth)
#define FACE_LIST \
	X(walk) \
	X(rest) \
	X(swim) \
	X(dance) \
	X(wave) \
	X(point) \
	X(stand) \
	X(cute) \
	X(pushup) \
	X(freaky) \
	X(bow) \
	X(worm) \
	X(shake) \
	X(shrug) \
	X(dead) \
	X(crab) \
	X(defualt) \
	X(idle) \
	X(idle_blink) \
	X(happy) \
	X(talk_happy) \
	X(sad) \
	X(talk_sad) \
	X(angry) \
	X(talk_angry) \
	X(surprised) \
	X(talk_surprised) \
	X(sleepy) \
	X(talk_sleepy) \
	X(love) \
	X(talk_love) \
	X(excited) \
	X(talk_excited) \
	X(confused) \
	X(talk_confused) \
	X(thinking) \
	X(talk_thinking)

// Extern declarations for all base bitmaps
#define X(name) extern const unsigned char epd_bitmap_##name[] PROGMEM __attribute__((weak));
FACE_LIST
#undef X

// Extern declarations for animated frames
//		NOTE: Optional animated frames (weak symbols). If a frame is not defined, 
// 		its pointer will be null and the animation will stop at the base frame.
#define DECLARE_FACE_FRAMES(name) \
	extern const unsigned char epd_bitmap_##name##_1[] PROGMEM __attribute__((weak)); \
	extern const unsigned char epd_bitmap_##name##_2[] PROGMEM __attribute__((weak)); \
	extern const unsigned char epd_bitmap_##name##_3[] PROGMEM __attribute__((weak)); \
	extern const unsigned char epd_bitmap_##name##_4[] PROGMEM __attribute__((weak)); \
	extern const unsigned char epd_bitmap_##name##_5[] PROGMEM __attribute__((weak));

#define X(name) DECLARE_FACE_FRAMES(name)
FACE_LIST
#undef X
#undef DECLARE_FACE_FRAMES
