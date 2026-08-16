#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"

#define SCREEN_W DISPLAY_WIDTH
#define SCREEN_H DISPLAY_HEIGHT

#define COL_BG      lv_color_hex(0x000000)
#define COL_ARC_BG  lv_color_hex(0x1E1E1E)
#define COL_BLUE    lv_color_hex(0x3936F5)
#define COL_WHITE   lv_color_hex(0xFFFFFF)
#define COL_GRAY    lv_color_hex(0x8A8A8A)

/* ============ zone separators ============ */
#define Z1_END 56    /* top bar: clock + speed + CAN led */
#define Z2_END 324   /* two gauges: coolant + battery icon */
#define Z3_END 416   /* secondary params: rpm/load/oil/thr */
#define Z4_END 480   /* footer: wifi info */

static lv_obj_t *scr_main;
static lv_obj_t *can_bars[4];
static lv_obj_t *clock_label;
static lv_obj_t *spd_label;
static lv_obj_t *coolant_arc, *coolant_val, *coolant_unit;
static lv_obj_t *batt_body, *batt_fill, *batt_nub, *batt_val, *batt_unit;
static lv_obj_t *rpm_val, *load_val, *iat_val, *thr_val;
static lv_obj_t *wifi_label;
static lv_obj_t *dtc_label;

#define COL_YELLOW lv_color_hex(0xD9C100)

static lv_obj_t *label_center(lv_obj_t *parent, int y, const lv_font_t *font,
                              lv_color_t color, int off_x, const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_width(l, SCREEN_W);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, off_x, y);
    lv_label_set_text(l, text);
    return l;
}

