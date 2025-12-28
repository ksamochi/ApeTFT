/********************************************************************************/
/* myTacho                                                                      */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: -                                                                 */
/********************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include <esp_timer.h>
#include <driver/mcpwm_cap.h>

#include "myTacho.h"

/* ---------------------------------------------------------------- Definitions */
#define TA_PULSE_PIN (46)

#define TA_AVERAGE (4)
#define TA_RATE_DEF (120000000U)
#define TA_DELAY_DEF (10)

#define ENG_ON_THRESH (2000)
#define ENG_OFF_THRESH (1000)

/* ------------------------------------------------------------------ Variables */
static Preferences prefs;

static uint32_t TaPlsRate;
static uint32_t TaDelay;

static mcpwm_cap_timer_handle_t cap_tm;
static mcpwm_cap_channel_handle_t cap_ch;

static volatile bool TaPls1st;
static volatile uint32_t TaPlsStore[TA_AVERAGE];
static volatile uint32_t TaPlsIndex;
static volatile uint32_t TaPlsLastCap;
static volatile uint64_t TaPlsLastTime;

static uint32_t TaRaw;
static uint32_t TaDisp;
static bool EngON;
static uint32_t EngOnCnt;

/* ------------------------------------------------------- Function Proto-Types */
static bool IRAM_ATTR capt_cb(mcpwm_cap_channel_handle_t cap_channel, const mcpwm_capture_event_data_t *edata, void *user_ctx);

/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void myTacho_Init(void)
{
    Serial.println("myTacho_Init.");
    
    /* load setting */
    prefs.begin("MetSettings", true);
    TaPlsRate = (uint32_t)prefs.getInt("tarate", TA_RATE_DEF);
    TaDelay = (uint32_t)prefs.getInt("tadelay", TA_DELAY_DEF);
    prefs.end();

    /* initialise pulse buffer */
    TaPls1st = true;
    TaPlsLastTime = 0;
    for (uint32_t i = 0; i < TA_AVERAGE; i++) TaPlsStore[i] = 0xFFFFFFFF;

    /* port config */
    //mcpwm_capture_timer_config_t cap_tmcfg = {
    //    .group_id = 0, .clk_src = 0, .resolution_hz = 0, .flags = { .allow_pd = 0 }, };
    mcpwm_capture_timer_config_t cap_tmcfg = { .group_id = 1, };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_tmcfg, &cap_tm));

    mcpwm_capture_channel_config_t cap_chcfg = {
        .gpio_num = TA_PULSE_PIN, .prescale = 80, .flags = { .pos_edge = false, .neg_edge = true,  }, };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_tm, &cap_chcfg, &cap_ch));

    mcpwm_capture_event_callbacks_t cbs = { .on_cap = capt_cb, };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_ch, &cbs, NULL));

    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_tm));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_ch));
    
    TaRaw = 0U;
    TaDisp = 0U;
    EngON = false;
}

/* ----------------------------------------------------------------------- main */
void myTacho_Main(void)
{
    /* store pulse interval */
    uint64_t pls_interval = esp_timer_get_time() - TaPlsLastTime;
    if (pls_interval >= 0x00000000FFFFFFFFUL) {
        // If no pulse arrives for more than 71 minutes,
        // initialization will already have been performed in the previous branch,
        // so no further processing is required here.
    } else if (pls_interval >= 100000) {
        noInterrupts();
        TaPlsStore[TaPlsIndex] = pls_interval;
        TaPlsIndex = (TaPlsIndex +1) % TA_AVERAGE;
        interrupts();
        TaPls1st = true;
    }

    /* calculate speed raw */
    uint32_t tacalc, tamin = UINT32_MAX, tamax = 0, tatemp = 0;
    for (uint32_t i = 0; i < TA_AVERAGE; i++) {
        if (TaPlsStore[i] != 0) {
            tacalc = TaPlsRate / TaPlsStore[i];
        } else {
            tacalc = 0;
        }
        tatemp += tacalc;
        if (tacalc < tamin) tamin = tacalc;
        if (tamax < tacalc) tamax = tacalc;
    }
    TaRaw = (tatemp - tamin - tamax) / (TA_AVERAGE - 2);

    /* update delayed display value */
    int32_t tadelta = ((int32_t)TaRaw - (int32_t)TaDisp) / (int32_t)TaDelay;
    TaDisp = (uint32_t)((int32_t)TaDisp + tadelta);

    /* judge engine on/off */
    if (!EngON && (ENG_ON_THRESH <= TaRaw)) Serial.println(TaRaw);
    if (!EngON && (ENG_ON_THRESH <= TaRaw)) EngON = true;
    if (EngON && (TaRaw <= ENG_OFF_THRESH)) EngON = false;
}

/* ------------------------------------------------------------------ interface */
uint32_t myTacho_Raw(void)
{
    return TaRaw;
}
uint32_t myTacho_Disp(void)
{
    return TaDisp;
}
bool myTacho_EngOn(void)
{
    return EngON;
}


/* **************************************************************** Sub Process */
static bool IRAM_ATTR capt_cb(mcpwm_cap_channel_handle_t cap_channel, const mcpwm_capture_event_data_t *edata, void *user_ctx)
{
    uint32_t cap_val = edata->cap_value;
    if (!TaPls1st){
        uint32_t diff = cap_val - TaPlsLastCap;
        TaPlsStore[TaPlsIndex] = diff;
        TaPlsIndex = (TaPlsIndex +1) % TA_AVERAGE;
    }
    TaPls1st = false;
    TaPlsLastCap = cap_val;
    TaPlsLastTime = esp_timer_get_time();

    return true;
}
