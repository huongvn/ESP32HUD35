#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include <esp_sntp.h>
#include <Preferences.h>
#include "wifi_module.h"
#include "wifi_config.h"

#define TZ_OFFSET_SEC (7 * 3600) /* UTC+7 */

#define NVS_NS   "hudtime"
#define NVS_KEY  "epoch"

static volatile bool g_connected = false;
static volatile bool g_time_valid = false;

/* running clock: now_epoch = g_epoch_base + (millis()-g_base_ms)/1000 */
static volatile time_t g_epoch_base = 0;
static volatile uint32_t g_base_ms = 0;

static void time_store_epoch(time_t now)
{
    Preferences p;
    if (p.begin(NVS_NS, false))
    {
        p.putLong(NVS_KEY, (int32_t)now);
        p.end();
    }
}

/* compute current epoch from base + uptime */
static time_t now_epoch(void)
{
    if (!g_time_valid)
        return 0;
    uint32_t elapsed = (uint32_t)(millis() - g_base_ms);
    return g_epoch_base + (time_t)(elapsed / 1000);
}

static void time_set(time_t now)
{
    g_epoch_base = now;
    g_base_ms = millis();
    g_time_valid = true;
}

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
        time_set(now);
        time_store_epoch(now);
        struct tm ti;
        localtime_r(&now, &ti);
        Serial.printf("NTP: time %02u:%02u\n", ti.tm_hour, ti.tm_min);
    }
    else
    {
        Serial.println("NTP: fail (using stored clock)");
    }
}

static void wifi_task(void *arg)
{
    (void)arg;

    /* restore last known time from flash so clock works before/without WiFi */
    Preferences p;
    if (p.begin(NVS_NS, true))
    {
        int32_t saved = p.getLong(NVS_KEY, 0);
        p.end();
        if (saved > 1000000000)
        {
            time_set((time_t)saved);
            Serial.println("TIME: restored from flash");
        }
    }

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
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(600000)); /* re-sync every 10 min */
            ntp_sync();
        }
    }
    else
    {
        Serial.printf("WIFI: fail to connect %s\n", WIFI_SSID);
        g_connected = false;
    }

    /* keep running clock alive (no WiFi) */
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(60000));
        time_store_epoch(now_epoch());
    }
}

void wifi_module_init(void)
{
    xTaskCreatePinnedToCore(wifi_task, "wifi", 8192, NULL, 3, NULL, 0);
}

bool wifi_module_get(wifi_info_t *out)
{
    if (!out)
        return false;

    if (g_time_valid)
    {
        time_t now = now_epoch();
        struct tm ti;
        localtime_r(&now, &ti);
        out->hour = ti.tm_hour;
        out->minute = ti.tm_min;
        out->time_valid = true;
    }
    else
    {
        out->hour = 0;
        out->minute = 0;
        out->time_valid = false;
    }
    out->connected = g_connected;
    out->ssid = g_connected ? WIFI_SSID : NULL;
    return g_connected;
}