#pragma once

#include <pebble.h>

#define ANTIALIASING true

// Colors
#define HOUR_HAND_COLOR GColorRed
#define MINUTE_HAND_COLOR GColorDarkGray
#define CLOCKFACE_COLOR GColorWhite
#define DECOR_COLOR GColorDarkGray
#define DAY_BOX_COLOR GColorRed
#define BLUETOOTH_COLOR GColorBlue
  
// General
#define THICK_LINE 3
#define THIN_LINE 1

// Day box
#define BUFFER_LENGTH 20
#define TEXT_BOX_WIDTH 44
#define TEXT_BOX_HEIGHT 18
#define TEXT_BOX_CORNER 5

// Hands
#define MINUTE_HAND_MARGIN 18
#define HOUR_HAND_MARGIN 28
#define CENTER_RADIUS 4

// Clockface shapes
#define TICK_LENGTH 7
#define NUM_OF_POINTS 360
  
// tweaking a bit to eliminate "ugly" results of rounding
#define EXTERNAL_MARGIN 2.4
#define INTERNAL_MARGIN 4.5

// Bluetooth 
#define BLUETOOTH_STEP 4
#define BLUETOOTH_HEIGHT 16
#define BLUETOOTH_WIDTH 8

// Simple struct for time (inherited from original example)
typedef struct {
  int hours;
  int minutes;
} Time;
