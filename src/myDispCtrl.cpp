/********************************************************************************/
/* myDispCtrl                                                                   */
/* Auth-date: 2025/12/07                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: Ksamochi                                                          */
/* Depends on:                                                                  */
/*    lvgl v9.4.0 by ksvegabor                                                  */
/********************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include "ui/ui.h"

#include "myTacho.h"
#include "mySpeed.h"
#include "myNeut.h"
#include "myOdoTrip.h"
#include "myLFW.h"
#include "myClock.h"
#include "myIllum.h"

#include "myDispCtrl.h"

/* ---------------------------------------------------------------- Definitions */

/* ------------------------------------------------------------------ Variables */
static Preferences prefs;
static bool demo_mode;

static uint32_t OpnImg;
static lv_obj_t* DispSel;
static uint32_t DispCtrl_Cnt;
static uint32_t DispCycle_Cnt;
static uint32_t DemoCycle_Cnt;
static uint32_t OdoTripSel;

static int32_t spMark;
static int32_t spDisp;
static int32_t taMark;
static int32_t taDisp;
static int32_t odDisp;

/* ------------------------------------------------------- Function Proto-Types */
static void disp_updOpnImg(void);
static void disp_demo(void);
static void disp_TaArc(void);
static void disp_TaDig(void);
static void disp_SpDig(void);
static void disp_Neut(void);
static void disp_OdoTrip(void);
static void disp_LFW(void);
static void disp_Clk(void);

/* *************************************************** Main Process & Interface */
/* ----------------------------------------------------------------------- init */
void myDispCtrl_Init(void)
{
    Serial.println("myDispCtrl_Init.");

    prefs.begin("MetSettings", true);
    OpnImg = prefs.getInt("Disp_OpnImg", 0U);
    prefs.end();
    disp_updOpnImg();

    DispSel = ui_Dark;
    DispCtrl_Cnt = 0U;
    DispCycle_Cnt = 0U;
    demo_mode = true;
}

/* ----------------------------------------------------------------------- main */
void myDispCtrl_Main(void)
{
    if (DispCtrl_Cnt < 0xFFFFFFFF) DispCtrl_Cnt++;
    if (DispCtrl_Cnt == 300U) {
        if (DispSel == ui_Light) {
            _ui_screen_change(&ui_Light, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Light_screen_init);
        }  else {
            _ui_screen_change(&ui_Dark, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Dark_screen_init);
        }

        if (demo_mode) {
            Serial.println("Start Demo.");
        } else {
            Serial.println("Start Normal.");
        }
    }

    DispCycle_Cnt = (DispCycle_Cnt + 1) % 100;

    if (myTacho_EngOn()) demo_mode = false;
    if (demo_mode) {
        disp_demo();
    } else {
        if (DispCycle_Cnt %10 == 0) disp_TaArc();
        if (DispCycle_Cnt %10 == 5) disp_Neut();
        switch(DispCycle_Cnt){
        case 0:
        case 50:
            disp_TaDig();
            break;
        case 10:
        case 60:
            disp_SpDig();
        case 20:
        case 70:
            disp_OdoTrip();
            break;
        case 30:
        case 80:
            disp_LFW();
            break;
        case 40:
        case 90:
            disp_Clk();
            break;
        default:
            break;
        }
    }
}

/* ------------------------------------------------------------------ interface */
void myDispCtrl_SwOdoTrip(void)
{
    OdoTripSel = (OdoTripSel +1) %2;
}

void myDispCtrl_SwOpnImg(void)
{
    OpnImg = (OpnImg + 1) % 5;
    prefs.begin("MetSettings", false);
    prefs.putInt("Disp_OpnImg", OpnImg);
    prefs.end();
    disp_updOpnImg();
}

