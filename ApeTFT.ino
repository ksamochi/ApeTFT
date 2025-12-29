/********************************************************************************/
/* ApeTFT.ino                                                                   */
/* Auth-date: 2025/12/27                                                        */
/*------------------------------------------------------------------------------*/
/* licence:   MIT                                                               */
/* Copyright: Ksamochi                                                          */
/* Depends on:                                                                  */
/*    board package esp32 by Espressif Systems                                  */
/*    lvgl v9.4.0 by ksvegabor                                                  */
/*    SquereLineStudio by SquareLine Kft.                                       */
/********************************************************************************/
#include <Arduino.h>
#include <Ticker.h>
#include <esp_timer.h>
#include <Wire.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_interface.h"
#include "src/esp_lcd_axs15231b/esp_lcd_axs15231b.h"

#include "src/lvgl/lvgl.h"
#include "src/ui/ui.h"
#include "src/myApplMain.h"
#include "src/mySettingMgr.h"
#include "src/myIllum.h"

/* ---------------------------------------------------------------- Definitions */
#define SCREEN_WIDTH  (480U)  /* LVGL Screen size */
#define SCREEN_HEIGHT (320U)  /* LVGL Screen size */
#define LV_BUF_LINE (40U)     /* LVGL rendering area size at once */

#define LCD_WIDTH   (320U)    /* Screen Buffer & LCD size */
#define LCD_HEIGHT  (480U)    /* Screen Buffer & LCD size */
#define DMA_BUF_LINE (80U)    /* DMA transportation chunk size */

#define LCD_CS      (45)
#define LCD_SCLK    (47)
#define LCD_SDIO0   (21)
#define LCD_SDIO1   (48)
#define LCD_SDIO2   (40)
#define LCD_SDIO3   (39)
#define LCD_RST     (-1)
#define LCD_BL      (1)

#define TOUCH_I2C_ADDR  (0x3B)
#define TOUCH_SCL   (8)
#define TOUCH_SDA   (4)
#define TOUCH_INT   (3)


/* ------------------------------------------------------------------ Variables */
static esp_lcd_panel_io_handle_t LcdIO;
static esp_lcd_panel_handle_t LcdPanel;
static SemaphoreHandle_t my_dma_smp;

/* LVGL */
Ticker Lv_Tick;
static lv_draw_buf_t Lv_DrawBufInfo1;
static lv_color_t Lv_DrawBuf1[SCREEN_WIDTH * LV_BUF_LINE];
//static lv_color_t Lv_DrawBuf2[SCREEN_WIDTH * LV_BUF_LINE];
static uint16_t* LcdBuf = nullptr;

lv_indev_t *gLv_Indev = nullptr;

