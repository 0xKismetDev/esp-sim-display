#include <Arduino.h>
#include <lvgl.h>

extern "C"
{
#include "bsp/display.h"
#include "bsp/esp_bsp.h"
#include "bsp/lv_port.h"
}

#include "config.h"
#include "sim_screen.h"
#include "telemetry.h"

SimTelemetry g_sim;

static lv_obj_t *screens[4];
static int mode = 0;
static volatile int switch_request = 0;

static void gesture_cb(lv_event_t *)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_LEFT) switch_request = 1;
    else if (dir == LV_DIR_RIGHT) switch_request = -1;
}

static void longpress_cb(lv_event_t *)
{
    switch_request = 1;
}

static void make_touch_transparent(lv_obj_t *obj)
{
    uint32_t count = lv_obj_get_child_cnt(obj);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        lv_obj_clear_flag(child, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        make_touch_transparent(child);
    }
}

static void attach_navigation(lv_obj_t *screen)
{
    make_touch_transparent(screen);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, gesture_cb, LV_EVENT_GESTURE, nullptr);
    lv_obj_add_event_cb(screen, longpress_cb, LV_EVENT_LONG_PRESSED, nullptr);
}

static void update_active()
{
    if (mode == 0) sim_screen_update();
    else if (mode == 1) sim_gt_screen_update();
    else if (mode == 2) sim_tyres_screen_update();
    else sim_timing_screen_update();
}

static void switch_mode(int direction)
{
    int previous = mode;
    mode = (mode + direction + 4) % 4;
    lv_scr_load_anim(screens[mode], direction > 0 ? LV_SCR_LOAD_ANIM_MOVE_LEFT
                                                  : LV_SCR_LOAD_ANIM_MOVE_RIGHT,
                     180, 0, false);
    update_active();
    Serial.printf("[mode] %d -> %d\n", previous, mode);
}

void setup()
{
    Serial.begin(115200);
    delay(100);

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
#if LVGL_PORT_ROTATION_DEGREE == 270
        .rotate = LV_DISP_ROT_270,
#elif LVGL_PORT_ROTATION_DEGREE == 90
        .rotate = LV_DISP_ROT_90,
#else
        .rotate = LV_DISP_ROT_NONE,
#endif
    };

    lv_disp_t *disp = bsp_display_start_with_config(&cfg);
    if (!disp) while (true) delay(1000);
    bsp_display_backlight_on();

    bsp_display_lock(0);
    screens[0] = sim_screen_create();
    screens[1] = sim_gt_screen_create();
    screens[2] = sim_tyres_screen_create();
    screens[3] = sim_timing_screen_create();
    for (lv_obj_t *screen : screens) attach_navigation(screen);

    lv_indev_t *indev = lv_indev_get_next(nullptr);
    if (indev) {
        indev->driver->long_press_time = SWITCH_LONG_PRESS_MS;
        indev->driver->gesture_limit = GESTURE_LIMIT_PX;
    }
    lv_scr_load(screens[0]);
    update_active();
    bsp_display_unlock();
    Serial.println("[boot] ready");
}

void loop()
{
    while (Serial.available()) sim_feed(Serial.read());

    if (switch_request != 0) {
        int request = switch_request;
        switch_request = 0;
        if (bsp_display_lock(20)) {
            switch_mode(request);
            bsp_display_unlock();
        }
    }

    static uint32_t last_draw = 0;
    uint32_t now = millis();
    if ((g_sim.updated || g_sim.isLive()) && now - last_draw >= SIM_FRAME_MS) {
        last_draw = now;
        g_sim.updated = false;
        if (bsp_display_lock(10)) {
            update_active();
            bsp_display_unlock();
        }
    }
    delay(1);
}
