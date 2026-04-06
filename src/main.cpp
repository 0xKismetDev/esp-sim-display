#include <Arduino.h>
#include <lvgl.h>

extern "C"
{
#include "bsp/display.h"
#include "bsp/esp_bsp.h"
#include "bsp/lv_port.h"
}

#define LVGL_PORT_ROTATION_DEGREE 90
#define SCREEN_W 480
#define SCREEN_H 320
#define NUM_FIELDS 5

static int tel_speed = 0;
static char tel_gear[4] = "N";
static int tel_rpm = 0;
static int tel_max_rpm = 8000;
static int tel_redline_pct = 95;
static int tel_rpm_pct = 0;
static bool tel_updated = false;

static lv_obj_t *rpm_bar;
static lv_obj_t *gear_label;
static lv_obj_t *speed_label;
static lv_obj_t *rpm_label;
static lv_obj_t *shift_lights[9];

#define C_BG lv_color_hex(0x000000)
#define C_GREEN lv_color_hex(0x00FF00)
#define C_YELLOW lv_color_hex(0xFFCC00)
#define C_RED lv_color_hex(0xFF0000)
#define C_ORANGE lv_color_hex(0xFF8800)
#define C_DIM_GREEN lv_color_hex(0x002800)
#define C_DIM_YELLOW lv_color_hex(0x282800)
#define C_DIM_RED lv_color_hex(0x280000)
#define C_WHITE lv_color_hex(0xFFFFFF)
#define C_GREY lv_color_hex(0x666666)
#define C_DARK lv_color_hex(0x1A1A1A)