static const axs15231b_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xBB, (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0xA5}, 8, 0},
    {0xA0, (uint8_t []){0xC0, 0x10, 0x00, 0x02, 0x00, 0x00, 0x04, 0x3F, 0x20, 0x05, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00}, 17, 0},
    {0xA2, (uint8_t []){0x30, 0x3C, 0x24, 0x14, 0xD0, 0x20, 0xFF, 0xE0, 0x40, 0x19, 0x80, 0x80, 0x80, 0x20, 0xf9, 0x10, 0x02, 0xff, 0xff, 0xF0, 0x90, 0x01, 0x32, 0xA0, 0x91, 0xE0, 0x20, 0x7F, 0xFF, 0x00, 0x5A}, 31, 0},
    {0xD0, (uint8_t []){0xE0, 0x40, 0x51, 0x24, 0x08, 0x05, 0x10, 0x01, 0x20, 0x15, 0x42, 0xC2, 0x22, 0x22, 0xAA, 0x03, 0x10, 0x12, 0x60, 0x14, 0x1E, 0x51, 0x15, 0x00, 0x8A, 0x20, 0x00, 0x03, 0x3A, 0x12}, 30, 0},
    {0xA3, (uint8_t []){0xA0, 0x06, 0xAa, 0x00, 0x08, 0x02, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x55, 0x55}, 22, 0},
    {0xC1, (uint8_t []){0x31, 0x04, 0x02, 0x02, 0x71, 0x05, 0x24, 0x55, 0x02, 0x00, 0x41, 0x00, 0x53, 0xFF, 0xFF, 0xFF, 0x4F, 0x52, 0x00, 0x4F, 0x52, 0x00, 0x45, 0x3B, 0x0B, 0x02, 0x0d, 0x00, 0xFF, 0x40}, 30, 0},
    {0xC3, (uint8_t []){0x00, 0x00, 0x00, 0x50, 0x03, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01}, 11, 0},
    {0xC4, (uint8_t []){0x00, 0x24, 0x33, 0x80, 0x00, 0xea, 0x64, 0x32, 0xC8, 0x64, 0xC8, 0x32, 0x90, 0x90, 0x11, 0x06, 0xDC, 0xFA, 0x00, 0x00, 0x80, 0xFE, 0x10, 0x10, 0x00, 0x0A, 0x0A, 0x44, 0x50}, 29, 0},
    {0xC5, (uint8_t []){0x18, 0x00, 0x00, 0x03, 0xFE, 0x3A, 0x4A, 0x20, 0x30, 0x10, 0x88, 0xDE, 0x0D, 0x08, 0x0F, 0x0F, 0x01, 0x3A, 0x4A, 0x20, 0x10, 0x10, 0x00}, 23, 0},
    {0xC6, (uint8_t []){0x05, 0x0A, 0x05, 0x0A, 0x00, 0xE0, 0x2E, 0x0B, 0x12, 0x22, 0x12, 0x22, 0x01, 0x03, 0x00, 0x3F, 0x6A, 0x18, 0xC8, 0x22}, 20, 0},
    {0xC7, (uint8_t []){0x50, 0x32, 0x28, 0x00, 0xa2, 0x80, 0x8f, 0x00, 0x80, 0xff, 0x07, 0x11, 0x9c, 0x67, 0xff, 0x24, 0x0c, 0x0d, 0x0e, 0x0f}, 20, 0},
    {0xC9, (uint8_t []){0x33, 0x44, 0x44, 0x01}, 4, 0},
    {0xCF, (uint8_t []){0x2C, 0x1E, 0x88, 0x58, 0x13, 0x18, 0x56, 0x18, 0x1E, 0x68, 0x88, 0x00, 0x65, 0x09, 0x22, 0xC4, 0x0C, 0x77, 0x22, 0x44, 0xAA, 0x55, 0x08, 0x08, 0x12, 0xA0, 0x08}, 27, 0},
    {0xD5, (uint8_t []){0x40, 0x8E, 0x8D, 0x01, 0x35, 0x04, 0x92, 0x74, 0x04, 0x92, 0x74, 0x04, 0x08, 0x6A, 0x04, 0x46, 0x03, 0x03, 0x03, 0x03, 0x82, 0x01, 0x03, 0x00, 0xE0, 0x51, 0xA1, 0x00, 0x00, 0x00}, 30, 0},
    {0xD6, (uint8_t []){0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE, 0x93, 0x00, 0x01, 0x83, 0x07, 0x07, 0x00, 0x07, 0x07, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x84, 0x00, 0x20, 0x01, 0x00}, 30, 0},
    {0xD7, (uint8_t []){0x03, 0x01, 0x0b, 0x09, 0x0f, 0x0d, 0x1E, 0x1F, 0x18, 0x1d, 0x1f, 0x19, 0x40, 0x8E, 0x04, 0x00, 0x20, 0xA0, 0x1F}, 19, 0},
    {0xD8, (uint8_t []){0x02, 0x00, 0x0a, 0x08, 0x0e, 0x0c, 0x1E, 0x1F, 0x18, 0x1d, 0x1f, 0x19}, 12, 0},
    {0xD9, (uint8_t []){0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, 12, 0},
    {0xDD, (uint8_t []){0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, 12, 0},
    {0xDF, (uint8_t []){0x44, 0x73, 0x4B, 0x69, 0x00, 0x0A, 0x02, 0x90}, 8,  0},
    {0xE0, (uint8_t []){0x3B, 0x28, 0x10, 0x16, 0x0c, 0x06, 0x11, 0x28, 0x5c, 0x21, 0x0D, 0x35, 0x13, 0x2C, 0x33, 0x28, 0x0D}, 17, 0},
    {0xE1, (uint8_t []){0x37, 0x28, 0x10, 0x16, 0x0b, 0x06, 0x11, 0x28, 0x5C, 0x21, 0x0D, 0x35, 0x14, 0x2C, 0x33, 0x28, 0x0F}, 17, 0},
    {0xE2, (uint8_t []){0x3B, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x35, 0x44, 0x32, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F, 0x0D}, 17, 0},
    {0xE3, (uint8_t []){0x37, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x35, 0x44, 0x32, 0x0C, 0x14, 0x14, 0x36, 0x32, 0x2F, 0x0F}, 17, 0},
    {0xE4, (uint8_t []){0x3B, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x39, 0x44, 0x2E, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F, 0x0D}, 17, 0},
    {0xE5, (uint8_t []){0x37, 0x07, 0x12, 0x18, 0x0E, 0x0D, 0x17, 0x39, 0x44, 0x2E, 0x0C, 0x14, 0x14, 0x36, 0x3A, 0x2F, 0x0F}, 17, 0},
    {0xA4, (uint8_t []){0x85, 0x85, 0x95, 0x82, 0xAF, 0xAA, 0xAA, 0x80, 0x10, 0x30, 0x40, 0x40, 0x20, 0xFF, 0x60, 0x30}, 16, 0},
    {0xA4, (uint8_t []){0x85, 0x85, 0x95, 0x85}, 4, 0},
    {0xBB, (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 8, 0},
    {0x13, (uint8_t []){0x00}, 0, 0},
    {0x3A, (uint8_t []){0x55}, 1, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x2C, (uint8_t []){0x00, 0x00, 0x00, 0x00}, 4, 0},
};
static const uint8_t touch_read_cmds[] = {
    0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00, 0x00, 0x08
};

/* ------------------------------------------------------- Function Proto-Types */
static void lvgl_task(void *arg);
static void IRAM_ATTR cyclic_ir(void);
static void cyclic_process(void);
static void my_disp_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static IRAM_ATTR bool my_dma_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
static void my_indev_cb(lv_indev_t * indev, lv_indev_data_t * data);

/* *************************************************************** Main Process */
/* ---------------------------------------------------------------------- setup */
void setup() {
  Serial.begin(115200);
  Serial.println("hello from ESP32");

  /* Import Settings from SD-Card */
  /* SPI bus settings conflict, so execute before display initialization. */
  mySettingMgr_Init();

  
  /* Initialize SPI bus */
  const spi_bus_config_t buscfg = AXS15231B_PANEL_BUS_QSPI_CONFIG(
    LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3, LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
  ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));


  /* Initialize LCD libraries */
  const esp_lcd_panel_io_spi_config_t io_config = AXS15231B_PANEL_IO_QSPI_CONFIG(
    LCD_CS, NULL, NULL);
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &LcdIO));

  const axs15231b_vendor_config_t vendor_config = {
    .init_cmds = lcd_init_cmds,
    .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
    .flags = {
        .use_qspi_interface = 1,
    },
  };
  const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = LCD_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
    .vendor_config = (void *)&vendor_config,
  };
  esp_lcd_new_panel_axs15231b(LcdIO, &panel_config, &LcdPanel);
  esp_lcd_panel_reset(LcdPanel);
  esp_lcd_panel_init(LcdPanel);
  esp_lcd_panel_disp_on_off(LcdPanel, false );

  LcdBuf = (uint16_t*)ps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
  memset(LcdBuf, 0x00, LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));

  my_dma_smp = xSemaphoreCreateBinary();
  xSemaphoreTake(my_dma_smp, 0);
  const esp_lcd_panel_io_callbacks_t cbs = {
    .on_color_trans_done = my_dma_cb,
  };
  esp_lcd_panel_io_register_event_callbacks(LcdIO, &cbs, NULL);


  /* Initialize Touch Panel */
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(400000);  // 400kHz I2C speed


  /* Initialize LVGL */
  lv_init();

  size_t buf_bytes = SCREEN_WIDTH * LV_BUF_LINE * sizeof(lv_color_t);
  lv_draw_buf_init(&Lv_DrawBufInfo1, SCREEN_WIDTH, LV_BUF_LINE, LV_COLOR_FORMAT_RGB565, SCREEN_WIDTH, Lv_DrawBuf1, buf_bytes);

  lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_draw_buffers(disp, &Lv_DrawBufInfo1, nullptr);
  lv_display_set_flush_cb(disp, my_disp_cb);

  gLv_Indev = lv_indev_create();
  lv_indev_set_type(gLv_Indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(gLv_Indev, my_indev_cb);


  /* Initialize SquereLineStudio UI */
  ui_init();


  /* Initialize User Application */
  myApplMain_Init();


  /* launch lvgl ticker & Task */
  Lv_Tick.attach_ms(1, []() { lv_tick_inc(1); });
  (void)lv_timer_create(cyclic_process, 10, NULL);
  xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 2, NULL, 1);


  Serial.println("Initialised. GO!!");
}

