#include <lvgl.h>
#include "ui.h"

#define SCREEN_W DISPLAY_WIDTH
#define SCREEN_H DISPLAY_HEIGHT

#define COL_BG      lv_color_hex(0x0B0F14)
#define COL_PANEL   lv_color_hex(0x141B26)
#define COL_ARCBG   lv_color_hex(0x26303E)
#define COL_CYAN    lv_color_hex(0x00E5FF)
#define COL_GREEN   lv_color_hex(0x00E676)
#define COL_ORANGE  lv_color_hex(0xFF9800)
#define COL_RED     lv_color_hex(0xFF3B30)
#define COL_REDLINE lv_color_hex(0x3A1717)
#define COL_WHITE   lv_color_hex(0xF5F7FA)
#define COL_DIM     lv_color_hex(0x8A94A6)
#define COL_BLUE    lv_color_hex(0x2196F3)

static lv_obj_t *scr;
static lv_obj_t *turn_left;
static lv_obj_t *turn_right;
static lv_obj_t *odo_label;
static lv_obj_t *gear_label;
static lv_obj_t *speed_label;
static lv_obj_t *kmh_label;
static lv_obj_t *speed_arc;
static lv_obj_t *coolant_arc;
static lv_obj_t *coolant_val;
static lv_obj_t *coolant_unit;
static lv_obj_t *rpm_arc;
static lv_obj_t *rpm_val;
static lv_obj_t *rpm_unit;

static lv_obj_t *label_full(int y, const lv_font_t *font, lv_color_t color, int off_x, const char *text)
{
    lv_obj_t *l = lv_label_create(scr);
    lv_obj_set_width(l, SCREEN_W);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, off_x, y);
    lv_label_set_text(l, text);
    return l;
}

static lv_obj_t *arc_create(int cx, int cy, int size, int thick,
                            int start_angle, int end_angle, int arc_max,
                            lv_color_t bg_col, lv_color_t val_col)
{
    lv_obj_t *arc = lv_arc_create(scr);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, cx - size / 2, cy - size / 2);
    lv_arc_set_rotation(arc, 0);
    lv_arc_set_bg_angles(arc, start_angle, end_angle);
    lv_arc_set_range(arc, 0, arc_max);
    lv_arc_set_value(arc, 0);
    lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);

    lv_obj_set_style_arc_width(arc, thick, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, bg_col, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

    lv_obj_set_style_arc_width(arc, thick, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, val_col, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    return arc;
}

static lv_obj_t *redzone_create(int cx, int cy, int size, int thick,
                                int start_angle, int end_angle, int arc_max)
{
    lv_obj_t *arc = arc_create(cx, cy, size, thick, start_angle, end_angle, arc_max,
                               lv_color_hex(0x000000), COL_REDLINE);
    lv_obj_set_style_arc_opa(arc, 0, LV_PART_MAIN);
    lv_arc_set_value(arc, arc_max);
    return arc;
}

void ui_init(void)
{
    scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COL_BG, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);

    // Top status bar
    turn_left = lv_label_create(scr);
    lv_obj_set_style_text_font(turn_left, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(turn_left, COL_DIM, 0);
    lv_obj_align(turn_left, LV_ALIGN_TOP_LEFT, 24, 14);
    lv_label_set_text(turn_left, LV_SYMBOL_LEFT);

    turn_right = lv_label_create(scr);
    lv_obj_set_style_text_font(turn_right, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(turn_right, COL_DIM, 0);
    lv_obj_align(turn_right, LV_ALIGN_TOP_RIGHT, -24, 14);
    lv_label_set_text(turn_right, LV_SYMBOL_RIGHT);

    odo_label = label_full(18, &lv_font_montserrat_20, COL_DIM, 0, "ODO 000000 km");

    // Gear indicator
    gear_label = label_full(64, &lv_font_montserrat_28, COL_CYAN, 0, "P");

    // Big speed arc (270 deg gauge)
    redzone_create(160, 250, 258, 14, 337, 405, 240);          // red zone >180 km/h
    speed_arc = arc_create(160, 250, 258, 14, 135, 405, 240, COL_ARCBG, COL_CYAN);

    speed_label = label_full(212, &lv_font_montserrat_48, COL_WHITE, 0, "0");
    kmh_label = label_full(268, &lv_font_montserrat_20, COL_DIM, 0, "km/h");

    // Coolant gauge (left)
    label_full(384, &lv_font_montserrat_14, COL_DIM, -75, "COOLANT");
    redzone_create(85, 438, 100, 10, 337, 405, 80);            // >100C
    coolant_arc = arc_create(85, 438, 100, 10, 135, 405, 80, COL_ARCBG, COL_GREEN);
    coolant_val = label_full(398, &lv_font_montserrat_28, COL_WHITE, -75, "00");
    coolant_unit = label_full(428, &lv_font_montserrat_14, COL_DIM, -75, "C");

    // RPM gauge (right)
    label_full(384, &lv_font_montserrat_14, COL_DIM, 75, "RPM");
    redzone_create(235, 438, 100, 10, 337, 405, 80);           // redline >6k
    rpm_arc = arc_create(235, 438, 100, 10, 135, 405, 80, COL_ARCBG, COL_GREEN);
    rpm_val = label_full(398, &lv_font_montserrat_28, COL_WHITE, 75, "0.0");
    rpm_unit = label_full(428, &lv_font_montserrat_14, COL_DIM, 75, "x1000");
}

void ui_update(int speed_kmh, int coolant_c, int rpm, int gear, uint32_t odo_km)
{
    static const char gear_chars[] = {'P', 'R', 'N', 'D', 'S'};

    // Speed
    lv_label_set_text_fmt(speed_label, "%d", speed_kmh);
    if (speed_kmh > 240) speed_kmh = 240;
    lv_arc_set_value(speed_arc, speed_kmh);

    // Gear
    if (gear < 0) gear = 0;
    if (gear > 4) gear = 3;
    lv_label_set_text_fmt(gear_label, "%c", gear_chars[gear]);

    // Odometer
    lv_label_set_text_fmt(odo_label, "ODO %06lu km", (unsigned long)odo_km);

    // Coolant
    if (coolant_c > 120) coolant_c = 120;
    lv_label_set_text_fmt(coolant_val, "%02d", coolant_c);
    lv_arc_set_value(coolant_arc, coolant_c - 40);
    lv_color_t ccol = COL_GREEN;
    if (coolant_c < 60) ccol = COL_BLUE;
    else if (coolant_c >= 110) ccol = COL_RED;
    else if (coolant_c >= 100) ccol = COL_ORANGE;
    lv_obj_set_style_arc_color(coolant_arc, ccol, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(coolant_val, ccol, 0);

    // RPM
    if (rpm < 0) rpm = 0;
    if (rpm > 8000) rpm = 8000;
    lv_label_set_text_fmt(rpm_val, "%d.%d", rpm / 1000, (rpm % 1000) / 100);
    lv_arc_set_value(rpm_arc, rpm / 100);
    lv_color_t rcol = COL_GREEN;
    if (rpm >= 6000) rcol = COL_RED;
    else if (rpm >= 5000) rcol = COL_ORANGE;
    lv_obj_set_style_arc_color(rpm_arc, rcol, LV_PART_INDICATOR);
}