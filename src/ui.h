#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);
void ui_update(int speed_kmh, int coolant_c, int rpm, int gear, uint32_t odo_km,
               int load_pct, int intake_c, int throttle_pct, int batt_mv);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */