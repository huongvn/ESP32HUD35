#ifndef CAN_MODULE_H
#define CAN_MODULE_H

#include <stdbool.h>
#include <stdint.h>

#define CAN_DTC_MAX 6 /* max stored DTCs kept in struct */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool fresh;          /* data valid (received recently) */
    uint32_t last_ms;    /* millis() of last valid frame */
    uint16_t rpm;        /* engine speed RPM */
    uint8_t speed;       /* km/h */
    int coolant_c;       /* coolant temperature degC, -40..215 */
    uint8_t load_pct;    /* engine load % */
    int iat_c;           /* intake air temperature degC */
    uint16_t maf_gs;     /* MAF g/s * 100 */
    uint8_t throttle_pct;/* throttle position % */
    uint8_t fuel_pct;    /* fuel level % */
    uint16_t batt_mv;    /* battery voltage mV */
    uint8_t dtc_count;   /* number of stored DTCs */
    uint16_t dtc_codes[CAN_DTC_MAX]; /* raw 2-byte OBD DTC codes */
} can_data_t;

void can_module_init(void);
bool can_module_get(can_data_t *out);
void can_module_get_debug(uint32_t *rx, int *state, uint32_t *tx_fail, uint32_t *bus_err);

#ifdef __cplusplus
}
#endif

#endif /* CAN_MODULE_H */