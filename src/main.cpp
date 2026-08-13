#include <Arduino.h>
#include <lvgl.h>
#include <esp32_smartdisplay.h>

static lv_obj_t *scr;
static int g_idx = 0;
static uint32_t g_last_hb = 0;

static void color_phase_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    static const uint32_t cols[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF, 0x000000};
    static const char *names[] = {"DO", "XANH LA", "XANH DUONG", "TRANG", "DEN"};
    lv_color_t c = lv_color_hex(cols[g_idx]);
    lv_obj_set_style_bg_color(scr, c, LV_STATE_DEFAULT);
    Serial.printf("[PHA %d] %s rgb=(%d,%d,%d)\n", g_idx + 1, names[g_idx], c.red, c.green, c.blue);
    g_idx = (g_idx + 1) % 5;
}

void setup()
{
    Serial.begin(115200);
    smartdisplay_init();
    lv_tick_set_cb(millis);
    smartdisplay_lcd_set_backlight(1.0f);

    scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF800), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);

    lv_timer_t *t = lv_timer_create(color_phase_timer_cb, 5000, NULL);
    lv_timer_ready(t);
    Serial.println("READY");
}

void loop()
{
    if (millis() - g_last_hb >= 1000)
    {
        g_last_hb = millis();
        Serial.printf("HB %lu\n", (unsigned long)millis());
    }
    lv_timer_handler();
    delay(5);
}