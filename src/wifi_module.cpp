#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include <esp_sntp.h>
#include "wifi_module.h"
#include "wifi_config.h"

#define TZ_OFFSET_SEC (7 * 3600) /* UTC+7 */

static volatile bool g_connected = false;
static volatile bool g_time_valid = false;
static volatile uint8_t g_hour = 0;
static volatile uint8_t g_minute = 0;

static void ntp_sync(void)
{
    configTime(TZ_OFFSET_SEC, 0, "pool.ntp.org", "time.nist.gov");
    sntp_set_sync_interval(60000); /* retry every 60s if first attempt fails */

    time_t now = 0;
    int tries = 0;
    while (tries < 30) /* up to 30s */
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        now = time(nullptr);
        if (now > 1000000000) /* year >= 2001 => synced */
            break;
        tries++;
    }

    if (now > 1000000000)
    {
        struct tm ti;
        localtime_r(&now, &ti);
        g_hour = ti.tm_hour;
        g_minute = ti.tm_min;
        g_time_valid = true;
        Serial.printf("NTP: time %02u:%02u\n", g_hour, g_minute);
    }
    else
    {
        g_time_valid = false;
        Serial.println("NTP: fail");
    }
}

static void wifi_task(void *arg)
{
    (void)arg;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 60)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        tries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        g_connected = true;
        Serial.printf("WIFI: connected %s (%s)\n", WIFI_SSID,
                      WiFi.localIP().toString().c_str());
        ntp_sync();
        /* keep WiFi alive for potential re-sync; NTP is enough */
        vTaskDelay(pdMS_TO_TICKS(600000)); /* re-sync every 10 min */
        while (true)
        {
            ntp_sync();
            vTaskDelay(pdMS_TO_TICKS(600000));
        }
    }
    else
    {
        Serial.printf("WIFI: fail to connect %s\n", WIFI_SSID);
        g_connected = false;
        g_time_valid = false;
    }

    vTaskDelete(NULL);
}

void wifi_module_init(void)
{
    xTaskCreatePinnedToCore(wifi_task, "wifi", 8192, NULL, 3, NULL, 0);
}

bool wifi_module_get(wifi_info_t *out)
{
    if (!out)
        return false;
    out->hour = g_hour;
    out->minute = g_minute;
    out->time_valid = g_time_valid;
    out->connected = g_connected;
    out->ssid = g_connected ? WIFI_SSID : NULL;
    return g_connected;
}