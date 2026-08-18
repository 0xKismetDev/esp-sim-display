#include "sim_screen.h"

#include <Arduino.h>

#include "config.h"
#include "telemetry.h"
#include "theme.h"

static lv_obj_t *scr;
static lv_obj_t *rpm_bar;
static lv_obj_t *gear_label;
static lv_obj_t *speed_label;
static lv_obj_t *rpm_label;
static lv_obj_t *shift_lights[9];
static lv_obj_t *classic_abs;
static lv_obj_t *classic_tc;

static lv_obj_t *gt_scr;
static lv_obj_t *gt_gear;
static lv_obj_t *gt_speed;
static lv_obj_t *gt_rpm;
static lv_obj_t *gt_abs;
static lv_obj_t *gt_tc;
static lv_obj_t *gt_rpm_segments[16];
static lv_obj_t *gt_tyre_temp[4];
static lv_obj_t *gt_tyre_pressure[4];
static lv_obj_t *gt_tyre_body[4];
static lv_obj_t *gt_tyre_groove[4][2];
static lv_obj_t *gt_lap_time;

static lv_obj_t *tyre_scr;
static lv_obj_t *tyre_temp[4];
static lv_obj_t *tyre_pressure[4];
static lv_obj_t *tyre_bar[4];
static lv_obj_t *tyre_body[4];
static lv_obj_t *tyre_groove[4][3];
static lv_obj_t *tyre_abs;
static lv_obj_t *tyre_tc;
static lv_obj_t *tyre_rpm_segments[16];
static lv_obj_t *tyre_lap_time;

static lv_obj_t *timing_scr;
static lv_obj_t *timing_track;
static lv_obj_t *timing_current;
static lv_obj_t *timing_delta;
static lv_obj_t *timing_last;
static lv_obj_t *timing_best;
static lv_obj_t *timing_lap;
static lv_obj_t *timing_position;
static lv_obj_t *timing_abs;
static lv_obj_t *timing_tc;
static lv_obj_t *timing_rpm_segments[16];

static lv_obj_t *plain_screen()
{
    lv_obj_t *s = lv_obj_create(NULL);
    lv_obj_remove_style_all(s);
    lv_obj_set_size(s, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x080A0D), 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    return s;
}

static lv_obj_t *label_at(lv_obj_t *parent, const char *text, const lv_font_t *font,
                          lv_color_t color, int x, int y, int w, lv_text_align_t align)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_width(l, w);
    lv_obj_set_style_text_align(l, align, 0);
    return l;
}