/* ----------------------------------------------------------------------- loop */
void loop() 
{
  static uint32_t last_tick = 0;
  uint32_t now = lv_tick_get();
  uint32_t fint = now - last_tick;
  if (fint >= 100) {
    int64_t t0 = esp_timer_get_time();  // us
    for (uint32_t ofs = 0; ofs < LCD_HEIGHT; ofs += DMA_BUF_LINE) {
      uint32_t h = DMA_BUF_LINE;
      if ((ofs + h) > LCD_HEIGHT) h = LCD_HEIGHT - ofs;
      uint16_t* buf = LcdBuf + (ofs * LCD_WIDTH);
      esp_lcd_panel_draw_bitmap(LcdPanel, 0, ofs, LCD_WIDTH, ofs + h, buf);
      xSemaphoreTake(my_dma_smp, portMAX_DELAY);
    }
    int64_t t1 = esp_timer_get_time();
    int64_t dt = t1 - t0;
    printf("flush time: %06.2f[ms] @ %04.1f[fps]\n", dt / 1000.0, 1000.0 / fint);
    last_tick = now;
  }

  myApplMain_BG();
}

/* **************************************************************** Sub Process */
/* ------------------------------------------------------------------ lvgl_task */
static void lvgl_task(void *arg) {
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ------------------------------------------------------------- cyclic_process */
static void cyclic_process(lv_timer_t * timer)
{
  myApplMain_10ms();
}

/* ----------------------------------------------------------------- my_disp_cb */
/* This function is invoked by LVGL after each component (object) has finished rendering. */
/* In this project/branch, it rotates the image by 90 degrees and expands the pixel data */
/* into the full-screen buffer. */
static void my_disp_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint16_t *src = (uint16_t *)px_map;
  for (uint16_t dstx = LCD_WIDTH - area->y1; LCD_WIDTH - area->y2 <= dstx; dstx--) {
    uint32_t dst = area->x1 * LCD_WIDTH + dstx - 1;
    for (uint16_t dsty = area->x1; dsty <= area->x2; dsty++) {
      uint16_t c = *src++;
      LcdBuf[dst] = (c >> 8) | (c << 8);
      dst += LCD_WIDTH;
    }
  }
  lv_display_flush_ready(disp);
}
/* ------------------------------------------------------------------ my_dma_cb */
/* This function is called by the system when a DMA transfer completes. */
/* In this project, it gives a semaphore to allow the next DMA transfer to begin. */
static IRAM_ATTR bool my_dma_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(my_dma_smp, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  return false;
}

