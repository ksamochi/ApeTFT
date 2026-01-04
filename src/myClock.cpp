/********************************************************************************/
/* myClock                                                                      */
/* Auth-date: 2025/12/28                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: -                                                                 */
/********************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>

#include "myClock.h"

/* ---------------------------------------------------------------- Definitions */
const String ssid_def = "xxxx's iPhone";
const String pass_def = "xxxx-xxxx-xxxx-xxxx";

/* ------------------------------------------------------------------ Variables */
static Preferences prefs;
static String SSIDhome;
static String PASShome;
static String SSIDsmp;
static String PASSsmp;
static uint32_t clkSyncPhase;
static uint32_t clkSyncTimeout;
static tm ClockInfo;

static const char* ntpServer = "133.243.238.243";           // ntp.nict.jp
static const long  gmtOffset_sec = 9 * 3600;                // JST = UTC+9
static const int   daylightOffset_sec = 0;


/* ------------------------------------------------------- Function Proto-Types */

/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void myClock_Init(void)
{
    Serial.println("myClock_Init.");

    prefs.begin("MetSettings", true);
    SSIDhome = prefs.getString("wifissidhome", ssid_def);
    PASShome = prefs.getString("wifipasshome", pass_def);
    SSIDsmp = prefs.getString("wifissidsmp", ssid_def);
    PASSsmp = prefs.getString("wifipasssmp", pass_def);
    prefs.end();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(SSIDhome.c_str(), PASShome.c_str());

    ClockInfo.tm_hour = -1;
    ClockInfo.tm_min  = -1;
    ClockInfo.tm_sec  = -1;
    clkSyncPhase = 1;
    clkSyncTimeout = millis() + 3000;

    Serial.println("myClock_Init_Done.");
}

/* ----------------------------------------------------------------------- main */
void myClock_Main(void)
{
    switch ( clkSyncPhase ) {
    case 0: // Initial State
        WiFi.begin(SSIDsmp.c_str(), PASSsmp.c_str());
        clkSyncTimeout = millis() + 5000;
        clkSyncPhase++;
        break;
    case 1: // Start WiFi Connection
        if ((int32_t)(millis() - clkSyncTimeout) >= 0) {
            Serial.println("WiFi Connect Timeout.");
            WiFi.disconnect(false);
            clkSyncPhase = 0;
            break;
        }
        if (WiFi.status() == WL_CONNECTED){
            Serial.println("Wifi Connected.");
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
            clkSyncPhase++;
        }
        break;
    case 2: // Wait for Clock Sync
        if ((int32_t)(millis() - clkSyncTimeout) >= 0) {
            Serial.println("Clock Sync Timeout.");
            WiFi.disconnect(false);
            clkSyncPhase = 0;
            break;
        }
        if (WiFi.status() != WL_CONNECTED) break;
        
        if (getLocalTime(&ClockInfo)) {
            Serial.println("Clock Synchronized.");
            WiFi.disconnect(false);
            clkSyncPhase++;
        }
        break;
    default:
        getLocalTime(&ClockInfo);
        break;
    }
}

/* ------------------------------------------------------------------ interface */
int myClock_GetHour(void){
    return ClockInfo.tm_hour;
}
int myClock_GetMin(void){
    return ClockInfo.tm_min;
}
int myClock_GetSec(void){
    return ClockInfo.tm_sec;
}

/* **************************************************************** Sub Process */