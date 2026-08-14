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

static can_data_t g_can = {0};
static volatile uint32_t g_rx_count = 0;

static void can_update_fresh(void)
{
    g_can.fresh = (millis() - g_can.last_ms) < CAN_FRESH_MS;
}

static void can_handle_rx(const twai_message_t *msg)
{
    if (msg->identifier != CAN_ID_RESPONSE || msg->data_length_code < 6)
        return;

    /* OBD response: [2][0x41][PID][data...] */
    if (msg->data[0] != 0x02 || msg->data[1] != 0x41)
        return;

    switch (msg->data[2])
    {
    case PID_ENGINE_RPM:
        g_can.rpm = (msg->data[3] * 256U + msg->data[4]) / 4U;
        g_can.last_ms = millis();
        can_update_fresh();
        break;
    case PID_VEHICLE_SPEED:
        g_can.speed = msg->data[3];
        g_can.last_ms = millis();
        can_update_fresh();
        break;
    case PID_ENGINE_COOLANT:
        g_can.coolant_c = msg->data[3] - 40;
        g_can.last_ms = millis();
        can_update_fresh();
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

static void can_task(void *arg)
{
    (void)arg;
    uint8_t pid_seq[] = {PID_ENGINE_COOLANT, PID_ENGINE_RPM, PID_ENGINE_RPM,
                         PID_VEHICLE_SPEED, PID_ENGINE_RPM};
    uint8_t idx = 0;
    uint32_t last_req = 0;

    for (;;)
    {
        if ((millis() - last_req) >= CAN_FRAME_MS)
        {
            can_send_pid(pid_seq[idx % sizeof(pid_seq)]);
            last_req = millis();
            idx++;
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
                Serial.printf("CAN: data=%s rx=%u state=%d tx_fail=%u bus_err=%u\n",
                              g_can.fresh ? "OK" : "NO", g_rx_count, st.state,
                              st.tx_failed_count, st.bus_error_count);
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
    *out = g_can;
    can_update_fresh();
    return g_can.fresh;
}