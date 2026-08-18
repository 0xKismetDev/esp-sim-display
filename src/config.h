#pragma once

#define LVGL_PORT_ROTATION_DEGREE 270
#define SCREEN_W 480
#define SCREEN_H 320
#define SIM_NUM_FIELDS 26
#define SIM_LIVE_MS 3000
// A full rotated frame takes roughly 45 ms on this panel. Requesting one every
// 16 ms keeps the LVGL lock occupied continuously and starves touch sampling.
// Sixty milliseconds leaves a reliable input window between frames.
#define SIM_FRAME_MS 60
#define SWITCH_LONG_PRESS_MS 700
#define GESTURE_LIMIT_PX 28
