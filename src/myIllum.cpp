/********************************************************************************/
/* myIllum                                                                      */
/* Auth-date: 2025/12/11                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: -                                                                 */
/********************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include "ui/ui.h"

#include "myIllum.h"

/* ---------------------------------------------------------------- Definitions */
#define ILLUM_PIN (1)
#define ILLUM_DARK_DEF (32U)
#define ILLUM_LIGHT_DEF (255U)
#define ILLUM_RESPONSE (10U)

/* ------------------------------------------------------------------ Variables */
static Preferences prefs;
static uint32_t Illum_Dark;
static uint32_t Illum_Light;
static uint32_t Illum_Cur;
static uint32_t Illum_Tag;

/* ------------------------------------------------------- Function Proto-Types */

/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void myIllum_Init(void)
{
    Serial.println("myIllum_Init.");

    prefs.begin("MetSettings", true);
    Illum_Dark = (uint8_t)prefs.getInt("illum_dark", ILLUM_DARK_DEF) *10U;
    Illum_Light = (uint8_t)prefs.getInt("illum_light", ILLUM_LIGHT_DEF) *10U;
    prefs.end();

    Illum_Tag = 1U;
    Illum_Cur = 1U;
}

/* ----------------------------------------------------------------------- main */
void myIllum_Main(void)
{
    lv_obj_t* screen = lv_scr_act();

    if (screen == ui_Opn) {
        if (Illum_Tag < Illum_Light) Illum_Tag += Illum_Light /200U;
    } else if (screen == ui_Dark) {
        Illum_Tag = Illum_Dark;
    } else if (screen == ui_Light) {
        Illum_Tag = Illum_Light;
    } else if (screen == ui_Clk) {
        Illum_Tag = Illum_Dark;
    } else {
        Illum_Tag = Illum_Dark;
    }

    if (Illum_Cur < (Illum_Tag - ILLUM_RESPONSE)) {
        Illum_Cur += ILLUM_RESPONSE;
    } else if (Illum_Cur > (Illum_Tag + ILLUM_RESPONSE)) {
        Illum_Cur -= ILLUM_RESPONSE;
    } else {
        Illum_Cur = Illum_Tag;
    }
    analogWrite(ILLUM_PIN, Illum_Cur /10U);
}

/* ------------------------------------------------------------------ interface */
void myIllum_Flash(void)
{
    Illum_Cur = Illum_Light;
}
void myIllum_Blank(void)
{
    Illum_Cur = 0U;
}

/* **************************************************************** Sub Process */