static void divider(int y)
{
    lv_obj_t *d = lv_obj_create(scr_main);
    lv_obj_set_size(d, SCREEN_W - 24, 2);
    lv_obj_align(d, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_remove_flag(d, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(d, COL_ARC_BG, 0);
    lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(d, 0, 0);
    lv_obj_set_style_pad_all(d, 0, 0);
}

static lv_obj_t *arc_create(int cx, int cy, int size, int thick,
                            int start_angle, int end_angle, int arc_max,
                            lv_color_t bg_col, lv_color_t val_col)
{
    lv_obj_t *arc = lv_arc_create(scr_main);
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
                               lv_color_hex(0x000000), COL_ARC_BG);
    lv_obj_set_style_arc_opa(arc, 0, LV_PART_MAIN);
    lv_arc_set_value(arc, arc_max);
    return arc;
}

static void arc_tick(int cx, int cy, int size, int thick, int arc_max, int v, lv_color_t col)
{
    double f = (double)v / arc_max;
    int a = 135 + (int)(270 * f);

    lv_obj_t *t = lv_arc_create(scr_main);
    lv_obj_set_size(t, size, size);
    lv_obj_set_pos(t, cx - size / 2, cy - size / 2);
    lv_arc_set_rotation(t, 0);
    lv_arc_set_bg_angles(t, a - 4, a + 4);
    lv_arc_set_range(t, 0, 100);
    lv_arc_set_value(t, 100);
    lv_obj_remove_style(t, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_opa(t, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_width(t, thick + 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(t, col, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(t, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(t, false, LV_PART_INDICATOR);
}

/* ============ ZONE 2: coolant gauge ============ */
static void coolant_gauge(void)
{
    label_center(scr_main, 100, &lv_font_montserrat_14, COL_GRAY, -75, "COOLANT");
    int cx = 85, cy = 192;
    redzone_create(cx, cy, 140, 12, 337, 405, 80);
    coolant_arc = arc_create(cx, cy, 140, 12, 135, 405, 80, COL_ARC_BG, COL_BLUE);
    arc_tick(cx, cy, 140, 12, 80, 20, COL_WHITE);
    arc_tick(cx, cy, 140, 12, 80, 60, COL_WHITE);
    coolant_val = label_center(scr_main, 172, &lv_font_montserrat_48, COL_WHITE, -75, "--");
    lv_obj_set_width(coolant_val, 160);
    coolant_unit = label_center(scr_main, 238, &lv_font_montserrat_14, COL_GRAY, -75, "C");
}

/* ============ ZONE 2: battery icon (pill shape) ============ */
static void battery_icon(void)
{
    int cx = 235, cy = 192;

    label_center(scr_main, 100, &lv_font_montserrat_14, COL_GRAY, 75, "BATTERY");

    batt_body = lv_obj_create(scr_main);
    lv_obj_set_size(batt_body, 96, 40);
    lv_obj_set_pos(batt_body, cx - 48, cy - 20);
    lv_obj_remove_flag(batt_body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(batt_body, 8, 0);
    lv_obj_set_style_bg_color(batt_body, COL_BG, 0);
    lv_obj_set_style_bg_opa(batt_body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(batt_body, COL_WHITE, 0);
    lv_obj_set_style_border_width(batt_body, 2, 0);
    lv_obj_set_style_pad_all(batt_body, 0, 0);

    batt_nub = lv_obj_create(scr_main);
    lv_obj_set_size(batt_nub, 8, 18);
    lv_obj_set_pos(batt_nub, cx + 48, cy - 9);
    lv_obj_remove_flag(batt_nub, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(batt_nub, 3, 0);
    lv_obj_set_style_bg_color(batt_nub, COL_WHITE, 0);
    lv_obj_set_style_bg_opa(batt_nub, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(batt_nub, 0, 0);
    lv_obj_set_style_pad_all(batt_nub, 0, 0);

    batt_fill = lv_obj_create(scr_main);
    lv_obj_set_size(batt_fill, 84, 28);
    lv_obj_set_pos(batt_fill, cx - 45, cy - 14);
    lv_obj_remove_flag(batt_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(batt_fill, 5, 0);
    lv_obj_set_style_bg_color(batt_fill, COL_BLUE, 0);
    lv_obj_set_style_bg_opa(batt_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(batt_fill, 0, 0);
    lv_obj_set_style_pad_all(batt_fill, 0, 0);

    batt_val = label_center(scr_main, 252, &lv_font_montserrat_20, COL_WHITE, 75, "0.0");
    batt_unit = label_center(scr_main, 278, &lv_font_montserrat_14, COL_GRAY, 75, "V");
}

/* ============ ZONE 3: secondary readouts ============ */
static void sub_readout(int cx, const char *title, lv_obj_t **val_out)
{
    int off = cx - SCREEN_W / 2;
    label_center(scr_main, 352, &lv_font_montserrat_14, COL_GRAY, off, title);
    lv_obj_t *val = label_center(scr_main, 376, &lv_font_montserrat_20, COL_WHITE, off, "--");
    lv_obj_set_width(val, 80);
    *val_out = val;
}

/* ============ settings page ============ */

void ui_init(void)
{
    scr_main = lv_screen_active();
    lv_obj_set_style_bg_color(scr_main, COL_BG, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr_main, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_remove_flag(scr_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(scr_main, 0, 0);

    /* ============ ZONE 1: top bar ============ */
    for (int i = 0; i < 4; i++)
    {
        can_bars[i] = lv_obj_create(scr_main);
        lv_obj_set_size(can_bars[i], 4, 6 + i * 4);
        lv_obj_align(can_bars[i], LV_ALIGN_TOP_LEFT, 8 + i * 6, 14 - i * 2);
        lv_obj_remove_flag(can_bars[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(can_bars[i], 1, 0);
        lv_obj_set_style_bg_color(can_bars[i], COL_GRAY, 0);
        lv_obj_set_style_bg_opa(can_bars[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(can_bars[i], 0, 0);
        lv_obj_set_style_pad_all(can_bars[i], 0, 0);
    }

    clock_label = label_center(scr_main, 10, &lv_font_montserrat_28, COL_WHITE, 0, "--:--");
    lv_obj_set_width(clock_label, 120);

    spd_label = lv_label_create(scr_main);
    lv_obj_set_style_text_font(spd_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(spd_label, COL_WHITE, 0);
    lv_obj_set_style_text_align(spd_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(spd_label, LV_ALIGN_TOP_RIGHT, -8, 12);
    lv_label_set_text(spd_label, "0 km/h");
    divider(Z1_END - 2);

    /* ============ ZONE 2: coolant gauge + battery icon ============ */
    coolant_gauge();
    battery_icon();
    divider(Z2_END - 2);

    /* ============ ZONE 3: secondary params ============ */
    sub_readout(40, "RPM", &rpm_val);
    sub_readout(120, "LOAD", &load_val);
    sub_readout(200, "IAT", &iat_val);
    sub_readout(280, "THR", &thr_val);
    divider(Z3_END - 2);

    /* ============ ZONE 4: wifi info (left) + DTC (right) ============ */
    wifi_label = lv_label_create(scr_main);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(wifi_label, COL_GRAY, 0);
    lv_obj_align(wifi_label, LV_ALIGN_BOTTOM_LEFT, 12, -10);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI " --");

    dtc_label = lv_label_create(scr_main);
    lv_obj_set_style_text_font(dtc_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(dtc_label, COL_YELLOW, 0);
    lv_obj_set_style_text_align(dtc_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(dtc_label, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
    lv_label_set_text(dtc_label, "");
    divider(Z4_END - 2);
}

void ui_update(int speed_kmh, int coolant_c, int rpm, int load_pct,
               int iat_c, int throttle_pct, int batt_mv)
{
    /* ZONE 1 */
    lv_label_set_text_fmt(spd_label, "%d km/h", speed_kmh);

    /* ZONE 2: coolant */
    lv_label_set_text_fmt(coolant_val, "%d", coolant_c);
    lv_label_set_text(coolant_unit, "C");
    int cool_v = coolant_c - 40;
    if (cool_v < 0) cool_v = 0;
    if (cool_v > 80) cool_v = 80;
    lv_arc_set_value(coolant_arc, cool_v);
    lv_obj_set_style_arc_color(coolant_arc, COL_BLUE, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(coolant_val, COL_WHITE, 0);

    /* ZONE 2: battery icon fill (9-16V -> 0..84 px) */
    bool batt_low = false;
    int fill_w = 0;
    if (batt_mv > 0)
    {
        if (batt_mv < 9000) batt_mv = 9000;
        if (batt_mv > 16000) batt_mv = 16000;
        fill_w = (batt_mv - 9000) * 84 / 7000;
        if (batt_mv < 11500) batt_low = true;
    }
    lv_obj_set_width(batt_fill, fill_w);
    lv_obj_set_style_bg_color(batt_fill, batt_low ? lv_color_hex(0xD9C100) : COL_BLUE, 0);
    if (batt_mv > 0)
        lv_label_set_text_fmt(batt_val, "%d.%d", batt_mv / 1000, (batt_mv % 1000) / 100);
    else
        lv_label_set_text(batt_val, "0.0");
    lv_label_set_text(batt_unit, "V");

    /* ZONE 3 */
    lv_label_set_text_fmt(rpm_val, "%u", rpm);
    lv_label_set_text_fmt(load_val, "%d%%", load_pct);
    lv_label_set_text_fmt(iat_val, "%dC", iat_c);
    lv_label_set_text_fmt(thr_val, "%d%%", throttle_pct);

    /* Background warnings (priority: overheat > low battery > normal) */
    bool blink = ((lv_tick_get() / 250) % 2) == 0;
    if (coolant_c > 100)
        lv_obj_set_style_bg_color(scr_main, blink ? lv_color_hex(0xE52B2B) : COL_BG, 0);
    else if (batt_low)
        lv_obj_set_style_bg_color(scr_main, blink ? lv_color_hex(0xD9C100) : COL_BG, 0);
    else
        lv_obj_set_style_bg_color(scr_main, COL_BG, 0);
}

void ui_set_status(const ui_status_t *st)
{
    if (st->time_valid)
        lv_label_set_text_fmt(clock_label, "%02u:%02u", st->hour, st->minute);
    else
        lv_label_set_text(clock_label, "--:--");

    if (st->wifi_connected && st->wifi_ssid)
    {
        lv_label_set_text_fmt(wifi_label, LV_SYMBOL_WIFI " %s", st->wifi_ssid);
        lv_obj_set_style_text_color(wifi_label, COL_BLUE, 0);
    }
    else
    {
        lv_label_set_text(wifi_label, LV_SYMBOL_WIFI " --");
        lv_obj_set_style_text_color(wifi_label, COL_GRAY, 0);
    }
}

void ui_set_can_status(bool ok)
{
    for (int i = 0; i < 4; i++)
        lv_obj_set_style_bg_color(can_bars[i], ok ? COL_BLUE : COL_GRAY, 0);
}

void ui_set_dtc(uint8_t count, const uint16_t *codes)
{
    if (count == 0 || !codes)
    {
        lv_label_set_text(dtc_label, "");
        return;
    }

    static char buf[64];
    buf[0] = '\0';
    int pos = 0;
    for (int i = 0; i < count && i < CAN_DTC_MAX; i++)
    {
        const char *letter = "P";
        switch ((codes[i] >> 14) & 0x3)
        {
        case 1: letter = "C"; break;
        case 2: letter = "B"; break;
        case 3: letter = "U"; break;
        default: letter = "P"; break;
        }
        int num = codes[i] & 0x3FFF;
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%s%03X  ", letter, num);
    }
    lv_label_set_text(dtc_label, buf);
}