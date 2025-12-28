SET(SOURCES components/ui_comp_tachobar.c
    components/ui_comp.c
    screens/ui_Opn.c
    screens/ui_Dark.c
    screens/ui_Light.c
    screens/ui_Clk.c
    screens/ui_PlsSet.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.cpp
    images/ui_img_imgopape_png.c
    images/ui_img_imgophnd_png.c
    images/ui_img_imgopkws_png.c
    images/ui_img_imgopszk_png.c
    images/ui_img_imgopymh_png.c
    images/ui_img_imgbg_dark_png.c
    images/ui_img_imglfw_png.c
    images/ui_img_imgdaylight_png.c
    images/ui_img_imgneut_png.c
    images/ui_img_imgtagauge_png.c
    images/ui_img_imgbg_light_png.c
    fonts/ui_font_Font30.c
    fonts/ui_font_N200.c
    fonts/ui_font_NUM120.c
    fonts/ui_font_Num60.c
    fonts/ui_font_Sp100.c
    fonts/ui_font_Sp200.c)

add_library(ui ${SOURCES})
