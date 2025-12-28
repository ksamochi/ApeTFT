/********************************************************************************/
/* myOdoTrip                                                                    */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: -                                                                 */
/********************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include <driver/pcnt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include "ui/ui.h"

#include "myTacho.h"

#include "myOdoTrip.h"

/* ---------------------------------------------------------------- Definitions */
#define ODO_PULSE_PIN (14)
#define ODO_PULSE_CH PCNT_CHANNEL_0
#define ODO_PULSE_UNIT PCNT_UNIT_0

#define ODO_RATE_DEF (7692U)

/* ------------------------------------------------------------------ Variables */
static Preferences prefs;

static uint32_t OdoRate;
static uint32_t OdoPulse;
static uint32_t Odo;

static uint32_t rstPulse;
static uint32_t rstMajor;

static uint32_t Trip;

static int CalibPls;

static bool EngOn;

/* ------------------------------------------------------- Function Proto-Types */
static uint32_t calc_trip(uint32_t rate, uint32_t major, uint32_t pulse, uint32_t rstmajor, uint32_t rstpulse);
static void calib_pls(uint32_t in);

/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void myOdoTrip_Init(void)
{
    Serial.println("myOdoTrip_Init.");

    /* load setting */
    prefs.begin("MetSettings", true);
    OdoRate = (uint32_t)prefs.getInt("odorate", ODO_RATE_DEF);
    uint32_t odoset = (uint32_t)prefs.getInt("odoset", 0);
    prefs.remove("odoset");
    prefs.end();

    /* reload odo/trip info */
    prefs.begin("MetLog", false);
    if (odoset > 0) prefs.putInt("odo", odoset <<12);
    uint32_t odoread = (uint32_t)prefs.getInt("odo", 0);
    OdoPulse = odoread & 0x00000FFF;
    Odo = (odoread & 0xFFFFF000) >>12;

    uint32_t tripreset = (uint32_t)prefs.getInt("triprst", 0);
    rstPulse = tripreset & 0x00000FFF;
    rstMajor = (tripreset & 0xFFFFF000) >>12;
    
    Trip = calc_trip(OdoRate, Odo, OdoPulse, rstMajor, rstPulse);

    /* pulse count setting */
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = ODO_PULSE_PIN,
        .ctrl_gpio_num  = PCNT_PIN_NOT_USED,
        .lctrl_mode     = PCNT_MODE_KEEP,
        .hctrl_mode     = PCNT_MODE_KEEP,
        .pos_mode       = PCNT_COUNT_DIS,
        .neg_mode       = PCNT_COUNT_INC,
        .counter_h_lim  = 255,
        .counter_l_lim  = 0,
        .unit           = ODO_PULSE_UNIT,
        .channel        = ODO_PULSE_CH,
    };
    pcnt_unit_config(&pcnt_config);
    pcnt_counter_clear(ODO_PULSE_UNIT);
    pcnt_counter_resume(ODO_PULSE_UNIT);

    EngOn = true;
}

/* ----------------------------------------------------------------------- main */
void myOdoTrip_Main(void)
{
    int16_t pulsein = 0;
    noInterrupts();
    pcnt_get_counter_value(ODO_PULSE_UNIT, &pulsein);
    pcnt_counter_clear(ODO_PULSE_UNIT);
    interrupts();

    /* integrate pulse & store odo per 1km */
    OdoPulse += pulsein;
    if (OdoPulse >= OdoRate) {
        OdoPulse -= OdoRate;
        Odo++;
        uint32_t odoset = (Odo <<12) | OdoPulse;
        prefs.putInt("odo", (int)odoset);
    }
    Trip = calc_trip(OdoRate, Odo, OdoPulse, rstMajor, rstPulse);

    /* judge ig-off & store odo data */
    bool nowEngOn = myTacho_EngOn();
    if (EngOn && !nowEngOn) {
        uint32_t odoset = (Odo <<12) | OdoPulse;
        prefs.putInt("odo", (int)odoset);
    }
    EngOn = nowEngOn;

    /* pulse calibration */
    calib_pls(pulsein);

}

/* ------------------------------------------------------------------ interface */
uint32_t myOdoTrip_getOdo(void)
{
    return Odo;
}
uint32_t myOdoTrip_getTrip(void)
{
    return Trip;
}
void myOdoTrip_TripReset(void)
{
    uint32_t odoset = (Odo <<12) | OdoPulse;
    prefs.putInt("odo", (int)odoset);
    prefs.putInt("triprst", (int)odoset);
    rstPulse = OdoPulse;
    rstMajor = Odo;
}

/* ------------------------------------------------------ Calibration interface */
void myOdoTrip_CalibPlsReset(void)
{
    CalibPls = 0;
}

uint32_t myOdoTrip_getCalibPls(void)
{
    return CalibPls;
}

void myOdoTrip_CalibOdoPlsRate(void)
{
    prefs.begin("MetSettings", false);
    prefs.putInt("odorate", CalibPls);
    prefs.end();
}

void myOdoTrip_CalibSpPlsRate(void)
{
    prefs.begin("MetSettings", false);
    prefs.putInt("spdrate", CalibPls * 36000);
    prefs.end();
}


/* **************************************************************** Sub Process */
/* ------------------------------------------------------------------ calc_trip */
static uint32_t calc_trip(uint32_t rate, uint32_t major, uint32_t pulse, uint32_t rstmajor, uint32_t rstpulse)
{
    int32_t d_major = (int32_t)major - (int32_t)rstmajor;
    int32_t d_pulse = (int32_t)pulse - (int32_t)rstpulse;

    if (d_pulse < 0) {
        d_major -= 1;
        d_pulse += (int32_t)rate;
    }
    int32_t adj = d_pulse / (int32_t)rate;
    int32_t trip = d_major + adj;

    if (trip < 0) trip = 0;
    return (uint32_t)trip;
}

/* -------------------------------------------------------------- calib_pls_adj */
static void calib_pls(uint32_t plsin)
{
    lv_obj_t* screen = lv_scr_act();
    if (screen != ui_PlsSet) {
        CalibPls = 0;
        return;
    }
    
    CalibPls += plsin;

    /* detect roller adjustment */
    static int prev = 0;
    static int value = 0;
    int now = lv_roller_get_selected(ui_PlsSet_PlsSetPlsRoll);
    int diff = now - prev;
    prev = now;
    if(diff > 5) diff -= 10;
    if(diff < -5) diff += 10;
    CalibPls += diff;

    if (CalibPls < 0) CalibPls = 0;
    if (CalibPls > 99999) CalibPls = 99999;

    char buf[8];
    snprintf(buf, sizeof(buf), "%05d", CalibPls);
    lv_textarea_set_text(ui_PlsSet_PlsSetCalibPls, buf);
    
}