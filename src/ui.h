#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <stdint.h>

#include "can_module.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t hour;
    uint8_t minute;
    bool time_valid;
    bool wifi_connected;
    const char *wifi_ssid; /* may be NULL */
} ui_status_t;

void ui_init(void);
void ui_update(int speed_kmh, int coolant_c, int rpm, int load_pct,
               int iat_c, int throttle_pct, int batt_mv);
void ui_set_status(const ui_status_t *st);
void ui_set_can_status(bool ok);
void ui_set_dtc(uint8_t count, const uint16_t *codes);
void ui_set_scanning(bool scanning);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */