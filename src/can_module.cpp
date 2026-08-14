#include "can_module.h"
#include <Arduino.h>
#include <driver/twai.h>

#define CAN_TX_PIN GPIO_NUM_25
#define CAN_RX_PIN GPIO_NUM_14

#define CAN_FRAME_MS 100      /* schedule period per PID */
#define CAN_FRESH_MS 1000     /* data considered stale after this */

/* OBD-II query: 7DF -> ECU response 7E8 */
#define CAN_ID_REQUEST 0x7DF
#define CAN_ID_RESPONSE 0x7E8

#define PID_ENGINE_COOLANT 0x05
#define PID_ENGINE_RPM 0x0C
#define PID_VEHICLE_SPEED 0x0D
#define PID_OIL_TEMP 0x5C
#define PID_MAF 0x10
#define PID_THROTTLE 0x11
#define PID_ENGINE_LOAD 0x04
#define PID_FUEL_LEVEL 0x2F
#define PID_BATTERY_VOLTAGE 0x42

/* service 03 (read DTCs), no PID byte in request */
#define OBD_SVC_READ_DTC 0x03

static can_data_t g_can = {0};
static portMUX_TYPE g_can_lock = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t g_rx_count = 0;

static volatile int g_state = -1;
static volatile uint32_t g_tx_fail = 0;
static volatile uint32_t g_bus_err = 0;

static void can_update_fresh(void)
{
    portENTER_CRITICAL(&g_can_lock);
    g_can.fresh = (millis() - g_can.last_ms) < CAN_FRESH_MS;
    portEXIT_CRITICAL(&g_can_lock);
}

static void can_handle_rx(const twai_message_t *msg)
{
    /* OBD-II response IDs: 0x7E8 - 0x7EF (ISO 15765-4) */
    if (msg->identifier < 0x7E8 || msg->identifier > 0x7EF || msg->data_length_code < 4)
        return;

    /* OBD response: PCI byte, then 0x41 (mode 1 response), PID, data */
    if (msg->data[1] == 0x43)
    {
        /* DTC response (mode 03): 43 0N DTC DTC ... */
        int n = msg->data[2];
        if (n > CAN_DTC_MAX) n = CAN_DTC_MAX;
        portENTER_CRITICAL(&g_can_lock);
        g_can.dtc_count = n;
        for (int i = 0; i < n; i++)
        {
            uint8_t hi = msg->data[3 + i * 2];
            uint8_t lo = msg->data[4 + i * 2];
            g_can.dtc_codes[i] = (hi << 8) | lo;
        }
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        Serial.printf("CAN: DTC count=%d\n", n);
        for (int i = 0; i < n; i++)
            Serial.printf("CAN:   DTC[%d]=%03X\n", i, g_can.dtc_codes[i]);
        return;
    }

    if (msg->data[1] != 0x41)
        return;

    switch (msg->data[2])
    {
    case PID_ENGINE_RPM:
        portENTER_CRITICAL(&g_can_lock);
        g_can.rpm = (msg->data[3] * 256U + msg->data[4]) / 4U;
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    case PID_VEHICLE_SPEED:
        portENTER_CRITICAL(&g_can_lock);
        g_can.speed = msg->data[3];
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    case PID_ENGINE_COOLANT:
        portENTER_CRITICAL(&g_can_lock);
        g_can.coolant_c = msg->data[3] - 40;
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    case PID_ENGINE_LOAD:
        portENTER_CRITICAL(&g_can_lock);
        g_can.load_pct = msg->data[3] * 100U / 255U;
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    case PID_OIL_TEMP:
        portENTER_CRITICAL(&g_can_lock);
        g_can.oil_c = msg->data[3] - 40;
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    case PID_MAF:
        portENTER_CRITICAL(&g_can_lock);
        g_can.maf_gs = msg->data[3] * 256U + msg->data[4];
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    case PID_THROTTLE:
        portENTER_CRITICAL(&g_can_lock);
        g_can.throttle_pct = msg->data[3] * 100U / 255U;
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    case PID_FUEL_LEVEL:
        portENTER_CRITICAL(&g_can_lock);
        g_can.fuel_pct = msg->data[3] * 100U / 255U;
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    case PID_BATTERY_VOLTAGE:
        portENTER_CRITICAL(&g_can_lock);
        g_can.batt_mv = msg->data[3] * 256U + msg->data[4];
        g_can.last_ms = millis();
        can_update_fresh();
        portEXIT_CRITICAL(&g_can_lock);
        break;
    default:
        break;
    }
}

static void can_send_pid(uint8_t pid)
{
    twai_message_t msg = {0};
    msg.identifier = CAN_ID_REQUEST;
    msg.data_length_code = 8;
    msg.data[0] = 0x02;
    msg.data[1] = 0x01;
    msg.data[2] = pid;
    twai_transmit(&msg, pdMS_TO_TICKS(10));
}

static void can_send_dtc(void)
{
    twai_message_t msg = {0};
    msg.identifier = CAN_ID_REQUEST;
    msg.data_length_code = 8;
    msg.data[0] = 0x02;
    msg.data[1] = OBD_SVC_READ_DTC;
    twai_transmit(&msg, pdMS_TO_TICKS(10));
}

static void can_task(void *arg)
{
    (void)arg;
    uint8_t pid_seq[] = {PID_ENGINE_COOLANT, PID_ENGINE_RPM, PID_ENGINE_RPM,
                         PID_VEHICLE_SPEED, PID_ENGINE_RPM, PID_ENGINE_LOAD,
                         PID_OIL_TEMP, PID_MAF, PID_THROTTLE, PID_FUEL_LEVEL,
                         PID_BATTERY_VOLTAGE, PID_ENGINE_RPM, PID_VEHICLE_SPEED};
    uint8_t idx = 0;
    uint32_t last_req = 0;
    uint32_t last_dtc = 0;

    for (;;)
    {
        if ((millis() - last_req) >= CAN_FRAME_MS)
        {
            can_send_pid(pid_seq[idx % sizeof(pid_seq)]);
            last_req = millis();
            idx++;
        }

        /* request DTCs once per second */
        if ((millis() - last_dtc) >= 1000)
        {
            can_send_dtc();
            last_dtc = millis();
        }

        twai_message_t msg;
        if (twai_receive(&msg, pdMS_TO_TICKS(20)) == ESP_OK)
        {
            g_rx_count++;
            can_handle_rx(&msg);
        }

        static uint32_t last_status = 0;
        if ((millis() - last_status) >= 3000)
        {
            twai_status_info_t st;
            if (twai_get_status_info(&st) == ESP_OK)
            {
                g_state = st.state;
                g_tx_fail = st.tx_failed_count;
                g_bus_err = st.bus_error_count;
                Serial.printf("CAN: data=%s rx=%u state=%d tx_fail=%u bus_err=%u\n",
                              g_can.fresh ? "OK" : "NO", g_rx_count, st.state,
                              st.tx_failed_count, st.bus_error_count);
            }
            last_status = millis();
        }

        can_update_fresh();
    }
}

void can_module_init(void)
{
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN,
                                                          TWAI_MODE_NORMAL);
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g, &t, &f) == ESP_OK)
    {
        if (twai_start() == ESP_OK)
        {
            xTaskCreatePinnedToCore(can_task, "can", 4096, NULL, 5, NULL, 1);
            Serial.println("CAN: TWAI started (500k, TX=25 RX=14)");
            return;
        }
        twai_driver_uninstall();
    }
    Serial.println("CAN: TWAI init FAILED");
}

bool can_module_get(can_data_t *out)
{
    if (!out)
        return false;
    portENTER_CRITICAL(&g_can_lock);
    can_update_fresh();
    *out = g_can;
    bool fresh = g_can.fresh;
    portEXIT_CRITICAL(&g_can_lock);
    return fresh;
}

void can_module_get_debug(uint32_t *rx, int *state, uint32_t *tx_fail, uint32_t *bus_err)
{
    if (rx)
        *rx = g_rx_count;
    if (state)
        *state = g_state;
    if (tx_fail)
        *tx_fail = g_tx_fail;
    if (bus_err)
        *bus_err = g_bus_err;
}