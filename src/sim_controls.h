#pragma once

#include <lvgl.h>

// Windows/game bindings use buttons 1-8; the Arduino API indexes them 0-7.
enum SimControlButton : unsigned char {
    CONTROL_TC_DOWN = 0,
    CONTROL_TC_UP,
    CONTROL_ABS_DOWN,
    CONTROL_ABS_UP,
    CONTROL_BIAS_DOWN,
    CONTROL_BIAS_UP,
    CONTROL_MAP_DOWN,
    CONTROL_MAP_UP,
};

void sim_controls_begin();
void sim_controls_task();
lv_obj_t *sim_controls_screen_create();
void sim_controls_screen_update();
