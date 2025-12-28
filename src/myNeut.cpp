/********************************************************************************/
/* myNeut                                                                       */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: -                                                                 */
/********************************************************************************/
#include <Arduino.h>

#include "myNeut.h"

/* ---------------------------------------------------------------- Definitions */
#define NEUT_PIN (17)
#define NEUT_ACT (HIGH)
#define NEUT_FILT (4)

/* ------------------------------------------------------------------ Variables */
static bool Neut_stat;
static uint32_t cnt;

/* ------------------------------------------------------- Function Proto-Types */

/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void myNeut_Init(void)
{
    Serial.println("myNeut_Init.");
    pinMode(NEUT_PIN, INPUT);
    
    Neut_stat = true;
    cnt = 0;
}

/* ----------------------------------------------------------------------- main */
void myNeut_Main(void)
{
    int port_in = digitalRead(NEUT_PIN);
    if ((Neut_stat) && (port_in != NEUT_ACT)) {
        cnt++;
    } else if ((!Neut_stat) && (port_in == NEUT_ACT)) {
        cnt++;
    } else {
        cnt = 0;
    }
    if (cnt >= NEUT_FILT) {
        Neut_stat = (port_in == NEUT_ACT);
        cnt = 0;
    }
}

/* ------------------------------------------------------------------ interface */
bool myNeut(void)
{
    return Neut_stat;
}

/* **************************************************************** Sub Process */