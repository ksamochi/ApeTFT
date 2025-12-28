/********************************************************************************/
/* myLFW                                                                       */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: -                                                                 */
/********************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include "myOdoTrip.h"

#include "myLFW.h"

/* ---------------------------------------------------------------- Definitions */
#define LFW_THRESH_DEF (150U)

/* ------------------------------------------------------------------ Variables */
static Preferences prefs;
static uint32_t LFW_thresh;
static bool LFW_stat;

/* ------------------------------------------------------- Function Proto-Types */

/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void myLFW_Init(void)
{
    Serial.println("myLFW_Init.");

    prefs.begin("MetSettings", true);
    LFW_thresh = (uint32_t)prefs.getInt("lfw_thresh", LFW_THRESH_DEF);
    prefs.end();
}

/* ----------------------------------------------------------------------- main */
void myLFW_Main(void)
{
    uint32_t trip = myOdoTrip_getTrip();
    LFW_stat = (trip >= LFW_thresh);
}

/* ------------------------------------------------------------------ interface */
bool myLFW(void)
{
    return LFW_stat;
}

/* **************************************************************** Sub Process */