/********************************************************************************/
/* myDispCtrl                                                                   */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: Ksamochi                                                          */
/* Depends on:                                                                  */
/*    lvgl v9.4.0 by ksvegabor                                                  */
/********************************************************************************/
#ifndef MYDISPCTRL_H
#define MYDISPCTRL_H
#ifdef __cplusplus
extern "C" {
#endif
/* ---------------------------------------------------------------- Definitions */

/* ------------------------------------------------------------------ Variables */

/* ------------------------------------------------------- Function Proto-Types */
void myDispCtrl_Init(void);
void myDispCtrl_Main(void);
void myDispCtrl_SwOpnImg(void);
void myDispCtrl_SwOdoTrip(void);

/* ---------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif
#endif