static void create_dashboard()
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, C_BG, 0);

    rpm_bar = lv_bar_create(scr);
    lv_obj_set_size(rpm_bar, 440, 22);
    lv_obj_align(rpm_bar, LV_ALIGN_TOP_MID, 0, 10);
    lv_bar_set_range(rpm_bar, 0, 100);
    lv_bar_set_value(rpm_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(rpm_bar, C_DARK, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rpm_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rpm_bar, C_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(rpm_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(rpm_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(rpm_bar, 4, LV_PART_INDICATOR);

    int total_w = 8 * 44;
    int start_x = (SCREEN_W - total_w) / 2;
    for (int i = 0; i < 9; i++)
    {
        shift_lights[i] = lv_obj_create(scr);
        lv_obj_remove_style_all(shift_lights[i]);
        lv_obj_set_size(shift_lights[i], 22, 22);
        lv_obj_set_pos(shift_lights[i], start_x + i * 44, 46);
        lv_obj_set_style_radius(shift_lights[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(shift_lights[i], LV_OPA_COVER, 0);
        lv_obj_set_scrollbar_mode(shift_lights[i], LV_SCROLLBAR_MODE_OFF);

        lv_color_t dim;
        if (i < 3)
            dim = C_DIM_GREEN;
        else if (i < 6)
            dim = C_DIM_YELLOW;
        else
            dim = C_DIM_RED;
        lv_obj_set_style_bg_color(shift_lights[i], dim, 0);
    }

    gear_label = lv_label_create(scr);
    lv_label_set_text(gear_label, "N");
    lv_obj_set_style_text_font(gear_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(gear_label, C_GREY, 0);
    lv_obj_align(gear_label, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *line1 = lv_obj_create(scr);
    lv_obj_remove_style_all(line1);
    lv_obj_set_size(line1, 400, 1);
    lv_obj_set_style_bg_color(line1, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(line1, LV_OPA_COVER, 0);
    lv_obj_align(line1, LV_ALIGN_TOP_MID, 0, 78);

    lv_obj_t *line2 = lv_obj_create(scr);
    lv_obj_remove_style_all(line2);
    lv_obj_set_size(line2, 400, 1);
    lv_obj_set_style_bg_color(line2, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(line2, LV_OPA_COVER, 0);
    lv_obj_align(line2, LV_ALIGN_TOP_MID, 0, 220);

    speed_label = lv_label_create(scr);
    lv_label_set_text(speed_label, "0");
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(speed_label, C_WHITE, 0);
    lv_obj_align(speed_label, LV_ALIGN_BOTTOM_MID, -80, -42);

    lv_obj_t *unit_kmh = lv_label_create(scr);
    lv_label_set_text(unit_kmh, "km/h");
    lv_obj_set_style_text_font(unit_kmh, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(unit_kmh, C_GREY, 0);
    lv_obj_align(unit_kmh, LV_ALIGN_BOTTOM_MID, -80, -22);

    rpm_label = lv_label_create(scr);
    lv_label_set_text(rpm_label, "0");
    lv_obj_set_style_text_font(rpm_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(rpm_label, C_WHITE, 0);
    lv_obj_align(rpm_label, LV_ALIGN_BOTTOM_MID, 80, -42);

    lv_obj_t *unit_rpm = lv_label_create(scr);
    lv_label_set_text(unit_rpm, "rpm");
    lv_obj_set_style_text_font(unit_rpm, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(unit_rpm, C_GREY, 0);
    lv_obj_align(unit_rpm, LV_ALIGN_BOTTOM_MID, 80, -22);
}

static unsigned long last_blink_ms = 0;
static bool blink_state = false;

static void update_dashboard()
{
    int pct = tel_rpm_pct;
    int red = tel_redline_pct;

    lv_bar_set_value(rpm_bar, pct, LV_ANIM_OFF);
    lv_color_t bar_col;
    if (pct >= red)
        bar_col = C_RED;
    else if (pct >= red * 85 / 100)
        bar_col = C_ORANGE;
    else if (pct >= red * 70 / 100)
        bar_col = C_YELLOW;
    else
        bar_col = C_GREEN;
    lv_obj_set_style_bg_color(rpm_bar, bar_col, LV_PART_INDICATOR);

    bool at_redline = (pct >= red);
    int light_range_start = red - 20;
    if (light_range_start < 0)
        light_range_start = 0;
    int step = 20 / 9;
    if (step < 1)
        step = 1;

    unsigned long now_ms = millis();
    if (at_redline && now_ms - last_blink_ms >= 60)
    {
        blink_state = !blink_state;
        last_blink_ms = now_ms;
    }
    else if (!at_redline)
    {
        blink_state = false;
    }

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, at_redline && blink_state ? C_RED : C_BG, 0);

    for (int i = 0; i < 9; i++)
    {
        if (at_redline)
        {
            lv_obj_set_style_bg_color(shift_lights[i], blink_state ? C_RED : C_BG, 0);
        }
        else
        {
            int threshold = light_range_start + i * step;
            bool lit = pct >= threshold;
            lv_color_t on, dim;
            if (i < 3)
            {
                on = C_GREEN;
                dim = C_DIM_GREEN;
            }
            else if (i < 6)
            {
                on = C_YELLOW;
                dim = C_DIM_YELLOW;
            }
            else
            {
                on = C_RED;
                dim = C_DIM_RED;
            }
            lv_obj_set_style_bg_color(shift_lights[i], lit ? on : dim, 0);
        }
    }

    lv_label_set_text(gear_label, tel_gear);
    lv_obj_align(gear_label, LV_ALIGN_CENTER, 0, -20);
    lv_color_t gear_col = C_WHITE;
    if (at_redline)
        gear_col = C_RED;
    else if (tel_gear[0] == 'N')
        gear_col = C_GREY;
    else if (tel_gear[0] == 'R')
        gear_col = C_ORANGE;
    lv_obj_set_style_text_color(gear_label, gear_col, 0);

    char spd[8];
    snprintf(spd, sizeof(spd), "%d", tel_speed);
    lv_label_set_text(speed_label, spd);
    lv_obj_align(speed_label, LV_ALIGN_BOTTOM_MID, -80, -42);

    char rpm_str[8];
    snprintf(rpm_str, sizeof(rpm_str), "%d", tel_rpm);
    lv_label_set_text(rpm_label, rpm_str);
    lv_obj_align(rpm_label, LV_ALIGN_BOTTOM_MID, 80, -42);
}

// SimHub sends fields in a continuous stream with no line terminator, auto syncc on gear field
static char field_buf[16];
static int field_pos = 0;
static int field_idx = 0;

static bool is_gear_value(const char *s)
{
    if (s[0] == '\0' || s[1] != '\0')
        return false;
    char c = s[0];
    return (c == 'N' || c == 'R' || (c >= '1' && c <= '8'));
}

static void process_field(const char *val)
{
    if (is_gear_value(val))
        field_idx = 1;

    switch (field_idx)
    {
    case 0:
    {
        int v = atoi(val);
        if (v >= 0 && v <= 999)
            tel_speed = v;
        break;
    }
    case 1:
        strncpy(tel_gear, val, sizeof(tel_gear) - 1);
        tel_gear[sizeof(tel_gear) - 1] = '\0';
        break;
    case 2:
    {
        int v = atoi(val);
        if (v >= 0 && v <= 30000)
            tel_rpm = v;
        break;
    }
    case 3:
    {
        int v = atoi(val);
        if (v >= 100 && v <= 30000)
            tel_max_rpm = v;
        break;
    }
    case 4:
    {
        int v = atoi(val);
        if (v >= 1 && v <= 100)
            tel_redline_pct = v;
        tel_rpm_pct = (tel_max_rpm > 0) ? (tel_rpm * 100) / tel_max_rpm : 0;
        if (tel_rpm_pct > 100)
            tel_rpm_pct = 100;
        tel_updated = true;
        break;
    }
    }
    field_idx = (field_idx + 1) % NUM_FIELDS;
}

static void parse_serial()
{
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == ';' || c == '\n' || c == '\r')
        {
            field_buf[field_pos] = '\0';
            if (field_pos > 0)
                process_field(field_buf);
            field_pos = 0;
        }
        else if (field_pos < (int)sizeof(field_buf) - 1)
        {
            field_buf[field_pos++] = c;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(100);

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
#if LVGL_PORT_ROTATION_DEGREE == 90
        .rotate = LV_DISP_ROT_90,
#elif LVGL_PORT_ROTATION_DEGREE == 270
        .rotate = LV_DISP_ROT_270,
#elif LVGL_PORT_ROTATION_DEGREE == 180
        .rotate = LV_DISP_ROT_180,
#else
        .rotate = LV_DISP_ROT_NONE,
#endif
    };

    lv_disp_t *disp = bsp_display_start_with_config(&cfg);
    if (!disp)
    {
        while (1)
            delay(1000);
    }
    bsp_display_backlight_on();

    bsp_display_lock(0);
    create_dashboard();
    bsp_display_unlock();
}

static unsigned long lastUpdate = 0;

void loop()
{
    parse_serial();

    unsigned long now = millis();
    if (tel_updated && now - lastUpdate >= 16)
    {
        lastUpdate = now;
        tel_updated = false;
        if (bsp_display_lock(10))
        {
            update_dashboard();
            bsp_display_unlock();
        }
    }
    delay(1);
}
