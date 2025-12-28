/********************************************************************************/
/* mySpeed                                                                      */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: -                                                                 */
/********************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include <esp_timer.h>
#include <driver/mcpwm_cap.h>

#include "mySpeed.h"

/* ---------------------------------------------------------------- Definitions */
#define SP_PULSE_PIN (16)

#define SP_AVERAGE (4)
#define SP_RATE_DEF (276912000U)
#define SP_DELAY_DEF (10)

/* ------------------------------------------------------------------ Variables */
static Preferences prefs;

static uint32_t SpPlsRate;
static uint32_t SpDelay;

static mcpwm_cap_timer_handle_t cap_tm;
static mcpwm_cap_channel_handle_t cap_ch;

static volatile bool SpPls1st;
static volatile uint32_t SpPlsStore[SP_AVERAGE];
static volatile uint32_t SpPlsIndex;
static volatile uint32_t SpPlsLastCap;
static volatile uint64_t SpPlsLastTime;

static uint32_t SpRaw;
static uint32_t SpDisp;

/* ------------------------------------------------------- Function Proto-Types */
static bool IRAM_ATTR capt_cb(mcpwm_cap_channel_handle_t cap_channel, const mcpwm_capture_event_data_t *edata, void *user_ctx);


/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void mySpeed_Init(void)
{
    Serial.println("mySpeed_Init.");

    /* load setting */
    prefs.begin("MetSettings", true);
    SpPlsRate = (uint32_t)prefs.getInt("spdrate", SP_RATE_DEF);
    SpDelay = (uint32_t)prefs.getInt("spddelay", SP_DELAY_DEF);
    prefs.end();

    /* initialise pulse buffer */
    SpPls1st = true;
    SpPlsLastTime = 0;
    for (uint32_t i = 0; i < SP_AVERAGE; i++) SpPlsStore[i] = 0xFFFFFFFF;

    /* port config */
    //mcpwm_gpio_init(SP_PULSE_UNIT, SP_PULSE_SIG, SP_PULSE_PIN);

    //mcpwm_capture_timer_config_t cap_tmcfg = {
    //    .group_id = 0, .clk_src = 0, .resolution_hz = 0, .flags = { .allow_pd = 0 }, };
    mcpwm_capture_timer_config_t cap_tmcfg = { .group_id = 0, };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_tmcfg, &cap_tm));

    mcpwm_capture_channel_config_t cap_chcfg = {
        .gpio_num = SP_PULSE_PIN, .prescale = 80, .flags = { .pos_edge = false, .neg_edge = true,  }, };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_tm, &cap_chcfg, &cap_ch));

    mcpwm_capture_event_callbacks_t cbs = { .on_cap = capt_cb, };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_ch, &cbs, NULL));

    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_tm));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_ch));

//    mcpwm_capture_config_t cap_conf;
//    cap_conf.cap_edge = MCPWM_NEG_EDGE;
//    cap_conf.cap_prescale = 80;
//    cap_conf.capture_cb = capture_isr;
//    cap_conf.user_data = NULL;
//    mcpwm_capture_enable(SP_PULSE_UNIT, SP_PULSE_SIG, &cap_conf);

    SpRaw = 0U;
    SpDisp = 0U;
}

/* ----------------------------------------------------------------------- main */
void mySpeed_Main(void)
{
    /* store pulse interval */
    uint64_t pls_interval = esp_timer_get_time() - SpPlsLastTime;
    if (pls_interval >= 0x00000000FFFFFFFFUL) {
        // If no pulse arrives for more than 71 minutes,
        // initialization will already have been performed in the previous branch,
        // so no further processing is required here.
    } else if (pls_interval >= 100000) {
        noInterrupts();
        SpPlsStore[SpPlsIndex] = pls_interval;
        SpPlsIndex = (SpPlsIndex +1) % SP_AVERAGE;
        interrupts();
        SpPls1st = true;
    }

    /* calculate speed raw */
    uint32_t spcalc, spmin = UINT32_MAX, spmax = 0, sptemp = 0;
    for (uint32_t i = 0; i < SP_AVERAGE; i++) {
        if (SpPlsStore[i] != 0) {
            spcalc = SpPlsRate / SpPlsStore[i];
        } else {
            spcalc = 0;
        }
        sptemp += spcalc;
        if (spcalc < spmin) spmin = spcalc;
        if (spmax < spcalc) spmax = spcalc;
    }
    SpRaw = (sptemp - spmin - spmax) / (SP_AVERAGE - 2);

    /* update delayed display value */
    int32_t spdelta = ((int32_t)SpRaw - (int32_t)SpDisp) / (int32_t)SpDelay;
    SpDisp = (uint32_t)((int32_t)SpDisp + spdelta);
}

/* ------------------------------------------------------------------ interface */
uint32_t mySpeed_Raw()
{
    return SpRaw;
}
uint32_t mySpeed_Disp()
{
    return SpDisp;
}


/* **************************************************************** Sub Process */
static bool IRAM_ATTR capt_cb(mcpwm_cap_channel_handle_t cap_channel, const mcpwm_capture_event_data_t *edata, void *user_ctx)
{
    uint32_t cap_val = edata->cap_value;
    if (!SpPls1st){
        uint32_t diff = cap_val - SpPlsLastCap;
        SpPlsStore[SpPlsIndex] = diff;
        SpPlsIndex = (SpPlsIndex +1) % SP_AVERAGE;
    }
    SpPls1st = false;
    SpPlsLastCap = cap_val;
    SpPlsLastTime = esp_timer_get_time();

    return true;
}
