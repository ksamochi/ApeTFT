/********************************************************************************/
/* myApplMain                                                                   */
/* Auth-date: 2025/11/30                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: Ksamochi                                                          */
/* Depends on:-                                                                 */
/********************************************************************************/
#include <Arduino.h>
#include "lvgl/lvgl.h"

#include "mySettingMgr.h"
#include "mySpeed.h"
#include "myTacho.h"
#include "myOdoTrip.h"
#include "myLFW.h"
#include "myNeut.h"
#include "myClock.h"
#include "myIllum.h"
#include "myDispCtrl.h"

#include "myApplMain.h"

/* ---------------------------------------------------------------- Definitions */


/* ------------------------------------------------------------------ Variables */
static uint32_t applMain_10msCnt;

/* ------------------------------------------------------- Function Proto-Types */


/* *************************************************************** Main Process */
/* -------------------------------------------------------------- ApplMain_Init */
void myApplMain_Init(void)
{
    Serial.println("myApplMain_Init.");

//    mySettingMgr_Init(); /* SPI bus settings conflict, so execute before display initialization.

    mySpeed_Init();
    myTacho_Init();
    myOdoTrip_Init();
    myLFW_Init();
    myNeut_Init();
    myClock_Init();
    myIllum_Init();

    myDispCtrl_Init();
    applMain_10msCnt = 0;
    Serial.println("myApplMain_Init_Done.");
}


/* -------------------------------------------------------------- ApplMain_10ms */
void myApplMain_10ms(void)
{
    if (applMain_10msCnt < 0xFFFFFFFFU) applMain_10msCnt++;

    /* 10ms Cyclic Processes */
    mySpeed_Main();
    myTacho_Main();
    myOdoTrip_Main();
    myLFW_Main();
    myNeut_Main();
    myClock_Main();
    myIllum_Main();

    myDispCtrl_Main();
}

/* ---------------------------------------------------------------- ApplMain_BG */
void myApplMain_BG(void)
{
    /* Back Ground Processes */

}

/* ---------------------------------------------------------------- ApplMain_BG */
uint32_t myApplMain_get10msCnt(void)
{
    return applMain_10msCnt;
}

/* **************************************************************** Sub Process */