/* ---------------------------------------------------------------- my_indev_cb */
/* This function is periodically called by LVGL to provide touch input data.    */
static void my_indev_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    data->state = LV_INDEV_STATE_RELEASED;

    static const uint8_t touch_read_cmds[] = {
        0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00, 0x00, 0x08};
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(touch_read_cmds, sizeof(touch_read_cmds));
    if (Wire.endTransmission() != 0) return;
    delayMicroseconds(10);
 
    Wire.requestFrom(TOUCH_I2C_ADDR, (uint8_t)8);
    /* rxd format : 00, nz, xx, xx, yy, yy, ##, ## */
    uint8_t rxd[8] = {0};
    int i = 0;
    while (Wire.available() && i < 8) rxd[i++] = Wire.read();
    if (i < 8) return;
    if (rxd[0] != 0) return;
    if (rxd[1] == 0) return;

    uint16_t raw_x = ((rxd[2] & 0x0F) << 8) | rxd[3];
    uint16_t raw_y = ((rxd[4] & 0x0F) << 8) | rxd[5];

    data->point.x = std::min<uint16_t>((int16_t)(LCD_HEIGHT -1), (int16_t)(raw_y));
    data->point.y = std::min<uint16_t>((int16_t)(LCD_WIDTH -1), (int16_t)(LCD_WIDTH - raw_x));
    data->state = LV_INDEV_STATE_PRESSED;

    Serial.printf("touch: %d, %d\n", data->point.x, data->point.y);
    myIllum_Flash();

}