static lv_obj_t *card(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(0x10141A), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(0x28303A), 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_radius(o, 5, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_color_t tyre_color(float t)
{
    if (t <= 0) return C_DIM;
    if (t < 60) return lv_color_hex(0x35A7FF);
    if (t <= 100) return lv_color_hex(0x34D17B);
    if (t <= 115) return C_WARN;
    return C_CRIT;
}

static void set_level(lv_obj_t *label, const char *name, int level, bool active)
{
    char b[16];
    if (level < 0) snprintf(b, sizeof(b), "%s --", name);
    else snprintf(b, sizeof(b), "%s %d", name, level);
    lv_label_set_text(label, b);
    lv_obj_set_style_text_color(label, active ? C_WHITE : (level < 0 ? C_DIM : C_MID), 0);
    bool flash = ((millis() / 75) & 1) == 0;
    lv_obj_set_style_bg_color(label, active ? (flash ? C_CRIT : lv_color_hex(0x641919))
                                               : lv_color_hex(0x151A20), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(label, 3, 0);
    lv_obj_set_style_pad_hor(label, 7, 0);
    lv_obj_set_style_pad_ver(label, 3, 0);
}

lv_obj_t *sim_gt_screen_create()
{
    gt_scr = plain_screen();
    for (int i = 0; i < 16; i++) {
        gt_rpm_segments[i] = lv_obj_create(gt_scr);
        lv_obj_remove_style_all(gt_rpm_segments[i]);
        lv_obj_set_pos(gt_rpm_segments[i], 18 + i * 28, 300);
        lv_obj_set_size(gt_rpm_segments[i], 24, 13);
        lv_obj_set_style_radius(gt_rpm_segments[i], 2, 0);
        lv_obj_set_style_bg_opa(gt_rpm_segments[i], LV_OPA_COVER, 0);
    }
    gt_gear = label_at(gt_scr, "N", &lv_font_montserrat_48, C_WHITE, 16, 45, 160, LV_TEXT_ALIGN_CENTER);
    gt_speed = label_at(gt_scr, "0", &lv_font_montserrat_36, C_WHITE, 16, 116, 160, LV_TEXT_ALIGN_CENTER);
    label_at(gt_scr, "GEAR", &lv_font_montserrat_10, C_DIM, 16, 36, 160, LV_TEXT_ALIGN_CENTER);
    label_at(gt_scr, "KM/H", &lv_font_montserrat_10, C_DIM, 16, 158, 160, LV_TEXT_ALIGN_CENTER);
    gt_rpm = label_at(gt_scr, "0 RPM", &lv_font_montserrat_16, C_MID, 16, 185, 160, LV_TEXT_ALIGN_CENTER);
    gt_abs = label_at(gt_scr, "ABS --", &lv_font_montserrat_20, C_MID, 104, 258, 126, LV_TEXT_ALIGN_CENTER);
    gt_tc = label_at(gt_scr, "TC --", &lv_font_montserrat_20, C_MID, 250, 258, 126, LV_TEXT_ALIGN_CENTER);

    const char *pos[4] = {"FL", "FR", "RL", "RR"};
    for (int i = 0; i < 4; i++) {
        bool right = (i % 2) != 0;
        int y = 38 + (i / 2) * 99;
        int body_x = right ? 426 : 202;
        gt_tyre_body[i] = lv_obj_create(gt_scr);
        lv_obj_remove_style_all(gt_tyre_body[i]);
        lv_obj_set_pos(gt_tyre_body[i], body_x, y);
        lv_obj_set_size(gt_tyre_body[i], 30, 54);
        lv_obj_set_style_bg_opa(gt_tyre_body[i], LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(gt_tyre_body[i], C_DIM, 0);
        lv_obj_set_style_border_width(gt_tyre_body[i], 1, 0);
        lv_obj_set_style_border_color(gt_tyre_body[i], lv_color_hex(0x87919D), 0);
        lv_obj_set_style_radius(gt_tyre_body[i], 9, 0);
        lv_obj_clear_flag(gt_tyre_body[i], LV_OBJ_FLAG_SCROLLABLE);
        for (int g = 0; g < 2; g++) {
            gt_tyre_groove[i][g] = lv_obj_create(gt_tyre_body[i]);
            lv_obj_remove_style_all(gt_tyre_groove[i][g]);
            lv_obj_set_pos(gt_tyre_groove[i][g], 9 + g * 10, 6);
            lv_obj_set_size(gt_tyre_groove[i][g], 2, 42);
            lv_obj_set_style_bg_color(gt_tyre_groove[i][g], lv_color_hex(0x10141A), 0);
            lv_obj_set_style_bg_opa(gt_tyre_groove[i][g], LV_OPA_70, 0);
            lv_obj_set_style_radius(gt_tyre_groove[i][g], 1, 0);
        }
        int text_x = right ? 342 : 240;
        lv_text_align_t align = right ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT;
        label_at(gt_scr, pos[i], &lv_font_montserrat_10, C_DIM, text_x, y, 76, align);
        gt_tyre_temp[i] = label_at(gt_scr, "-- C", &lv_font_montserrat_16, C_TEXT, text_x, y + 17, 76, align);
        gt_tyre_pressure[i] = label_at(gt_scr, "-- psi", &lv_font_montserrat_12, C_MID, text_x, y + 38, 76, align);
    }
    gt_lap_time = label_at(gt_scr, "--:--.---", &lv_font_montserrat_16,
                           C_WHITE, 286, 106, 100, LV_TEXT_ALIGN_CENTER);
    return gt_scr;
}

lv_obj_t *sim_tyres_screen_create()
{
    tyre_scr = plain_screen();
    const char *pos[4] = {"FL", "FR", "RL", "RR"};
    const int outer_margin = 18;
    const int text_w = 68;
    const int tyre_w = 48;
    const int tyre_inset = 96;
    for (int i = 0; i < 4; i++) {
        bool right = (i % 2) != 0;
        int y = 27 + (i / 2) * 112;
        int body_x = right ? SCREEN_W - tyre_inset - tyre_w : tyre_inset;
        tyre_body[i] = lv_obj_create(tyre_scr);
        lv_obj_remove_style_all(tyre_body[i]);
        lv_obj_set_pos(tyre_body[i], body_x, y);
        lv_obj_set_size(tyre_body[i], 48, 82);
        lv_obj_set_style_bg_opa(tyre_body[i], LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(tyre_body[i], C_DIM, 0);
        lv_obj_set_style_border_width(tyre_body[i], 2, 0);
        lv_obj_set_style_border_color(tyre_body[i], lv_color_hex(0x87919D), 0);
        lv_obj_set_style_radius(tyre_body[i], 13, 0);
        lv_obj_clear_flag(tyre_body[i], LV_OBJ_FLAG_SCROLLABLE);
        for (int g = 0; g < 3; g++) {
            tyre_groove[i][g] = lv_obj_create(tyre_body[i]);
            lv_obj_remove_style_all(tyre_groove[i][g]);
            lv_obj_set_pos(tyre_groove[i][g], 11 + g * 11, 8);
            lv_obj_set_size(tyre_groove[i][g], 3, 66);
            lv_obj_set_style_bg_color(tyre_groove[i][g], lv_color_hex(0x10141A), 0);
            lv_obj_set_style_bg_opa(tyre_groove[i][g], LV_OPA_70, 0);
            lv_obj_set_style_radius(tyre_groove[i][g], 2, 0);
        }
        int text_x = right ? SCREEN_W - outer_margin - text_w : outer_margin;
        lv_text_align_t align = right ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT;
        label_at(tyre_scr, pos[i], &lv_font_montserrat_12, C_DIM, text_x, y + 2, text_w, align);
        tyre_temp[i] = label_at(tyre_scr, "-- C", &lv_font_montserrat_20, C_TEXT, text_x, y + 25, text_w, align);
        tyre_pressure[i] = label_at(tyre_scr, "-- psi", &lv_font_montserrat_14, C_MID, text_x, y + 56, text_w, align);
    }
    tyre_lap_time = label_at(tyre_scr, "--:--.---", &lv_font_montserrat_20,
                             C_WHITE, 150, 111, 180, LV_TEXT_ALIGN_CENTER);
    tyre_abs = label_at(tyre_scr, "ABS --", &lv_font_montserrat_16, C_MID, 104, 262, 126, LV_TEXT_ALIGN_CENTER);
    tyre_tc = label_at(tyre_scr, "TC --", &lv_font_montserrat_16, C_MID, 250, 262, 126, LV_TEXT_ALIGN_CENTER);
    for (int i = 0; i < 16; i++) {
        tyre_rpm_segments[i] = lv_obj_create(tyre_scr);
        lv_obj_remove_style_all(tyre_rpm_segments[i]);
        lv_obj_set_pos(tyre_rpm_segments[i], 18 + i * 28, 300);
        lv_obj_set_size(tyre_rpm_segments[i], 24, 13);
        lv_obj_set_style_radius(tyre_rpm_segments[i], 2, 0);
        lv_obj_set_style_bg_opa(tyre_rpm_segments[i], LV_OPA_COVER, 0);
    }
    return tyre_scr;
}

lv_obj_t *sim_timing_screen_create()
{
    timing_scr = plain_screen();
    timing_track = label_at(timing_scr, "NO TRACK", &lv_font_montserrat_12,
                            C_MID, 18, 8, 444, LV_TEXT_ALIGN_CENTER);
    label_at(timing_scr, "CURRENT LAP", &lv_font_montserrat_10, C_DIM, 18, 39, 444, LV_TEXT_ALIGN_CENTER);
    timing_current = label_at(timing_scr, "--:--.---", &lv_font_montserrat_48,
                              C_WHITE, 18, 54, 444, LV_TEXT_ALIGN_CENTER);
    timing_delta = label_at(timing_scr, "DELTA --.---", &lv_font_montserrat_20,
                            C_MID, 140, 112, 200, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *left = card(timing_scr, 18, 151, 137, 65);
    lv_obj_t *mid = card(timing_scr, 172, 151, 137, 65);
    lv_obj_t *right = card(timing_scr, 326, 151, 136, 65);
    label_at(left, "LAST", &lv_font_montserrat_10, C_DIM, 8, 7, 121, LV_TEXT_ALIGN_CENTER);
    timing_last = label_at(left, "--:--.---", &lv_font_montserrat_16, C_TEXT, 5, 31, 127, LV_TEXT_ALIGN_CENTER);
    label_at(mid, "BEST", &lv_font_montserrat_10, C_DIM, 8, 7, 121, LV_TEXT_ALIGN_CENTER);
    timing_best = label_at(mid, "--:--.---", &lv_font_montserrat_16, C_TEXT, 5, 31, 127, LV_TEXT_ALIGN_CENTER);
    label_at(right, "SESSION", &lv_font_montserrat_10, C_DIM, 8, 7, 120, LV_TEXT_ALIGN_CENTER);
    timing_lap = label_at(right, "LAP --/--", &lv_font_montserrat_14, C_TEXT, 5, 29, 126, LV_TEXT_ALIGN_CENTER);
    timing_position = label_at(timing_scr, "P --", &lv_font_montserrat_14,
                               C_MID, 18, 233, 70, LV_TEXT_ALIGN_LEFT);
    timing_abs = label_at(timing_scr, "ABS --", &lv_font_montserrat_16, C_MID, 104, 232, 126, LV_TEXT_ALIGN_CENTER);
    timing_tc = label_at(timing_scr, "TC --", &lv_font_montserrat_16, C_MID, 250, 232, 126, LV_TEXT_ALIGN_CENTER);
    for (int i = 0; i < 16; i++) {
        timing_rpm_segments[i] = lv_obj_create(timing_scr);
        lv_obj_remove_style_all(timing_rpm_segments[i]);
        lv_obj_set_pos(timing_rpm_segments[i], 18 + i * 28, 300);
        lv_obj_set_size(timing_rpm_segments[i], 24, 13);
        lv_obj_set_style_radius(timing_rpm_segments[i], 2, 0);
        lv_obj_set_style_bg_opa(timing_rpm_segments[i], LV_OPA_COVER, 0);
    }
    return timing_scr;
}

lv_obj_t *sim_screen_create()
{
    scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    rpm_bar = lv_bar_create(scr);
    lv_obj_set_size(rpm_bar, 444, 12);
    lv_obj_set_pos(rpm_bar, 18, 302);
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
        lv_obj_set_size(shift_lights[i], 22, 18);
        lv_obj_set_pos(shift_lights[i], start_x + i * 44, 274);
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
    lv_obj_set_pos(gear_label, 170, 30);
    lv_obj_set_width(gear_label, 140);
    lv_obj_set_style_text_align(gear_label, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *line1 = lv_obj_create(scr);
    lv_obj_remove_style_all(line1);
    lv_obj_set_size(line1, 400, 1);
    lv_obj_set_style_bg_color(line1, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(line1, LV_OPA_COVER, 0);
    lv_obj_align(line1, LV_ALIGN_TOP_MID, 0, 98);

    lv_obj_t *line2 = lv_obj_create(scr);
    lv_obj_remove_style_all(line2);
    lv_obj_set_size(line2, 400, 1);
    lv_obj_set_style_bg_color(line2, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(line2, LV_OPA_COVER, 0);
    lv_obj_align(line2, LV_ALIGN_TOP_MID, 0, 204);

    speed_label = lv_label_create(scr);
    lv_label_set_text(speed_label, "0");
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(speed_label, C_WHITE, 0);
    lv_obj_set_pos(speed_label, 38, 120);
    lv_obj_set_width(speed_label, 170);
    lv_obj_set_style_text_align(speed_label, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *unit_kmh = lv_label_create(scr);
    lv_label_set_text(unit_kmh, "km/h");
    lv_obj_set_style_text_font(unit_kmh, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(unit_kmh, C_GREY, 0);
    lv_obj_set_pos(unit_kmh, 38, 166);
    lv_obj_set_width(unit_kmh, 170);
    lv_obj_set_style_text_align(unit_kmh, LV_TEXT_ALIGN_CENTER, 0);

    rpm_label = lv_label_create(scr);
    lv_label_set_text(rpm_label, "0");
    lv_obj_set_style_text_font(rpm_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(rpm_label, C_WHITE, 0);
    lv_obj_set_pos(rpm_label, 272, 120);
    lv_obj_set_width(rpm_label, 170);
    lv_obj_set_style_text_align(rpm_label, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *unit_rpm = lv_label_create(scr);
    lv_label_set_text(unit_rpm, "rpm");
    lv_obj_set_style_text_font(unit_rpm, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(unit_rpm, C_GREY, 0);
    lv_obj_set_pos(unit_rpm, 272, 166);
    lv_obj_set_width(unit_rpm, 170);
    lv_obj_set_style_text_align(unit_rpm, LV_TEXT_ALIGN_CENTER, 0);

    classic_abs = label_at(scr, "ABS --", &lv_font_montserrat_14, C_MID, 108, 230, 110, LV_TEXT_ALIGN_CENTER);
    classic_tc = label_at(scr, "TC --", &lv_font_montserrat_14, C_MID, 262, 230, 110, LV_TEXT_ALIGN_CENTER);

    return scr;
}

static unsigned long last_blink_ms = 0;
static bool blink_state = false;

void sim_screen_update()
{
    int pct = g_sim.rpmPct;
    int red = g_sim.redlinePct;

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

    lv_label_set_text(gear_label, g_sim.gear);
    lv_color_t gear_col = C_WHITE;
    if (at_redline)
        gear_col = C_RED;
    else if (g_sim.gear[0] == 'N')
        gear_col = C_GREY;
    else if (g_sim.gear[0] == 'R')
        gear_col = C_ORANGE;
    lv_obj_set_style_text_color(gear_label, gear_col, 0);

    char spd[8];
    snprintf(spd, sizeof(spd), "%d", g_sim.speed);
    lv_label_set_text(speed_label, spd);

    char rpm_str[8];
    snprintf(rpm_str, sizeof(rpm_str), "%d", g_sim.rpm);
    lv_label_set_text(rpm_label, rpm_str);
    set_level(classic_abs, "ABS", g_sim.absLevel, g_sim.absActive);
    set_level(classic_tc, "TC", g_sim.tcLevel, g_sim.tcActive);
}

static void format_tyre(char *temp, size_t temp_n, char *pressure, size_t pressure_n, int i)
{
    if (g_sim.tyreTemp[i] > 0) snprintf(temp, temp_n, "%.0f C", g_sim.tyreTemp[i]);
    else snprintf(temp, temp_n, "-- C");
    if (g_sim.tyrePressure[i] > 0)
        snprintf(pressure, pressure_n, "%.1f %s", g_sim.tyrePressure[i], g_sim.pressureUnit);
    else snprintf(pressure, pressure_n, "-- %s", g_sim.pressureUnit);
}

static bool sim_redline_flash(lv_obj_t *screen)
{
    bool redline = g_sim.rpmPct >= g_sim.redlinePct;
    bool flash = redline && (((millis() / 60) & 1) != 0);
    lv_obj_set_style_bg_color(screen, flash ? C_RED : lv_color_hex(0x080A0D), 0);
    return flash;
}

void sim_gt_screen_update()
{
    bool red_flash = sim_redline_flash(gt_scr);
    int lit = g_sim.rpmPct * 16 / 100;
    for (int i = 0; i < 16; i++) {
        lv_color_t on = i < 9 ? lv_color_hex(0x28D17C) : (i < 13 ? C_WARN : C_CRIT);
        lv_obj_set_style_bg_color(gt_rpm_segments[i], red_flash ? C_WHITE :
                                  (i < lit ? on : lv_color_hex(0x182027)), 0);
    }
    lv_label_set_text(gt_gear, g_sim.gear);
    char b[24];
    snprintf(b, sizeof(b), "%d", g_sim.speed); lv_label_set_text(gt_speed, b);
    snprintf(b, sizeof(b), "%d RPM", g_sim.rpm); lv_label_set_text(gt_rpm, b);
    lv_label_set_text(gt_lap_time, g_sim.currentLapTime);
    set_level(gt_abs, "ABS", g_sim.absLevel, g_sim.absActive);
    set_level(gt_tc, "TC", g_sim.tcLevel, g_sim.tcActive);
    for (int i = 0; i < 4; i++) {
        char t[16], p[20]; format_tyre(t, sizeof(t), p, sizeof(p), i);
        lv_label_set_text(gt_tyre_temp[i], t);
        lv_obj_set_style_text_color(gt_tyre_temp[i], tyre_color(g_sim.tyreTemp[i]), 0);
        lv_label_set_text(gt_tyre_pressure[i], p);
        lv_color_t heat = tyre_color(g_sim.tyreTemp[i]);
        lv_obj_set_style_bg_color(gt_tyre_body[i], heat, 0);
        lv_obj_set_style_border_color(gt_tyre_body[i], g_sim.tyreTemp[i] > 0 ? lv_color_lighten(heat, 70) : C_DIM, 0);
    }
}

void sim_tyres_screen_update()
{
    bool red_flash = sim_redline_flash(tyre_scr);
    int lit = g_sim.rpmPct * 16 / 100;
    for (int i = 0; i < 4; i++) {
        char t[16], p[20]; format_tyre(t, sizeof(t), p, sizeof(p), i);
        lv_label_set_text(tyre_temp[i], t);
        lv_obj_set_style_text_color(tyre_temp[i], tyre_color(g_sim.tyreTemp[i]), 0);
        lv_label_set_text(tyre_pressure[i], p);
        lv_color_t heat = tyre_color(g_sim.tyreTemp[i]);
        lv_obj_set_style_bg_color(tyre_body[i], heat, 0);
        lv_obj_set_style_border_color(tyre_body[i], g_sim.tyreTemp[i] > 0 ? lv_color_lighten(heat, 70) : C_DIM, 0);
    }
    lv_label_set_text(tyre_lap_time, g_sim.currentLapTime);
    for (int i = 0; i < 16; i++) {
        lv_color_t on = i < 9 ? lv_color_hex(0x28D17C) : (i < 13 ? C_WARN : C_CRIT);
        lv_obj_set_style_bg_color(tyre_rpm_segments[i], red_flash ? C_WHITE :
                                  (i < lit ? on : lv_color_hex(0x182027)), 0);
    }
    set_level(tyre_abs, "ABS", g_sim.absLevel, g_sim.absActive);
    set_level(tyre_tc, "TC", g_sim.tcLevel, g_sim.tcActive);
}

void sim_timing_screen_update()
{
    bool red_flash = sim_redline_flash(timing_scr);
    lv_label_set_text(timing_track, g_sim.trackName);
    lv_label_set_text(timing_current, g_sim.currentLapTime);
    lv_label_set_text(timing_last, g_sim.lastLapTime);
    lv_label_set_text(timing_best, g_sim.bestLapTime);

    char b[32];
    if (g_sim.deltaValid) {
        int a = abs(g_sim.deltaMs);
        snprintf(b, sizeof(b), "%c%d.%03d", g_sim.deltaMs <= 0 ? '-' : '+', a / 1000, a % 1000);
        lv_obj_set_style_text_color(timing_delta, g_sim.deltaMs <= 0 ? lv_color_hex(0x34D17B) : C_CRIT, 0);
    } else {
        snprintf(b, sizeof(b), "DELTA --.---");
        lv_obj_set_style_text_color(timing_delta, C_MID, 0);
    }
    lv_label_set_text(timing_delta, b);
    if (g_sim.totalLaps > 0) snprintf(b, sizeof(b), "LAP %d/%d", g_sim.currentLap, g_sim.totalLaps);
    else snprintf(b, sizeof(b), "LAP %d", g_sim.currentLap);
    lv_label_set_text(timing_lap, b);
    if (g_sim.position > 0) snprintf(b, sizeof(b), "P %d", g_sim.position);
    else snprintf(b, sizeof(b), "P --");
    lv_label_set_text(timing_position, b);
    set_level(timing_abs, "ABS", g_sim.absLevel, g_sim.absActive);
    set_level(timing_tc, "TC", g_sim.tcLevel, g_sim.tcActive);

    int lit = g_sim.rpmPct * 16 / 100;
    for (int i = 0; i < 16; i++) {
        lv_color_t on = i < 9 ? lv_color_hex(0x28D17C) : (i < 13 ? C_WARN : C_CRIT);
        lv_obj_set_style_bg_color(timing_rpm_segments[i], red_flash ? C_WHITE :
                                  (i < lit ? on : lv_color_hex(0x182027)), 0);
    }
}

// Extended packets are framed as @;...;# so single digit aid values cannot be
// mistaken for a gear. The legacy five-field stream remains accepted.
static char field_buf[48];
static int field_pos = 0;
static int field_idx = 0;
static bool framed = false;

static bool is_gear_value(const char *s)
{
    if (s[0] == '\0' || s[1] != '\0')
        return false;
    char c = s[0];
    return (c == 'N' || c == 'R' || (c >= '1' && c <= '8'));
}

static void process_field(const char *val)
{
    if (!framed && is_gear_value(val))
        field_idx = 1;

    switch (field_idx)
    {
    case 0:
    {
        int v = atoi(val);
        if (v >= 0 && v <= 999)
            g_sim.speed = v;
        break;
    }
    case 1:
        strncpy(g_sim.gear, val, sizeof(g_sim.gear) - 1);
        g_sim.gear[sizeof(g_sim.gear) - 1] = '\0';
        break;
    case 2:
    {
        int v = atoi(val);
        if (v >= 0 && v <= 30000)
            g_sim.rpm = v;
        break;
    }
    case 3:
    {
        int v = atoi(val);
        if (v >= 100 && v <= 30000)
            g_sim.maxRpm = v;
        break;
    }
    case 4:
    {
        int v = atoi(val);
        if (v >= 1 && v <= 100)
            g_sim.redlinePct = v;
        g_sim.rpmPct = (g_sim.maxRpm > 0) ? (g_sim.rpm * 100) / g_sim.maxRpm : 0;
        if (g_sim.rpmPct > 100)
            g_sim.rpmPct = 100;
        break;
    }
    case 5: g_sim.absLevel = atoi(val); break;
    case 6: g_sim.tcLevel = atoi(val); break;
    case 7: g_sim.absActive = atoi(val) != 0; break;
    case 8: g_sim.tcActive = atoi(val) != 0; break;
    case 9: case 10: case 11: case 12:
        g_sim.tyreTemp[field_idx - 9] = atof(val); break;
    case 13: case 14: case 15: case 16:
        g_sim.tyrePressure[field_idx - 13] = atof(val); break;
    case 17:
        strncpy(g_sim.pressureUnit, atoi(val) == 1 ? "bar" : (atoi(val) == 2 ? "kPa" : "psi"),
                sizeof(g_sim.pressureUnit) - 1);
        g_sim.pressureUnit[sizeof(g_sim.pressureUnit) - 1] = '\0';
        break;
    case 18:
        strncpy(g_sim.currentLapTime, val, sizeof(g_sim.currentLapTime) - 1);
        g_sim.currentLapTime[sizeof(g_sim.currentLapTime) - 1] = '\0'; break;
    case 19:
        strncpy(g_sim.lastLapTime, val, sizeof(g_sim.lastLapTime) - 1);
        g_sim.lastLapTime[sizeof(g_sim.lastLapTime) - 1] = '\0'; break;
    case 20:
        strncpy(g_sim.bestLapTime, val, sizeof(g_sim.bestLapTime) - 1);
        g_sim.bestLapTime[sizeof(g_sim.bestLapTime) - 1] = '\0'; break;
    case 21: g_sim.deltaMs = atoi(val); g_sim.deltaValid = strcmp(val, "x") != 0; break;
    case 22: g_sim.currentLap = atoi(val); break;
    case 23: g_sim.totalLaps = atoi(val); break;
    case 24: g_sim.position = atoi(val); break;
    case 25:
        strncpy(g_sim.trackName, val, sizeof(g_sim.trackName) - 1);
        g_sim.trackName[sizeof(g_sim.trackName) - 1] = '\0'; break;
    }
    field_idx = (field_idx + 1) % SIM_NUM_FIELDS;
}

void sim_feed(char c)
{
    if (c == '@') {
        framed = true; field_idx = 0; field_pos = 0; return;
    }
    if (c == '#') {
        field_buf[field_pos] = '\0';
        if (field_pos > 0) process_field(field_buf);
        field_pos = 0;
        if (framed && field_idx == 0) {
            g_sim.updated = true;
            g_sim.lastUpdateMs = millis();
        }
        framed = false;
        return;
    }
    if (c == ';' || c == '\n' || c == '\r')
    {
        field_buf[field_pos] = '\0';
        if (field_pos > 0)
            process_field(field_buf);
        field_pos = 0;
        if (!framed && field_idx == 0) {
            g_sim.updated = true;
            g_sim.lastUpdateMs = millis();
        }
    }
    else if (field_pos < (int)sizeof(field_buf) - 1)
    {
        field_buf[field_pos++] = c;
    }
}

