#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t hour;
    uint8_t minute;
    bool time_valid;
    bool connected;
    const char *ssid;
} wifi_info_t;

void wifi_module_init(void);
bool wifi_module_get(wifi_info_t *out);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MODULE_H */