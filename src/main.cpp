#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "soc/gpio_reg.h"

#define PIN_BL 27
#define W 320
#define H 480

static const int MOSI = 23, SCLK = 18, CS = 15, DC = 2;

static const int rst_rel[] = {0, 4, 5, 12, 14, 16, 17, 19, 21, 22, 25, 26, 32, 33};
#define NRST (int)(sizeof(rst_rel) / sizeof(rst_rel[0]))

static bool g_lofirst = false;
static uint8_t g_madctl = 0xC8;

static void pl(int pin, int level)
{
    if (level)
        REG_WRITE(GPIO_OUT_W1TS_REG, (uint32_t)1 << pin);
    else
        REG_WRITE(GPIO_OUT_W1TC_REG, (uint32_t)1 << pin);
}

static void w_byte(uint8_t d)
{
    for (int i = 7; i >= 0; i--)
    {
        pl(MOSI, (d >> i) & 1);
        pl(SCLK, 1);
        pl(SCLK, 0);
    }
}

static void w_cmd(uint8_t cmd, const uint8_t *data, size_t len)
{
    pl(CS, 0); pl(DC, 0); w_byte(cmd);
    if (len)
    {
        pl(DC, 1);
        for (size_t i = 0; i < len; i++)
            w_byte(data[i]);
    }
    pl(CS, 1);
}

static void init_disp(uint8_t madctl)
{
    for (int i = 0; i < NRST; i++)
    {
        int r = rst_rel[i];
        if (r == MOSI || r == SCLK || r == CS || r == DC || r == PIN_BL || r == 1 || r == 3)
            continue;
        gpio_set_direction((gpio_num_t)r, GPIO_MODE_OUTPUT);
        pl(r, 1);
    }
    gpio_set_direction((gpio_num_t)MOSI, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)SCLK, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CS, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)DC, GPIO_MODE_OUTPUT);
    pl(MOSI, 0); pl(SCLK, 0); pl(CS, 1); pl(DC, 0);

    uint8_t x;
    w_cmd(0x01, NULL, 0); vTaskDelay(pdMS_TO_TICKS(150));
    w_cmd(0x11, NULL, 0); vTaskDelay(pdMS_TO_TICKS(120));
    x = madctl; w_cmd(0x36, &x, 1);
    x = 0x55; w_cmd(0x3A, &x, 1);
    w_cmd(0x20, NULL, 0);
    w_cmd(0x29, NULL, 0);
}

static void fill_full(uint16_t col)
{
    uint8_t ca[4] = {0, 0, (uint8_t)((W - 1) >> 8), (uint8_t)((W - 1) & 0xFF)};
    uint8_t ra[4] = {0, 0, (uint8_t)((H - 1) >> 8), (uint8_t)((H - 1) & 0xFF)};
    pl(CS, 0);
    pl(DC, 0); w_byte(0x2A);
    pl(DC, 1); for (int i = 0; i < 4; i++) w_byte(ca[i]);
    pl(DC, 0); w_byte(0x2B);
    pl(DC, 1); for (int i = 0; i < 4; i++) w_byte(ra[i]);
    pl(DC, 0); w_byte(0x2C);
    pl(DC, 1);
    int total = W * H;
    uint8_t hi = col >> 8, lo = col & 0xFF;
    while (total--)
    {
        if (g_lofirst)
        {
            w_byte(lo);
            w_byte(hi);
        }
        else
        {
            w_byte(hi);
            w_byte(lo);
        }
    }
    pl(CS, 1);
}

struct combo_t { const char *name; uint8_t madctl; bool lofirst; };
static const combo_t COMBOS[] = {
    {"A BGR=1 HI-first",  0xC8, false},
    {"B BGR=0 HI-first",  0xC0, false},
    {"C BGR=1 LO-first",  0xC8, true },
    {"D BGR=0 LO-first",  0xC0, true },
};
static const uint16_t COLS[] = {0xF800, 0x07E0, 0x001F};
static const char *COLNAMES[] = {"RED", "GREEN", "BLUE"};

void setup()
{
    Serial.begin(115200);
    gpio_set_direction((gpio_num_t)PIN_BL, GPIO_MODE_OUTPUT);
    pl(PIN_BL, 1);
    Serial.println("===== TEST MAU: 4 COMBO x 3 MAU, moi mau 6 giay, lap lai =====");
    for (int i = 0; i < 4; i++)
        Serial.printf("COMBO %s: %s\n", COMBOS[i].name, i == 0 ? "(DANG XEM TRUOC)" : "");
    Serial.println("Ghi danh sach mau XEM DUOC o tung combo, so sanh voi RED/GREEN/BLUE cho dung:");
}

int g_idx = 0;
uint32_t g_t0 = 0;

void loop()
{
    if (millis() - g_t0 < 6000)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
        return;
    }
    g_t0 = millis();

    int combo = g_idx / 3;
    int col = g_idx % 3;
    const combo_t &c = COMBOS[combo];
    uint16_t co = COLS[col];

    g_madctl = c.madctl;
    g_lofirst = c.lofirst;
    init_disp(c.madctl);
    fill_full(co);
    Serial.printf("[%s] %s\n", c.name, COLNAMES[col]);

    g_idx = (g_idx + 1) % 12;
}