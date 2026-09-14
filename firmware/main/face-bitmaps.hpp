#pragma once

#include <Arduino.h>

// ======================================================================
// FACE BITMAPS
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
#define X(name) extern const unsigned char epd_bitmap_##name[] PROGMEM;
FACE_LIST
#undef X