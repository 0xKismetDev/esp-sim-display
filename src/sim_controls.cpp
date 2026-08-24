#include "sim_controls.h"

#include <Arduino.h>
#include <USBHIDGamepad.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "config.h"
#include "telemetry.h"
#include "theme.h"

static USBHIDGamepad gamepad;
static lv_obj_t *controls_screen;
static lv_obj_t *tc_value;
static lv_obj_t *abs_value;
static QueueHandle_t control_queue;

enum HidPulseState : unsigned char {
    HID_IDLE,
    HID_PRESSING,
    HID_HOLDING,
    HID_RELEASING,
    HID_RELEASE_GAP,
};

static HidPulseState hid_state = HID_IDLE;
static unsigned char active_button;
static uint32_t state_deadline;
static uint32_t last_enqueue_ms[8];

static void pulse_button(unsigned char button)
{
    if (!control_queue || button >= 8) return;

    uint32_t now = millis();
    if (now - last_enqueue_ms[button] < CONTROL_DEBOUNCE_MS) return;

    // The callback runs in LVGL's task while HID transmission runs in loop().
    // A FreeRTOS queue makes that handoff atomic and preserves quick sequences.
    if (xQueueSend(control_queue, &button, 0) == pdTRUE)
        last_enqueue_ms[button] = now;
}

static void control_clicked(lv_event_t *event)
{
    unsigned char button = static_cast<unsigned char>(
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
    pulse_button(button);
}

static lv_obj_t *label_at(lv_obj_t *parent, const char *text, const lv_font_t *font,
                          lv_color_t color, int x, int y, int w, lv_text_align_t align)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, w);
    lv_obj_set_style_text_align(label, align, 0);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
    return label;
}

static lv_obj_t *control_button(lv_obj_t *parent, const char *text, int x, int y,
                                SimControlButton button)
{
    lv_obj_t *obj = lv_btn_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, 92, 50);
    lv_obj_set_style_radius(obj, 7, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1D2730), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x37617B), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x40505E), 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    // Let a horizontal gesture that starts on a button reach the screen's
    // navigation callback instead of being swallowed by the button object.
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(obj, control_clicked, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<uintptr_t>(button)));

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, C_WHITE, 0);
    lv_obj_center(label);
    return obj;
}

static void add_row(const char *name, const char *binding, int y,
                    SimControlButton down, SimControlButton up, lv_obj_t **value)
{
    control_button(controls_screen, "-", 18, y, down);
    control_button(controls_screen, "+", 370, y, up);
    label_at(controls_screen, name, &lv_font_montserrat_20, C_TEXT,
             122, y + 3, 236, LV_TEXT_ALIGN_CENTER);
    *value = label_at(controls_screen, binding, &lv_font_montserrat_12, C_MID,
                      122, y + 29, 236, LV_TEXT_ALIGN_CENTER);
}

void sim_controls_begin()
{
    control_queue = xQueueCreate(CONTROL_QUEUE_LENGTH, sizeof(unsigned char));
    gamepad.begin();
    // Send a neutral report once the host finishes enumerating.
    gamepad.send(0, 0, 0, 0, 0, 0, HAT_CENTER, 0);
}

void sim_controls_task()
{
    uint32_t now = millis();

    switch (hid_state) {
    case HID_IDLE:
        if (control_queue && xQueueReceive(control_queue, &active_button, 0) == pdTRUE)
            hid_state = HID_PRESSING;
        break;

    case HID_PRESSING:
        // send() returns false while the USB endpoint is busy. Keep retrying so
        // a touch can never become a silently lost button-down report.
        if (gamepad.send(0, 0, 0, 0, 0, 0, HAT_CENTER,
                         static_cast<uint32_t>(1) << active_button)) {
            state_deadline = now + CONTROL_PULSE_MS;
            hid_state = HID_HOLDING;
        }
        break;

    case HID_HOLDING:
        if (static_cast<int32_t>(now - state_deadline) >= 0)
            hid_state = HID_RELEASING;
        break;

    case HID_RELEASING:
        // Retry the neutral report as well; otherwise a missed release can
        // leave the simulator believing the control is held down.
        if (gamepad.send(0, 0, 0, 0, 0, 0, HAT_CENTER, 0)) {
            state_deadline = now + CONTROL_RELEASE_GAP_MS;
            hid_state = HID_RELEASE_GAP;
        }
        break;

    case HID_RELEASE_GAP:
        if (static_cast<int32_t>(now - state_deadline) >= 0)
            hid_state = HID_IDLE;
        break;
    }
}

lv_obj_t *sim_controls_screen_create()
{
    controls_screen = lv_obj_create(nullptr);
    lv_obj_remove_style_all(controls_screen);
    lv_obj_set_size(controls_screen, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(controls_screen, lv_color_hex(0x080A0D), 0);
    lv_obj_set_style_bg_opa(controls_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(controls_screen, LV_OBJ_FLAG_SCROLLABLE);

    label_at(controls_screen, "CAR CONTROLS", &lv_font_montserrat_14, C_DIM,
             18, 5, 444, LV_TEXT_ALIGN_CENTER);
    add_row("TRACTION CONTROL", "TC --", 27, CONTROL_TC_DOWN, CONTROL_TC_UP, &tc_value);
    add_row("ABS", "ABS --", 91, CONTROL_ABS_DOWN, CONTROL_ABS_UP, &abs_value);
    lv_obj_t *unused;
    add_row("BRAKE BIAS", "GAME BINDING", 155, CONTROL_BIAS_DOWN, CONTROL_BIAS_UP, &unused);
    add_row("ENGINE MAP", "GAME BINDING", 219, CONTROL_MAP_DOWN, CONTROL_MAP_UP, &unused);
    label_at(controls_screen, "SWIPE TO CHANGE PAGE", &lv_font_montserrat_10, C_DIM,
             18, 297, 444, LV_TEXT_ALIGN_CENTER);
    return controls_screen;
}

void sim_controls_screen_update()
{
    char text[16];
    if (g_sim.tcLevel < 0) snprintf(text, sizeof(text), "TC --");
    else snprintf(text, sizeof(text), "TC %d", g_sim.tcLevel);
    lv_label_set_text(tc_value, text);
    lv_obj_set_style_text_color(tc_value, g_sim.tcLevel < 0 ? C_DIM : C_MID, 0);

    if (g_sim.absLevel < 0) snprintf(text, sizeof(text), "ABS --");
    else snprintf(text, sizeof(text), "ABS %d", g_sim.absLevel);
    lv_label_set_text(abs_value, text);
    lv_obj_set_style_text_color(abs_value, g_sim.absLevel < 0 ? C_DIM : C_MID, 0);
}
