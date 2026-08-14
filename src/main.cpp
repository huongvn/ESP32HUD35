#include <Arduino.h>
#include <lvgl.h>
#include <esp32_smartdisplay.h>
#include "ui.h"

static int g_speed = 0;
static int g_coolant = 20;
static bool g_accel = true;
static uint32_t g_odo = 100123;
static lv_obj_t *touch_label;

static void sim_timer_cb(lv_timer_t *timer)
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

    if (g_accel)
    {
        g_speed += 2;
        if (g_speed >= 115)
            g_accel = false;
    }
    else
    {
        g_speed -= 2;
        if (g_speed <= 0)
        {
            g_speed = 0;
            g_accel = true;
        }
    }

    if (g_coolant < 92)
        g_coolant += 1;

    int rpm;
    if (g_speed < 1)
        rpm = 850;
    else
    {
        rpm = 1500 + g_speed * 45;
        if (rpm > 6500)
            rpm = 6500;
    }

    int gear = (g_speed < 1) ? 0 : 3; // P khi dừng, D khi chạy

    g_odo += g_speed / 10;

    ui_update(g_speed, g_coolant, rpm, gear, g_odo);
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

    lv_timer_t *t = lv_timer_create(sim_timer_cb, 100, NULL);
    lv_timer_ready(t);
}

void loop()
{
    lv_timer_handler();
    delay(5);
}