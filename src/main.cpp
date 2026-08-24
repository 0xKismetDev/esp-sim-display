#include <Arduino.h>
#include <lvgl.h>

extern "C"
{
#include "bsp/display.h"
#include "bsp/esp_bsp.h"
#include "bsp/lv_port.h"
}

#include "config.h"
#include "sim_controls.h"
#include "sim_screen.h"
#include "telemetry.h"

SimTelemetry g_sim;

static constexpr int SCREEN_COUNT = 5;
static lv_obj_t *screens[SCREEN_COUNT];
static int mode = 0;
static volatile int switch_request = 0;
static uint32_t last_navigation_request_ms = 0;

static void request_screen_switch(int direction)
{
    uint32_t now = millis();
    // Loading a new screen while the finger is still moving can make LVGL
    // classify the tail of the same swipe on that screen. Accept at most one
    // navigation request per physical gesture/long press.
    if (last_navigation_request_ms != 0 &&
        now - last_navigation_request_ms < NAVIGATION_COOLDOWN_MS) return;
    last_navigation_request_ms = now;
    switch_request = direction;
}

static void gesture_cb(lv_event_t *)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_LEFT) request_screen_switch(1);
    else if (dir == LV_DIR_RIGHT) request_screen_switch(-1);
}

static void longpress_cb(lv_event_t *)
{
    request_screen_switch(1);
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
    else if (mode == 3) sim_timing_screen_update();
    else sim_controls_screen_update();
}

static void switch_mode(int direction)
{
    int previous = mode;
    mode = (mode + direction + SCREEN_COUNT) % SCREEN_COUNT;
    // A full-screen animated transition continuously flushes this panel and
    // starves touch sampling for the duration. An immediate load keeps input
    // available and makes consecutive swipes deterministic.
    lv_scr_load(screens[mode]);
    update_active();
    Serial.printf("[mode] %d -> %d\n", previous, mode);
}

void setup()
{
    sim_controls_begin();
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
    screens[4] = sim_controls_screen_create();
    for (int i = 0; i < SCREEN_COUNT - 1; ++i) attach_navigation(screens[i]);
    // Keep control buttons clickable. Gestures that begin outside a button still
    // navigate, while a tap can never also trigger the long-press page action.
    lv_obj_add_event_cb(screens[4], gesture_cb, LV_EVENT_GESTURE, nullptr);

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
    sim_controls_task();

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
