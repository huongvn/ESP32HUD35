#include <Arduino.h>
#include <lvgl.h>
#include <esp32_smartdisplay.h>
#include "ui.h"
#include "can_module.h"

static lv_obj_t *touch_label;
static lv_obj_t *can_label;

static bool g_can_ok = false;

static void hud_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev)
    {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER)
        {
            lv_point_t pt;
            lv_indev_get_point(indev, &pt);
            if (lv_indev_get_state(indev) & LV_INDEV_STATE_PRESSED)
            {
                lv_label_set_text_fmt(touch_label, "TOUCH %d,%d", pt.x, pt.y);
                Serial.printf("TOUCH %d,%d\n", pt.x, pt.y);
            }
            else
            {
                lv_label_set_text(touch_label, "");
            }
        }
        indev = lv_indev_get_next(indev);
    }

    can_data_t can;
    g_can_ok = can_module_get(&can);

    if (g_can_ok)
    {
        lv_label_set_text(can_label, "CAN OK");
        lv_obj_set_style_text_color(can_label, lv_color_hex(0x00FF00), 0);
        ui_update(can.speed, can.coolant_c, can.rpm, 0, 0);
    }
    else
    {
        lv_label_set_text(can_label, "NO CAN");
        lv_obj_set_style_text_color(can_label, lv_color_hex(0xFF0000), 0);
        ui_update(0, 20, 0, 0, 0);
    }
}

void setup()
{
    Serial.begin(115200);
    smartdisplay_init();
    lv_tick_set_cb(millis);
    smartdisplay_lcd_set_backlight(1.0f);

    ui_init();

    touch_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(touch_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(touch_label, lv_color_hex(0xFFEE00), 0);
    lv_obj_align(touch_label, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_label_set_text(touch_label, "");

    can_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(can_label, &lv_font_montserrat_14, 0);
    lv_obj_align(can_label, LV_ALIGN_TOP_LEFT, 4, 4);
    lv_label_set_text(can_label, "NO CAN");

    can_module_init();

    lv_timer_t *t = lv_timer_create(hud_timer_cb, 100, NULL);
    lv_timer_ready(t);
}

void loop()
{
    lv_timer_handler();
    delay(5);
}