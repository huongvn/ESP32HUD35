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
static lv_obj_t *spd_label;
static lv_obj_t *rpm_label;

static lv_obj_t *coolant_arc, *coolant_val, *coolant_unit;
static lv_obj_t *batt_arc, *batt_val, *batt_unit;
static lv_obj_t *aux_label;

static lv_obj_t *label_center(int y, const lv_font_t *font, lv_color_t color, int off_x, const char *text)
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

/* Big center gauge: title above, big value in middle */
static void big_gauge(int cx, const char *title, lv_obj_t **arc_out,
                      lv_obj_t **val_out, lv_obj_t **unit_out)
{
    int off = cx - SCREEN_W / 2;
    label_center(96, &lv_font_montserrat_14, COL_DIM, off, title);
    redzone_create(cx, 230, 140, 12, 337, 405, 80);
    lv_obj_t *arc = arc_create(cx, 230, 140, 12, 135, 405, 80, COL_ARCBG, COL_GREEN);
    lv_obj_t *val = label_center(212, &lv_font_montserrat_48, COL_WHITE, off, "--");
    lv_obj_set_width(val, 160);
    lv_obj_t *unit = label_center(268, &lv_font_montserrat_14, COL_DIM, off, "");
    *arc_out = arc;
    *val_out = val;
    *unit_out = unit;
}

void ui_init(void)
{
    scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COL_BG, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // Top bar: speed (right)
    spd_label = lv_label_create(scr);
    lv_obj_set_style_text_font(spd_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(spd_label, COL_WHITE, 0);
    lv_obj_set_style_text_align(spd_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(spd_label, LV_ALIGN_TOP_RIGHT, -8, 12);
    lv_label_set_text(spd_label, "0 km/h");

    // Two big gauges: COOL (left), BATT (right)
    big_gauge(85, "COOLANT", &coolant_arc, &coolant_val, &coolant_unit);
    big_gauge(235, "BATTERY", &batt_arc, &batt_val, &batt_unit);

    // Bottom center: RPM + intake/throttle
    rpm_label = label_center(404, &lv_font_montserrat_28, COL_WHITE, 0, "RPM 0");
    aux_label = label_center(444, &lv_font_montserrat_14, COL_DIM, 0, "IAT --C  THR --%");
}

void ui_update(int speed_kmh, int coolant_c, int rpm, int gear, uint32_t odo_km,
               int load_pct, int intake_c, int throttle_pct, int batt_mv)
{
    static const char gear_chars[] = {'P', 'R', 'N', 'D', 'S'};
    (void)gear_chars;

    // Odometer
    (void)odo_km;

    // Speed (small)
    lv_label_set_text_fmt(spd_label, "%d km/h", speed_kmh);

    // RPM (small)
    lv_label_set_text_fmt(rpm_label, "RPM %u", rpm);

    // Intake + Throttle
    lv_label_set_text_fmt(aux_label, "IAT %dC  THR %d%%", intake_c, throttle_pct);

    // Coolant (big gauge, 40-120C -> 0-80)
    lv_label_set_text_fmt(coolant_val, "%d", coolant_c);
    lv_label_set_text(coolant_unit, "C");
    int cool_v = coolant_c - 40;
    if (cool_v < 0) cool_v = 0;
    if (cool_v > 80) cool_v = 80;
    lv_arc_set_value(coolant_arc, cool_v);
    lv_color_t ccol = COL_GREEN;
    if (coolant_c < 60) ccol = COL_BLUE;
    else if (coolant_c >= 110) ccol = COL_RED;
    else if (coolant_c >= 100) ccol = COL_ORANGE;
    lv_obj_set_style_arc_color(coolant_arc, ccol, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(coolant_val, ccol, 0);

    // Battery (big gauge, 9-16V -> 0-80)
    if (batt_mv <= 0)
    {
        lv_label_set_text(batt_val, "0.0");
        lv_label_set_text(batt_unit, "V");
        lv_arc_set_value(batt_arc, 0);
        lv_obj_set_style_arc_color(batt_arc, COL_ORANGE, LV_PART_INDICATOR);
        lv_obj_set_style_text_color(batt_val, COL_ORANGE, 0);
        goto batt_done;
    }
    if (batt_mv < 9000) batt_mv = 9000;
    if (batt_mv > 16000) batt_mv = 16000;
    int batt_v = (batt_mv - 9000) * 80 / 7000;
    lv_label_set_text_fmt(batt_val, "%d.%d", batt_mv / 1000, (batt_mv % 1000) / 100);
    lv_label_set_text(batt_unit, "V");
    lv_arc_set_value(batt_arc, batt_v);
    lv_color_t bcol = COL_ORANGE;
    if (batt_mv >= 13000) bcol = COL_GREEN;
    else if (batt_mv < 11500) bcol = COL_RED;
    lv_obj_set_style_arc_color(batt_arc, bcol, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(batt_val, bcol, 0);
batt_done:
    (void)0;
}