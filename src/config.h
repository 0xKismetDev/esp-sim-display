#pragma once

#define LVGL_PORT_ROTATION_DEGREE 270
#define SCREEN_W 480
#define SCREEN_H 320
#define SIM_NUM_FIELDS 26
#define SIM_LIVE_MS 3000
// A full rotated frame takes roughly 45 ms on this panel. Requesting one every
// 16 ms keeps the LVGL lock occupied continuously and starves touch sampling.
// Eighty milliseconds leaves a larger input window between full-frame panel
// transfers. Touch sampling shares the display bus/lock and can otherwise miss
// short taps or the early motion samples needed to classify a swipe.
#define SIM_FRAME_MS 80
#define SWITCH_LONG_PRESS_MS 700
#define GESTURE_LIMIT_PX 28
#define NAVIGATION_COOLDOWN_MS 400
#define CONTROL_PULSE_MS 100
#define CONTROL_RELEASE_GAP_MS 45
#define CONTROL_DEBOUNCE_MS 120
#define CONTROL_QUEUE_LENGTH 12
