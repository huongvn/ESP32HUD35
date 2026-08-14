#include <Arduino.h>
#include <lvgl.h>
#include <esp32_smartdisplay.h>
#include "ui.h"
#include "can_module.h"
#include "wifi_module.h"

static void hud_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    can_data_t can;
    bool ok = can_module_get(&can);
    ui_set_can_status(ok);

    if (ok)
        ui_update(can.speed, can.coolant_c, can.rpm, can.load_pct,
                  can.oil_c, can.throttle_pct, can.batt_mv);
    else
        ui_update(0, 0, 0, 0, 0, 0, 0);

    wifi_info_t wi;
    wifi_module_get(&wi);
    ui_status_t st;
    st.hour = wi.hour;
    st.minute = wi.minute;
    st.time_valid = wi.time_valid;
    st.wifi_connected = wi.connected;
    st.wifi_ssid = wi.connected ? wi.ssid : NULL;
    ui_set_status(&st);
}

void setup()
{
    Serial.begin(115200);
    smartdisplay_init();
    lv_tick_set_cb(millis);
    smartdisplay_lcd_set_backlight(1.0f);

    ui_init();
    wifi_module_init();
    can_module_init();

    lv_timer_t *t = lv_timer_create(hud_timer_cb, 100, NULL);
    lv_timer_ready(t);
}

void loop()
{
    lv_timer_handler();
    delay(5);
}