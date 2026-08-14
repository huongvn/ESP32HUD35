#include <Arduino.h>
#include <lvgl.h>
#include <esp32_smartdisplay.h>
#include "ui.h"
#include "can_module.h"

static lv_obj_t *can_label;

static void hud_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    can_data_t can;
    bool ok = can_module_get(&can);

    if (ok)
    {
        lv_label_set_text(can_label, "CAN OK");
        lv_obj_set_style_text_color(can_label, lv_color_hex(0x00FF00), 0);
        ui_update(can.speed, can.coolant_c, can.rpm, 0, 0,
                  can.load_pct, can.intake_c, can.throttle_pct, can.batt_mv);
    }
    else
    {
        lv_label_set_text(can_label, "NO CAN");
        lv_obj_set_style_text_color(can_label, lv_color_hex(0xFF0000), 0);
        ui_update(0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

void setup()
{
    Serial.begin(115200);
    smartdisplay_init();
    lv_tick_set_cb(millis);
    smartdisplay_lcd_set_backlight(1.0f);

    ui_init();

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