/********************************************************************************/
/* ApeTFT.ino                                                                   */
/* Auth-date: 2025/11/30                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: Ksamochi                                                          */
/* Depends on:                                                                  */
/*    board package esp32 by Espressif Systems                                  */
/*    JC3248W535-Driver v1.0.0 by Processware                                   */
/*    GFX Library for Arduino v1.6.3 by moononournation                         */
/*    lvgl v9.4.0 by ksvegabor                                                  */
/********************************************************************************/
//#define LV_CONF_INCLUDE_SIMPLE
#include "src/lvgl/lvgl.h"
#include "src/GFX Library for Arduino/src/Arduino_GFX_Library.h"
#include "src/JC3248W535-Driver/src/JC3248W535.h"
#include <Ticker.h>
#include "src/ui/ui.h"
#include "src/myApplMain.h"
#include "src/mySettingMgr.h"
#include "src/myIllum.h"

/* ---------------------------------------------------------------- Definitions */
#define SCREEN_WIDTH JC3248_LCD_HEIGHT 
#define SCREEN_HEIGHT JC3248_LCD_WIDTH  
#define LV_BUF_LINE (40U)

/* ------------------------------------------------------------------ Variables */
/* JC3248W535-Driver wrapper class */
static Arduino_Canvas *DispCanvas;
JC3248W535_Display Display;
JC3248W535_Touch Touch;

/* LVGL */
Ticker Lv_Tick;
static lv_draw_buf_t Lv_DrawBufInfo1;
//static lv_draw_buf_t Lv_DrawBufInfo2;
static lv_color_t Lv_DrawBuf1[SCREEN_WIDTH * LV_BUF_LINE];
//static lv_color_t Lv_DrawBuf2[SCREEN_WIDTH * LV_BUF_LINE];
static lv_indev_t *Lv_Indev;

/* ------------------------------------------------------- Function Proto-Types */
static void lvgl_task(void *arg);
static void IRAM_ATTR cyclic_ir(void);
static void cyclic_process(void);
static void my_disp_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void my_indev_cb(lv_indev_t * indev, lv_indev_data_t * data);

/* *************************************************************** Main Process */
/* ---------------------------------------------------------------------- setup */
void setup() {
  Serial.begin(115200);
  Serial.println("hello from ESP32");

  /* Import Settings from SD-Card */
  /* SPI bus settings conflict, so execute before display initialization. */
  mySettingMgr_Init();

  /* Initialize Display&Touch */
  Display.begin();
  DispCanvas = Display.getCanvas();
  DispCanvas->setRotation(ROTATION_90);            /* ROTATION_90 =Landscape(480x320) */
  Touch.begin();
  Touch.setRotation(ROTATION_90, SCREEN_WIDTH, SCREEN_HEIGHT);

  /* Initialize LVGL */
  lv_init();

  size_t buf_bytes = SCREEN_WIDTH * LV_BUF_LINE * sizeof(lv_color_t);
  lv_draw_buf_init(&Lv_DrawBufInfo1, SCREEN_WIDTH, LV_BUF_LINE, LV_COLOR_FORMAT_RGB565, SCREEN_WIDTH, Lv_DrawBuf1, buf_bytes);
  //lv_draw_buf_init(&Lv_DrawBufInfo2, SCREEN_WIDTH, LV_BUF_LINE, LV_COLOR_FORMAT_RGB565, SCREEN_WIDTH, Lv_DrawBuf2, buf_bytes);

  lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_draw_buffers(disp, &Lv_DrawBufInfo1, nullptr);
  //lv_display_set_draw_buffers(disp, &Lv_DrawBufInfo1, &Lv_DrawBufInfo2);
  lv_display_set_flush_cb(disp, my_disp_cb);

  Lv_Indev = lv_indev_create();
  lv_indev_set_type(Lv_Indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(Lv_Indev, my_indev_cb);


  /* Initialize SuereLine UI */
  ui_init();

  myApplMain_Init();

  /* launch lvgl ticker */
  Lv_Tick.attach_ms(1, []() { lv_tick_inc(1); });
  (void)lv_timer_create(cyclic_process, 10, NULL);
  // xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 2, NULL, 1);

  Serial.println("Initialised. GO!!");
}

/* ----------------------------------------------------------------------- loop */
void loop() 
{
  lv_timer_handler_run_in_period(10);
  myApplMain_BG();

  static uint32_t last_tick = 0;
  uint32_t now = lv_tick_get();
  uint32_t fint = now - last_tick;
  if (fint >= 200) {
    int64_t t0 = esp_timer_get_time();
    Display.flush();
    int64_t t1 = esp_timer_get_time();
    int64_t dt = t1 - t0;
    printf("flush time: %06.2f[ms] @ %04.1f[fps]\n", dt / 1000.0, 1000.0 / fint);
    last_tick = now;
  }
  vTaskDelay(1);
}

/* **************************************************************** Sub Process */
/* ------------------------------------------------------------------ lvgl_task */
static void lvgl_task(void *arg) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        lv_timer_handler();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

/* ------------------------------------------------------------- cyclic_process */
static void cyclic_process(lv_timer_t * timer)
{
  myApplMain_10ms();
}

/* ----------------------------------------------------------------- my_disp_cb */
static void my_disp_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;
  if (DispCanvas) {
    DispCanvas->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
    lv_display_flush_ready(disp);
  }
}

/* ---------------------------------------------------------------- my_indev_cb */
static void my_indev_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    TouchPoint p;
    bool ok = Touch.read(p);

    if (ok && p.touched) {
        data->state   = LV_INDEV_STATE_PRESSED;
        data->point.x = p.x;
        data->point.y = p.y;
        Serial.printf("touch: %d, %d\n", p.x, p.y);
        myIllum_Flash();
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