/* **************************************************************** Sub Process */
static void disp_updOpnImg(void)
{
    static const void* op_img_table[5] = {
        &ui_img_imgopape_png,
        &ui_img_imgophnd_png,
        &ui_img_imgopkws_png,
        &ui_img_imgopszk_png,
        &ui_img_imgopymh_png,
    };
    OpnImg = OpnImg %5;
    lv_obj_set_style_bg_image_src(ui_Opn, op_img_table[OpnImg], LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void disp_demo()
{
    lv_obj_t* screen = lv_scr_act();
    if (screen == ui_Dark){
        DispSel = ui_Dark;
    } else if (screen == ui_Light){
        DispSel = ui_Light;
    } else if (screen == ui_Clk){
        DemoCycle_Cnt = 0;
    }

    DemoCycle_Cnt++;
    if (DemoCycle_Cnt < 500){
        spMark = 0;
        taMark = 1200;
    } else if (DemoCycle_Cnt < 700){
        spMark = 300;
        taMark = 3000;
    } else if (DemoCycle_Cnt < 900){
        spMark = 600;
        taMark = 6000;
    } else if (DemoCycle_Cnt < 1100){
        spMark = 900;
        taMark = 9000;
    } else if (DemoCycle_Cnt < 1300){
        spMark = 1100;
        taMark = 11000;
    } else if (DemoCycle_Cnt < 1500){
        spMark = 1500;
        taMark = 15000;
    } else {
        DemoCycle_Cnt = 0;
        odDisp++;
        if (odDisp %4 == 0){
            DispCtrl_Cnt = 0U;
            _ui_screen_change(&ui_Opn, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Opn_screen_init);
            myIllum_Blank();
        }
    }
    
    int32_t Response = 24;
    spDisp = spDisp + (spMark - spDisp) / Response + ((DemoCycle_Cnt %3) -1) *20;
    taDisp = taDisp + (taMark - taDisp) / Response + ((DemoCycle_Cnt %3) -1) *200;


    if (DispCycle_Cnt %10 == 0) {
        lv_arc_set_value(ui_Dark_TachoBarDark, (taDisp + 125) / 250);
        lv_arc_set_value(ui_Light_TachoBarLight, (taDisp + 125 ) / 250);
    }
    if (DispCycle_Cnt %10 == 5) {
        if (spDisp < 100){
            lv_obj_remove_flag(ui_Dark_NeutDark, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_Light_NeutLight, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_Dark_NeutDark, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Light_NeutLight, LV_OBJ_FLAG_HIDDEN);
        }
    }

    char buf[8];
    switch(DispCycle_Cnt){
    case 0:
    case 50:
        snprintf(buf, sizeof(buf), "%04d", (taDisp) %10000);
        lv_textarea_set_text(ui_Dark_TaDgDark, buf);
        lv_textarea_set_text(ui_Light_TaDgLight, buf);
        break;
    case 10:
    case 60:
        if (spDisp < 30){
            lv_textarea_set_text(ui_Dark_SpDgDark100, " ");
            lv_textarea_set_text(ui_Light_SpDgLight100, " ");
            snprintf(buf, sizeof(buf), "%2d", 0);
        } else if (spDisp < 1000){
            lv_textarea_set_text(ui_Dark_SpDgDark100, " ");
            lv_textarea_set_text(ui_Light_SpDgLight100, " ");
            snprintf(buf, sizeof(buf), "%2d", spDisp /10);
        } else {
            lv_textarea_set_text(ui_Dark_SpDgDark100, "1");
            lv_textarea_set_text(ui_Light_SpDgLight100, "1");
            snprintf(buf, sizeof(buf), "%02d", (spDisp /10) %100);
        }
        lv_textarea_set_text(ui_Dark_SpDgDark, buf);
        lv_textarea_set_text(ui_Light_SpDgLight, buf);
        break;
    case 20:
    case 70:
        switch (OdoTripSel) {
        case 0: /* tripmeter */
            lv_obj_add_flag(ui_Dark_OdoLblDark, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_Dark_TripLblDark, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Light_OdoLblLight, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_Light_TripLblLight, LV_OBJ_FLAG_HIDDEN);
            break;
        case 1: /* odometer */
            lv_obj_remove_flag(ui_Dark_OdoLblDark, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Dark_TripLblDark, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_Light_OdoLblLight, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Light_TripLblLight, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            lv_obj_add_flag(ui_Dark_OdoLblDark, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Dark_TripLblDark, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Light_OdoLblLight, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Light_TripLblLight, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        snprintf(buf, sizeof(buf), "%05d", odDisp);
        lv_textarea_set_text(ui_Dark_OdoTripDark, buf);
        lv_textarea_set_text(ui_Light_OdoTripLight, buf);
        break;
    case 30:
    case 80:
        //disp_LFW();
        break;
    case 40:
    case 90:
        disp_Clk();
        break;
    default:
        break;
    }
}

static void disp_TaArc()
{
    taDisp = myTacho_Disp();
    lv_arc_set_value(ui_Dark_TachoBarDark, (taDisp + 125) / 250);
    lv_arc_set_value(ui_Light_TachoBarLight, (taDisp + 125) / 250);
}
static void disp_TaDig()
{
    taDisp = myTacho_Disp();
    char buf[8];
    snprintf(buf, sizeof(buf), "%04d", (taDisp) %10000);
    lv_textarea_set_text(ui_Dark_TaDgDark, buf);
    lv_textarea_set_text(ui_Light_TaDgLight, buf);
}
static void disp_SpDig()
{
    spDisp = mySpeed_Disp();
    char buf[8];
    if (spDisp < 30){
        lv_textarea_set_text(ui_Dark_SpDgDark100, " ");
        lv_textarea_set_text(ui_Light_SpDgLight100, " ");
        snprintf(buf, sizeof(buf), "%2d", 0);
    } else if (spDisp < 1000){
        lv_textarea_set_text(ui_Dark_SpDgDark100, " ");
        lv_textarea_set_text(ui_Light_SpDgLight100, " ");
        snprintf(buf, sizeof(buf), "%2d", spDisp /10);
    } else {
        lv_textarea_set_text(ui_Dark_SpDgDark100, "1");
        lv_textarea_set_text(ui_Light_SpDgLight100, "1");
        snprintf(buf, sizeof(buf), "%02d", (spDisp /10) %100);
    }
    lv_textarea_set_text(ui_Dark_SpDgDark, buf);
    lv_textarea_set_text(ui_Light_SpDgLight, buf);
}
static void disp_Neut()
{
    if (myNeut()){
        lv_obj_remove_flag(ui_Dark_NeutDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_Light_NeutLight, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui_Dark_NeutDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Light_NeutLight, LV_OBJ_FLAG_HIDDEN);
    }
}
static void disp_OdoTrip()
{
    char buf[8];
    switch (OdoTripSel)
    {
    case 0: /* tripmeter */
        lv_obj_add_flag(ui_Dark_OdoLblDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_Dark_TripLblDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Light_OdoLblLight, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_Light_TripLblLight, LV_OBJ_FLAG_HIDDEN);
        snprintf(buf, sizeof(buf), "%05d", myOdoTrip_getOdo());
        break;
    case 1: /* odometer */
        lv_obj_remove_flag(ui_Dark_OdoLblDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Dark_TripLblDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_Light_OdoLblLight, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Light_TripLblLight, LV_OBJ_FLAG_HIDDEN);
        snprintf(buf, sizeof(buf), "%05d", myOdoTrip_getTrip());
        break;
    default:
        lv_obj_add_flag(ui_Dark_OdoLblDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Dark_TripLblDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Light_OdoLblLight, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Light_TripLblLight, LV_OBJ_FLAG_HIDDEN);
        snprintf(buf, sizeof(buf), "-----");
        break;
    }
    lv_textarea_set_text(ui_Dark_OdoTripDark, buf);
    lv_textarea_set_text(ui_Light_OdoTripLight, buf);
}
static void disp_LFW()
{
    if (myLFW()){
        lv_obj_remove_flag(ui_Dark_LfwDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_Light_LfwLight, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui_Dark_LfwDark, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_Light_LfwLight, LV_OBJ_FLAG_HIDDEN);
    }
}
static void disp_Clk()
{
    int hour = myClock_GetHour();
    int min = myClock_GetMin();
    int sec = myClock_GetSec();
    if (hour == -1){
        lv_textarea_set_text(ui_Dark_ClockDark, "--:--");
        lv_textarea_set_text(ui_Light_ClockLight, "--:--");
    } else {
        char buf[8];
        snprintf(buf, sizeof(buf), "%2d:%02d", (uint32_t)hour, (uint32_t)min);
        lv_textarea_set_text(ui_Dark_ClockDark, buf);
        lv_textarea_set_text(ui_Light_ClockLight, buf);
        lv_roller_set_selected(ui_Clk_Hour, (uint32_t)hour, LV_ANIM_ON);
        lv_roller_set_selected(ui_Clk_Min, (uint32_t)min, LV_ANIM_ON);
    }

    lv_obj_t* screen = lv_scr_act();
    if ((screen == ui_Clk) && (sec == 0)){
        myIllum_Flash();
    }
}
