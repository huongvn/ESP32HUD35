#ifndef UI_H
#define UI_H

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
    bool wifi_connected;
    const char *wifi_ssid; /* may be NULL */
} ui_status_t;

void ui_init(void);
void ui_update(int speed_kmh, int coolant_c, int rpm, int load_pct,
               int oil_c, int throttle_pct, int batt_mv);
void ui_set_status(const ui_status_t *st);
void ui_set_can_status(bool ok);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */