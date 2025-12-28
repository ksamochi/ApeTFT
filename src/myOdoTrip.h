/********************************************************************************/
/* myOdoTrip.h                                                                       */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: -                                                                 */
/********************************************************************************/
#ifndef MYODOTRIP_H
#define MYODOTRIP_H
#ifdef __cplusplus
extern "C" {
#endif
/* ---------------------------------------------------------------- Definitions */

/* ------------------------------------------------------------------ Variables */

/* ------------------------------------------------------- Function Proto-Types */
void myOdoTrip_Init(void);
void myOdoTrip_Main(void);
uint32_t myOdoTrip_getOdo(void);
uint32_t myOdoTrip_getTrip(void);
void myOdoTrip_TripReset(void);

void myOdoTrip_CalibPlsReset(void);
uint32_t myOdoTrip_getCalibPls(void);
void myOdoTrip_CalibOdoPlsRate(void);
void myOdoTrip_CalibSpPlsRate(void);
/* ---------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif
#endif