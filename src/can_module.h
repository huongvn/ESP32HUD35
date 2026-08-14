#ifndef CAN_MODULE_H
#define CAN_MODULE_H

#include <stdbool.h>
#include <stdint.h>

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
} can_data_t;

void can_module_init(void);
bool can_module_get(can_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* CAN_MODULE_H */