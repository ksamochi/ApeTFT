/********************************************************************************/
/* mySettingMgr                                                                 */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: Ksamochi                                                          */
/********************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>

#include "mySettingMgr.h"

/* ---------------------------------------------------------------- Definitions */
#define SD_CS_PIN (10)

/* ------------------------------------------------------------------ Variables */
static Preferences prefs;

/* ------------------------------------------------------- Function Proto-Types */

/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void mySettingMgr_Init(void)
{
    Serial.println("mySettingMgr_Init.");
    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Failed to open SD Card.");
        return;
    }
    File file = SD.open("/MetSettings.csv");
    if (!file) {
        Serial.println("Failed to open MetSettings.csv");
        return;
    }
    prefs.begin("MetSettings", false);

    while (file.available()) {
        String line = file.readStringUntil('\n');

        int c_pos;
        /* trim & form check */
        line.trim();
        line.replace("\r", "");
        int validlen = line.length();
        c_pos = line.indexOf('/');
        if (c_pos >= 0) validlen = min(validlen, c_pos);
        c_pos = line.indexOf('#');
        if (c_pos >= 0) validlen = min(validlen, c_pos);
        c_pos = line.indexOf('\'');
        if (c_pos >= 0) validlen = min(validlen, c_pos);
        line = line.substring(0, validlen);
        line.trim();
        if (line.length() == 0) continue;

        /* comma sepalate */
        c_pos = line.indexOf(',');
        if (c_pos <= 0) continue;

        String keyStr = line.substring(0, c_pos);
        keyStr.trim();
        const char* key = keyStr.c_str();

        String datStr = line.substring(c_pos + 1);
        c_pos = datStr.indexOf(',');
        if (c_pos >= 0) datStr = datStr.substring(0, c_pos);
        datStr.trim();
        const char* dat = datStr.c_str();

        /* store to Preferences */
        char *endptr;
        long value = strtol(dat, &endptr, 10);
        if (*endptr != '\0') {
            if (datStr.startsWith("\"") && datStr.endsWith("\"")) {
                datStr = datStr.substring(1, datStr.length() - 1);
            }
            prefs.putString(key, datStr);
            Serial.printf("Import Setting(str)... %s = %s\n", key, datStr.c_str());
        } else {
            prefs.putInt(key, (int)value);
            Serial.printf("Import Setting(int)... %s = %d\n", key, (int)value);
        }       
    }
    file.close();
    prefs.end();
}

/* **************************************************************** Sub Process */