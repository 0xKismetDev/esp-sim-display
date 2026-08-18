#pragma once
#include <lvgl.h>

lv_obj_t *sim_screen_create();
lv_obj_t *sim_gt_screen_create();
lv_obj_t *sim_tyres_screen_create();
lv_obj_t *sim_timing_screen_create();
void sim_screen_update();
void sim_gt_screen_update();
void sim_tyres_screen_update();
void sim_timing_screen_update();
void sim_feed(char c);
