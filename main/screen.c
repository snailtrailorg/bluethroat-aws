#include "nvs_flash.h"
#include "esp_log.h"
#include "screen.h"
#include "config.h"
#include "freertos/timers.h"

#define UI_COLOR_BACKGROUND             LV_COLOR_BLACK
#define UI_COLOR_BORDER                 LV_COLOR_GRAY
#define UI_COLOR_SCALE_GLIDING          LV_COLOR_ORANGE
#define UI_COLOR_SCALE_LIFT             LV_COLOR_GREEN
#define UI_COLOR_SCALE_SINK             LV_COLOR_RED
#define UI_COLOR_LABEL                  LV_COLOR_GRAY
#define UI_COLOR_TEXT                   LV_COLOR_WHITE

#define LV_SYMBOL_LOCK                  "\xef\x80\xA3" //61475 f023
#define LV_SYMBOL_UNLOCK                "\xef\x82\x9c" //61596 f09c
LV_FONT_DECLARE(awesome_14);

LV_IMG_DECLARE(paragliding_logo);
LV_IMG_DECLARE(compass);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_12);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_14);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_16);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_18);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_20);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_24);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_32);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_48);
LV_FONT_DECLARE(lv_font_arial_rounded_mt_72);

static SemaphoreHandle_t ui_data_mutex = NULL;
static double ui_altitude = 0;
static double ui_speed = 0;
static double ui_pressure = 101325.0;
static double ui_temperature = 25.88;
static double ui_humidity = 99.99;

static SemaphoreHandle_t ui_mutex = NULL;

static lv_obj_t * current_screen = NULL;
static lv_obj_t * main_screen = NULL;
static lv_obj_t * setting_screen = NULL;

static lv_obj_t * scale_left_lift[8];
static lv_obj_t * scale_left_sink[8];
static lv_obj_t * scale_right_lift[8];
static lv_obj_t * scale_right_sink[8];

static lv_obj_t * clock_text = NULL;
static lv_obj_t * lock_icon = NULL;
static lv_obj_t * gps_icon = NULL;
static lv_obj_t * sd_card_icon = NULL;
static lv_obj_t * bluetooth_icon = NULL;
static lv_obj_t * volume_icon = NULL;
static lv_obj_t * charge_icon = NULL;
static lv_obj_t * battery_icon = NULL;

static lv_obj_t * main_screen_tab_view = NULL;
static lv_obj_t * dashboard_tab = NULL;
static lv_obj_t * motion_tab = NULL;
static lv_obj_t * compass_tab = NULL;

static lv_obj_t * altitude_text = NULL;
static lv_obj_t * temperature_text = NULL;
static lv_obj_t * pressure_text = NULL;
static lv_obj_t * humidity_text = NULL;
static lv_obj_t * speed_text = NULL;

static lv_obj_t * motion_gauge = NULL;
static lv_obj_t * compass_image = NULL;

static lv_obj_t * setting_screen_tab_view = NULL;
static lv_obj_t * time_tab = NULL;
static lv_obj_t * volume_tab = NULL;
static lv_obj_t * brightness_tab = NULL;
static lv_obj_t * temperature_tab = NULL;
static lv_obj_t * bluetooth_tab = NULL;
static lv_obj_t * reset_tab = NULL;

static lv_obj_t * hour_roller = NULL;
static lv_obj_t * minute_roller = NULL;
static lv_obj_t * temprature_spinbox = NULL;
static lv_obj_t * volume_slider = NULL;
static lv_obj_t * brightness_slider = NULL;
static lv_obj_t * bluetooth_switch = NULL;

void lv_obj_debug(const char * name, lv_obj_t * object) {
    lv_coord_t x = lv_obj_get_x(object);
    lv_coord_t y = lv_obj_get_y(object);
    lv_coord_t w = lv_obj_get_width_margin(object);
    lv_coord_t h = lv_obj_get_height_margin(object);

    ESP_LOGI("SCREEN", "Object %s coordinate, left:%d, top:%d, right:%d, bottom:%d", name, x, y, x+w, y+h);
}

lv_obj_t * draw_label(lv_obj_t * parent, lv_obj_t * reference, lv_align_t reference_align, lv_coord_t x, lv_coord_t y, lv_label_align_t text_align, const char * text, const lv_font_t * font, lv_color_t color) {
    lv_style_t * style = malloc(sizeof(lv_style_t));
    lv_style_init(style);
    lv_style_set_text_font(style, LV_STATE_DEFAULT, font);
    lv_style_set_text_color(style, LV_STATE_DEFAULT, color);
    
    lv_obj_t * label = lv_label_create(parent, NULL);
    lv_obj_add_style(label, LV_OBJ_PART_MAIN, style);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CROP);
    lv_label_set_align(label, text_align);
    lv_obj_align(label, reference, reference_align, x, y);

    return label;
}

lv_obj_t * draw_line(lv_obj_t * parent, lv_coord_t x1, lv_coord_t y1, lv_coord_t x2, lv_coord_t y2, lv_color_t color) {
    //ESP_LOGI("SCREEN", "x1:%d, y1:%d, x2:%d, y2:%d", x1, y1, x2, y2);
    lv_obj_t * line = lv_line_create(parent, NULL);

    lv_point_t * line_points = malloc(sizeof(lv_point_t) * 2);
    line_points[0].x = x1;
    line_points[0].y = y1;
    line_points[1].x = x2;
    line_points[1].y = y2;
    lv_line_set_points(line, line_points, 2);

    lv_style_t * style = malloc(sizeof(lv_style_t));
    lv_style_init(style);
    lv_style_set_line_width(style, LV_STATE_DEFAULT, 1);
    lv_style_set_line_color(style, LV_STATE_DEFAULT, color);
    lv_obj_add_style(line, LV_LINE_PART_MAIN, style);

    return line;
}

lv_obj_t * draw_rect(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t width, lv_coord_t height, lv_color_t color) {
    //ESP_LOGI("SCREEN", "x:%d, y:%d, width:%d, height:%d", x, y, width, height);
    lv_obj_t * rect = lv_obj_create(parent, NULL);
    lv_obj_align(rect, parent, LV_ALIGN_IN_TOP_LEFT, x, y);
    lv_obj_set_size(rect, width, height);

    lv_style_t * style = malloc(sizeof(lv_style_t));
    lv_style_init(style);
    //lv_style_set_bg_color(style, LV_STATE_DEFAULT, color);
    lv_style_set_radius(style, LV_STATE_DEFAULT, 0);
    lv_style_set_border_width(style, LV_STATE_DEFAULT, 0);
    lv_obj_add_style(rect, LV_OBJ_PART_MAIN, style);

    lv_obj_set_style_local_bg_color(rect, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color);

    return rect;
}

void fill_rect(lv_obj_t * rect, lv_color_t color) {
    lv_obj_set_style_local_bg_color(rect, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color);
}

lv_obj_t * draw_icon(lv_obj_t * parent, lv_obj_t * reference, lv_align_t reference_align, lv_coord_t x, lv_coord_t y, lv_label_align_t icon_align, const char * text) {
    lv_obj_t* icon = lv_label_create(parent, NULL);
    lv_label_set_text(icon, text);
    lv_label_set_recolor(icon, true);
    lv_label_set_align(icon, icon_align);
    lv_obj_align(icon, reference, reference_align, x, y);

    return icon;
}

void screen_init() {
    int32_t brightness = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_BRIGHTNESS);

    ui_data_mutex = xSemaphoreCreateMutex();
    ui_mutex = xGuiSemaphore;

    // Display logo picture in dark
    xSemaphoreTake(ui_mutex, portMAX_DELAY);
    lv_obj_t * opener_scr = lv_scr_act();
    lv_obj_set_style_local_bg_color(opener_scr, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_t * logo_image = lv_img_create(opener_scr, NULL);
    lv_img_set_src(logo_image, &paragliding_logo);
    lv_obj_align(logo_image, opener_scr, LV_ALIGN_CENTER, 0, 0);
    xSemaphoreGive(ui_mutex);

    // Release CPU, let ui task render screen
    vTaskDelay(pdMS_TO_TICKS(10));

    // Show logo picture smoothly
    for (int8_t i = 0; i<=brightness; i++) {
        Core2ForAWS_Display_SetBrightness(i);
        vTaskDelay(pdMS_TO_TICKS(1000/brightness));
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    xSemaphoreTake(ui_mutex, portMAX_DELAY);

    // Clean logo image
    lv_obj_clean(opener_scr);

    // Draw main screen
    main_screen = lv_obj_create(NULL, NULL);
    lv_obj_set_style_local_bg_color(main_screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_scr_load_anim(main_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, true);

    main_screen_tab_view = lv_tabview_create(main_screen, NULL);
    lv_tabview_set_btns_pos(main_screen_tab_view, LV_TABVIEW_TAB_POS_NONE);
    lv_obj_set_style_local_bg_color(main_screen_tab_view, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_tabview_set_anim_time(main_screen_tab_view, 400);
    lv_obj_set_event_cb(main_screen_tab_view, tabview_and_tab_envent_handler);

    dashboard_tab = lv_tabview_add_tab(main_screen_tab_view, "dashboard");
    lv_obj_set_event_cb(dashboard_tab, tabview_and_tab_envent_handler);
    motion_tab = lv_tabview_add_tab(main_screen_tab_view, "motion");
    lv_obj_set_event_cb(motion_tab, tabview_and_tab_envent_handler);
    compass_tab = lv_tabview_add_tab(main_screen_tab_view, "compass");
    lv_obj_set_event_cb(compass_tab, tabview_and_tab_envent_handler);

    setting_screen = lv_obj_create(NULL, NULL);
    lv_obj_set_style_local_bg_color(setting_screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    setting_screen_tab_view = lv_tabview_create(setting_screen, NULL);
    lv_tabview_set_btns_pos(setting_screen_tab_view, LV_TABVIEW_TAB_POS_NONE);
    lv_obj_set_style_local_bg_color(setting_screen_tab_view, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_tabview_set_anim_time(setting_screen_tab_view, 400);
    lv_obj_set_event_cb(setting_screen_tab_view, tabview_and_tab_envent_handler);

    time_tab = lv_tabview_add_tab(setting_screen_tab_view, "clock");
    lv_obj_set_event_cb(time_tab, tabview_and_tab_envent_handler);
    volume_tab = lv_tabview_add_tab(setting_screen_tab_view, "volume");
    lv_obj_set_event_cb(volume_tab, tabview_and_tab_envent_handler);
    brightness_tab = lv_tabview_add_tab(setting_screen_tab_view, "brightness");
    lv_obj_set_event_cb(brightness_tab, tabview_and_tab_envent_handler);
    temperature_tab = lv_tabview_add_tab(setting_screen_tab_view, "temperature");
    lv_obj_set_event_cb(temperature_tab, tabview_and_tab_envent_handler);
    bluetooth_tab = lv_tabview_add_tab(setting_screen_tab_view, "bluetooth");
    lv_obj_set_event_cb(bluetooth_tab, tabview_and_tab_envent_handler);
    reset_tab = lv_tabview_add_tab(setting_screen_tab_view, "reset");
    lv_obj_set_event_cb(reset_tab, tabview_and_tab_envent_handler);

    // Draw speed bar outline
    draw_line(main_screen, 0, 0, 0, 239, UI_COLOR_BORDER);
    draw_line(main_screen, 31, 0, 31, 239, UI_COLOR_BORDER);
    draw_line(main_screen, 288, 0, 288, 239, UI_COLOR_BORDER);
    draw_line(main_screen, 319, 0, 319, 239, UI_COLOR_BORDER);
    for (lv_coord_t i=0; i<9; i++) draw_line(main_screen, 0, i * 14, 31, i * 14, UI_COLOR_BORDER);
    for (lv_coord_t i=9; i<18; i++) draw_line(main_screen, 0, i * 14 + 1, 31, i * 14 + 1, UI_COLOR_BORDER);
    for (lv_coord_t i=0; i<9; i++) draw_line(main_screen, 288, i * 14, 319, i * 14, UI_COLOR_BORDER);
    for (lv_coord_t i=9; i<18; i++) draw_line(main_screen, 288, i * 14 + 1, 319, i * 14 + 1, UI_COLOR_BORDER);
    // Draw speed bar lift and sink unit
    for (lv_coord_t i=0; i<8; i++) scale_left_lift[7-i] = draw_rect(main_screen, 1, i * 14 + 1, 30, 13, UI_COLOR_BACKGROUND);
    for (lv_coord_t i=9; i<17; i++) scale_left_sink[i-9] = draw_rect(main_screen, 1, i * 14 + 2, 30, 13, UI_COLOR_BACKGROUND);
    for (lv_coord_t i=0; i<8; i++) scale_right_lift[7-i] = draw_rect(main_screen, 289, i * 14 + 1, 30, 13, UI_COLOR_BACKGROUND);
    for (lv_coord_t i=9; i<17; i++) scale_right_sink[i-9] = draw_rect(main_screen, 289, i * 14 + 2, 30, 13, UI_COLOR_BACKGROUND);
    // Draw speed bar gliding unit
    draw_rect(main_screen, 289, 113, 30, 14, LV_COLOR_ORANGE);
    draw_rect(main_screen, 1, 113, 30, 14, LV_COLOR_ORANGE);

    // Draw clock text and system icons
    clock_text = draw_label(main_screen, main_screen, LV_ALIGN_IN_TOP_LEFT, 48, 6, LV_LABEL_ALIGN_LEFT, "88:88:88", &lv_font_arial_rounded_mt_16, UI_COLOR_TEXT);
    battery_icon = draw_icon(main_screen, main_screen, LV_ALIGN_IN_TOP_RIGHT, -48, 4, LV_LABEL_ALIGN_RIGHT, LV_SYMBOL_BATTERY_FULL);
    charge_icon = draw_icon(main_screen, battery_icon, LV_ALIGN_CENTER, 0, 0, LV_LABEL_ALIGN_CENTER, LV_SYMBOL_CHARGE);
    volume_icon = draw_icon(main_screen, battery_icon, LV_ALIGN_OUT_LEFT_TOP, -8, 0, LV_LABEL_ALIGN_RIGHT, LV_SYMBOL_VOLUME_MAX);
    bluetooth_icon = draw_icon(main_screen, volume_icon, LV_ALIGN_OUT_LEFT_TOP, -8, 0, LV_LABEL_ALIGN_RIGHT, LV_SYMBOL_BLUETOOTH);
    sd_card_icon = draw_icon(main_screen, bluetooth_icon, LV_ALIGN_OUT_LEFT_TOP, -8, 0, LV_LABEL_ALIGN_RIGHT, LV_SYMBOL_SD_CARD);
    gps_icon = draw_icon(main_screen, sd_card_icon, LV_ALIGN_OUT_LEFT_TOP, -8, 0, LV_LABEL_ALIGN_RIGHT, LV_SYMBOL_GPS);
    lock_icon = draw_label(main_screen, gps_icon, LV_ALIGN_OUT_LEFT_TOP, -9, 1, LV_LABEL_ALIGN_RIGHT, LV_SYMBOL_LOCK, &awesome_14, UI_COLOR_TEXT );

    draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_LEFT, 48, 30, LV_LABEL_ALIGN_LEFT, "altitude(m)", LV_THEME_DEFAULT_FONT_SMALL, UI_COLOR_LABEL);
    altitude_text = draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_MID, 0, 45, LV_LABEL_ALIGN_CENTER, "8888", &lv_font_arial_rounded_mt_72, UI_COLOR_TEXT);

    draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_LEFT, 48, 116, LV_LABEL_ALIGN_LEFT, "v-speed(m/s)", LV_THEME_DEFAULT_FONT_SMALL, UI_COLOR_LABEL);
    speed_text = draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_RIGHT, -168, 135, LV_LABEL_ALIGN_RIGHT, "18.88", &lv_font_arial_rounded_mt_32, UI_COLOR_TEXT);

    draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_LEFT, 48, 173, LV_LABEL_ALIGN_LEFT, "temperature", LV_THEME_DEFAULT_FONT_SMALL, UI_COLOR_LABEL);
    temperature_text = draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_RIGHT, -168, 192, LV_LABEL_ALIGN_RIGHT, "25.88", &lv_font_arial_rounded_mt_32, UI_COLOR_TEXT);

    draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_LEFT, 168, 116, LV_LABEL_ALIGN_LEFT, "pressure(hp)", LV_THEME_DEFAULT_FONT_SMALL, UI_COLOR_LABEL);
    pressure_text = draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_RIGHT, -48, 135, LV_LABEL_ALIGN_RIGHT, "1013.2", &lv_font_arial_rounded_mt_32, UI_COLOR_TEXT);

    draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_LEFT, 168, 173, LV_LABEL_ALIGN_LEFT, "humidity(%)", LV_THEME_DEFAULT_FONT_SMALL, UI_COLOR_LABEL);
    humidity_text = draw_label(dashboard_tab, main_screen, LV_ALIGN_IN_TOP_RIGHT, -48, 192, LV_LABEL_ALIGN_RIGHT, "99.9", &lv_font_arial_rounded_mt_32, UI_COLOR_TEXT);

    static lv_color_t needle_colors[3] = {LV_COLOR_BLUE, LV_COLOR_ORANGE, LV_COLOR_PURPLE};
    motion_gauge = lv_gauge_create(motion_tab, NULL);
    lv_obj_set_size(motion_gauge, 192, 192);
    lv_gauge_set_range(motion_gauge, -180, 180);
    lv_gauge_set_scale(motion_gauge, 270, 31, 7);
    lv_gauge_set_needle_count(motion_gauge, 3, needle_colors);
    lv_obj_align(motion_gauge, motion_tab, LV_ALIGN_CENTER, 0, 12);
    lv_gauge_set_value(motion_gauge, 0, -30);
    lv_gauge_set_value(motion_gauge, 1, 0);
    lv_gauge_set_value(motion_gauge, 2, 60);

    compass_image = lv_img_create(compass_tab, NULL);
    lv_img_set_src(compass_image, &compass);
    lv_obj_set_size(compass_image, 192, 192);
    lv_obj_align(compass_image, compass_tab, LV_ALIGN_CENTER, 0, 12);
    lv_img_set_pivot(compass_image, 96, 96);

    // Draw clock tab elements
    draw_label(time_tab, time_tab, LV_ALIGN_IN_TOP_MID, 0, 16, LV_LABEL_ALIGN_CENTER, "System Time Setting", LV_THEME_DEFAULT_FONT_TITLE, UI_COLOR_LABEL);
    hour_roller = lv_roller_create(time_tab, NULL);
    lv_roller_set_options(hour_roller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23", LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(hour_roller, 3);
    lv_roller_set_auto_fit(hour_roller, false);
    lv_obj_set_width(hour_roller, 60);
    lv_obj_align(hour_roller, time_tab, LV_ALIGN_CENTER, -40, -10);
    lv_obj_t * separator_label = lv_label_create(time_tab, NULL);
    lv_label_set_static_text(separator_label, ":");
    lv_obj_set_width(separator_label, 4);
    lv_obj_align(separator_label, time_tab, LV_ALIGN_CENTER, 0, -10);
    minute_roller = lv_roller_create(time_tab, NULL);
    lv_roller_set_options(minute_roller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n"
                                         "20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n"
                                         "40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59", LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(minute_roller, 3);
    lv_roller_set_auto_fit(minute_roller, false);
    lv_obj_set_width(minute_roller, 60);
    lv_obj_align(minute_roller, time_tab, LV_ALIGN_CENTER, 40, -10);
    lv_obj_t * time_button = lv_btn_create(time_tab, NULL);
    lv_obj_set_event_cb(time_button, time_button_event_handler);
    lv_obj_align(time_button, time_tab, LV_ALIGN_IN_BOTTOM_MID, 0, -12);
    lv_obj_t * time_button_label = lv_label_create(time_button, NULL);
    lv_label_set_text(time_button_label, "Set Time");

    draw_label(volume_tab, volume_tab, LV_ALIGN_IN_TOP_MID, 0, 64, LV_LABEL_ALIGN_CENTER, "Volume Setting", LV_THEME_DEFAULT_FONT_TITLE, UI_COLOR_LABEL);
    volume_slider = lv_slider_create(volume_tab, NULL);
    lv_slider_set_range(volume_slider, 0, 100);
    lv_slider_set_value(volume_slider, config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME), LV_ANIM_OFF);
    lv_obj_align(volume_slider, volume_tab, LV_ALIGN_CENTER, 0, 32);
    lv_obj_set_event_cb(volume_slider, volume_slider_event_handler);

    draw_label(brightness_tab, brightness_tab, LV_ALIGN_IN_TOP_MID, 0, 64, LV_LABEL_ALIGN_CENTER, "Brightness Setting", LV_THEME_DEFAULT_FONT_TITLE, UI_COLOR_LABEL);
    brightness_slider = lv_slider_create(brightness_tab, NULL);
    lv_slider_set_range(brightness_slider, 0, 100);
    lv_slider_set_value(brightness_slider, config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_BRIGHTNESS), LV_ANIM_OFF);
    lv_obj_align(brightness_slider, brightness_tab, LV_ALIGN_CENTER, 0, 32);
    lv_obj_set_event_cb(brightness_slider, brightness_slider_event_handler);

    draw_label(temperature_tab, temperature_tab, LV_ALIGN_IN_TOP_MID, 0, 32, LV_LABEL_ALIGN_CENTER, "Adjust Temperature", LV_THEME_DEFAULT_FONT_TITLE, UI_COLOR_LABEL);
    temprature_spinbox = lv_spinbox_create(temperature_tab, NULL);
    lv_spinbox_set_range(temprature_spinbox, -9999, 9999);
    lv_spinbox_set_digit_format(temprature_spinbox, 4, 2);
    lv_spinbox_step_prev(temprature_spinbox);
    lv_obj_set_width(temprature_spinbox, 96);
    lv_textarea_set_text_align(temprature_spinbox, LV_LABEL_ALIGN_CENTER);
    lv_obj_set_style_local_text_font(temprature_spinbox, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_arial_rounded_mt_20);
    lv_obj_align(temprature_spinbox, temperature_tab, LV_ALIGN_CENTER, 0, -12);
    lv_coord_t height = lv_obj_get_height(temprature_spinbox);
    lv_obj_t * inc_button = lv_btn_create(temperature_tab, NULL);
    lv_obj_set_style_local_radius(inc_button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 5);
    lv_obj_set_size(inc_button, height, height);
    lv_obj_align(inc_button, temprature_spinbox, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    lv_obj_t * inc_button_label = lv_label_create(inc_button, NULL);
    lv_label_set_text(inc_button_label, LV_SYMBOL_PLUS);
    lv_obj_set_event_cb(inc_button, tempreature_spinbox_increment_event_handler);
    lv_obj_t * dec_button = lv_btn_create(temperature_tab, NULL);
    lv_obj_set_style_local_radius(dec_button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 5);
    lv_obj_set_size(dec_button, height, height);
    lv_obj_align(dec_button, temprature_spinbox, LV_ALIGN_OUT_LEFT_MID, -8, 0);
    lv_obj_t * dec_button_label = lv_label_create(dec_button, NULL);
    lv_label_set_text(dec_button_label, LV_SYMBOL_MINUS);
    lv_obj_set_event_cb(dec_button, tempreature_spinbox_decrement_event_handler);
    lv_obj_t * temperature_button = lv_btn_create(temperature_tab, NULL);
    lv_obj_set_event_cb(temperature_button, temperature_button_event_handler);
    lv_obj_align(temperature_button, temperature_tab, LV_ALIGN_IN_BOTTOM_MID, 0, -28);
    lv_obj_t * temperature_button_label = lv_label_create(temperature_button, NULL);
    lv_label_set_text(temperature_button_label, "Adjust");

    draw_label(bluetooth_tab, bluetooth_tab, LV_ALIGN_IN_TOP_MID, 0, 40, LV_LABEL_ALIGN_CENTER, "Enable or Dsable Bluetooth\nnote that\nRestart Takes Effect", LV_THEME_DEFAULT_FONT_TITLE, UI_COLOR_LABEL);
    bluetooth_switch = lv_switch_create(bluetooth_tab, NULL);
    lv_obj_set_event_cb(bluetooth_switch, bluetooth_switch_event_handler);
    lv_obj_align(bluetooth_switch, bluetooth_tab, LV_ALIGN_IN_BOTTOM_MID, 0, -48);

    draw_label(reset_tab, reset_tab, LV_ALIGN_IN_TOP_MID, 0, 40, LV_LABEL_ALIGN_CENTER, "Reset all Parameters\nto\nFactory Default", LV_THEME_DEFAULT_FONT_TITLE, UI_COLOR_LABEL);
    lv_obj_t * reset_button = lv_btn_create(reset_tab, NULL);
    lv_obj_set_event_cb(reset_button, reset_button_event_handler);
    lv_obj_align(reset_button, reset_tab, LV_ALIGN_IN_BOTTOM_MID, 0, -48);
    lv_obj_t * reset_button_label = lv_label_create(reset_button, NULL);
    lv_label_set_text(reset_button_label, "Reset");

    xSemaphoreGive(ui_mutex);

    // Draw speed bar animation show
    for (int i=0; i<8; i++) {
        xSemaphoreTake(ui_mutex, portMAX_DELAY);
        fill_rect(scale_left_lift[i], UI_COLOR_SCALE_LIFT);
        fill_rect(scale_left_sink[i], UI_COLOR_SCALE_SINK);
        fill_rect(scale_right_lift[i], UI_COLOR_SCALE_LIFT);
        fill_rect(scale_right_sink[i], UI_COLOR_SCALE_SINK);
        xSemaphoreGive(ui_mutex);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Draw speed bar animation hide
    for (int i=0; i<8; i++) {
        xSemaphoreTake(ui_mutex, portMAX_DELAY);
        fill_rect(scale_left_lift[i], UI_COLOR_BACKGROUND);
        fill_rect(scale_left_sink[i], UI_COLOR_BACKGROUND);
        fill_rect(scale_right_lift[i], UI_COLOR_BACKGROUND);
        fill_rect(scale_right_sink[i], UI_COLOR_BACKGROUND);
        xSemaphoreGive(ui_mutex);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    int32_t volume = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME);
    ui_set_volume(volume);

    key_state_t key_state = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_KEYS_LOCK);
    ui_set_key_state(key_state);

    current_screen = main_screen;

    xTaskCreate(ui_loop, "SCREENTASK", 16384, NULL, tskIDLE_PRIORITY+2, NULL);
}

static void stop_motor_and_delete_timer(xTimerHandle handle) {
    ui_stop_motor();
    xTimerDelete(handle, 10);
}

void ui_loop(void * arguemnt) {
    static double altitude = 0;
    static double speed = 0;
    static double pressure = 101325.0;
    static double temperature = 25.88;
    static double humidity = 99.99;

    for ( ; ; ) {
        vTaskDelay(pdMS_TO_TICKS(250));

        xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
        altitude = ui_altitude;
        xSemaphoreGive(ui_data_mutex);

        static char last_altitude_string[16] = {'\0'};
        char altitude_string[16];
        snprintf(altitude_string, 16, "%.0f", altitude);

        if (0 != strcmp(altitude_string, last_altitude_string)) {
            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            lv_label_set_text(altitude_text, altitude_string);
            xSemaphoreGive(ui_mutex);

            strcpy(last_altitude_string, altitude_string);
        }

        xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
        speed = ui_speed;
        xSemaphoreGive(ui_data_mutex);

        static char last_speed_string[16] = {'\0'};
        char speed_string[16];

        if (speed >= 100.0 || speed <= -10.0) {
            snprintf(speed_string, 16, "%.1f", speed);
        } else {
            snprintf(speed_string, 16, "%.2f", speed);
        }

        if (0 != strcmp (speed_string, last_speed_string)) {
            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            lv_label_set_text(speed_text, speed_string);
            xSemaphoreGive(ui_mutex);
            strcpy(last_speed_string, speed_string);
        }

        static int32_t last_meter_speed = 0;
        int32_t meter_speed = (int32_t)speed;
        meter_speed = ((meter_speed > 8) ? 8 : ((meter_speed < -8) ? -8 : meter_speed));

        if (meter_speed != last_meter_speed) {
            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            if (last_meter_speed > 0) {
                for (int i=0; i<last_meter_speed; i++) {
                    fill_rect(scale_left_lift[i], UI_COLOR_BACKGROUND);
                    fill_rect(scale_right_lift[i], UI_COLOR_BACKGROUND);
                }
            }
            if (last_meter_speed < 0) {
                for (int i=0; i<-last_meter_speed; i++) {
                    fill_rect(scale_left_sink[i], UI_COLOR_BACKGROUND);
                    fill_rect(scale_right_sink[i], UI_COLOR_BACKGROUND);
                }
            }
            if (meter_speed > 0) {
                for (int i=0; i<meter_speed; i++) {
                    fill_rect(scale_left_lift[i], UI_COLOR_SCALE_LIFT);
                    fill_rect(scale_right_lift[i], UI_COLOR_SCALE_LIFT);
                }
            }
            if (meter_speed < 0) {
                for (int i=0; i<-meter_speed; i++) {
                    fill_rect(scale_left_sink[i], UI_COLOR_SCALE_SINK);
                    fill_rect(scale_right_sink[i], UI_COLOR_SCALE_SINK);
                }
            }
            xSemaphoreGive(ui_mutex);    
            last_meter_speed = meter_speed;
        }

        xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
        pressure = ui_pressure;
        xSemaphoreGive(ui_data_mutex);

        static char last_pressure_string[16] = {'\0'};
        char pressure_string[16];
        snprintf(pressure_string, 16, "%.1f", pressure / 100);
        if (0 != strcmp(pressure_string, last_pressure_string)) {
            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            lv_label_set_text(pressure_text, pressure_string);
            xSemaphoreGive(ui_mutex);
            strcpy(last_pressure_string, pressure_string);
        }

        xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
        temperature = ui_temperature;
        xSemaphoreGive(ui_data_mutex);

        static char last_temperature_string[16] = {'\0'};
        char temperature_string[16];
        snprintf(temperature_string, 16, "%.1f", temperature);
        if (0 != strcmp(last_temperature_string, temperature_string)) {
            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            lv_label_set_text(temperature_text, temperature_string);
            xSemaphoreGive(ui_mutex);
            strcpy(last_temperature_string, temperature_string);
        }
        
        xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
        humidity = ui_humidity;
        xSemaphoreGive(ui_data_mutex);

        static char last_humidity_string[16] = {'\0'};
        char humidity_string[16];
        snprintf(humidity_string, 16, "%.1f", humidity);
        if (0 != strcmp(humidity_string, last_humidity_string)) {
            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            lv_label_set_text(humidity_text, humidity_string);
            xSemaphoreGive(ui_mutex);
            strcpy(last_humidity_string, humidity_string);
        }

        static rtc_date_t last_datetime = {2023, 3, 30, 15, 29, 0};
        rtc_date_t current_datetime;
        BM8563_GetTime(&current_datetime);

        if (current_datetime.second != last_datetime.second) {
            char string[16];
            snprintf(string, 16, "%02d:%02d:%02d", current_datetime.hour%64u, current_datetime.minute%64u, current_datetime.second%64u);

            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            lv_label_set_text(clock_text, string);
            xSemaphoreGive(ui_mutex);

            last_datetime = current_datetime;
        }

        typedef enum {
            BATTERY_VOLTAGE_LEVEL_EMPTY,
            BATTERY_VOLTAGE_LEVEL_1,
            BATTERY_VOLTAGE_LEVEL_2,
            BATTERY_VOLTAGE_LEVEL_3,
            BATTERY_VOLTAGE_LEVEL_FULL,
            BATTERY_VOLTAGE_LEVEL_INVALID,
        } battery_voltage_level_t;

        static battery_voltage_level_t last_battery_voltage_level = BATTERY_VOLTAGE_LEVEL_INVALID;
        int32_t battery_voltage = (int32_t)(Core2ForAWS_PMU_GetBatVolt() * 1000.0);
        battery_voltage_level_t battery_voltage_level;

        if (battery_voltage < 3000) {
            ESP_LOGI("SCREEN", "Run out of power, power off.");
            Axp192_PowerOff();
            battery_voltage_level = BATTERY_VOLTAGE_LEVEL_EMPTY;
        } else if (battery_voltage < 3250) {
            battery_voltage_level = BATTERY_VOLTAGE_LEVEL_EMPTY;
        } else if (battery_voltage < 3800) {
            battery_voltage_level = BATTERY_VOLTAGE_LEVEL_1;
        } else if (battery_voltage < 3950) {
            battery_voltage_level = BATTERY_VOLTAGE_LEVEL_2;
        } else if (battery_voltage < 4100) {
            battery_voltage_level = BATTERY_VOLTAGE_LEVEL_3;
        } else {
            battery_voltage_level = BATTERY_VOLTAGE_LEVEL_FULL;
        }

        if (battery_voltage_level != last_battery_voltage_level) {
            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            switch (battery_voltage_level) {
            case BATTERY_VOLTAGE_LEVEL_EMPTY:
                lv_label_set_text(battery_icon, "#ff0000 " LV_SYMBOL_BATTERY_EMPTY "#");
                break;
            case BATTERY_VOLTAGE_LEVEL_1:
                lv_label_set_text(battery_icon, "#ff0000 " LV_SYMBOL_BATTERY_1 "#");
                break;
            case BATTERY_VOLTAGE_LEVEL_2:
                lv_label_set_text(battery_icon, "#ff9900 " LV_SYMBOL_BATTERY_2 "#");
                break;
            case BATTERY_VOLTAGE_LEVEL_3:
                lv_label_set_text(battery_icon, "#0ab300 " LV_SYMBOL_BATTERY_3 "#");
                break;
            case BATTERY_VOLTAGE_LEVEL_FULL:
                lv_label_set_text(battery_icon, "#0ab300 " LV_SYMBOL_BATTERY_FULL "#");
                break;
            default:
                ;// do nothing
            }
            xSemaphoreGive(ui_mutex);

            last_battery_voltage_level = battery_voltage_level;
        }

        typedef enum {
            BATTERY_CHAEGE_STATE_CHARGING,
            BATTERY_CHAEGE_STATE_CONSUMING,
            BATTERY_CHARGE_STATE_INVALID,
        } battery_charge_state_t;

        static battery_charge_state_t last_battery_charge_state = BATTERY_CHARGE_STATE_INVALID;
        float battery_current = Core2ForAWS_PMU_GetBatCurrent();
        //ESP_LOGI("SCREEN", "Battery current:%f",battery_current);
        battery_charge_state_t battery_charge_state;

        if (battery_current >= 0.0f) {
            battery_charge_state = BATTERY_CHAEGE_STATE_CHARGING;
        } else {
            battery_charge_state = BATTERY_CHAEGE_STATE_CONSUMING;
        }

        if (battery_charge_state != last_battery_charge_state) {
            xSemaphoreTake(ui_mutex, portMAX_DELAY);
            switch (battery_charge_state) {
            case BATTERY_CHAEGE_STATE_CHARGING:
                lv_label_set_text(charge_icon, "#ffffff " LV_SYMBOL_CHARGE "#");
                break;
            case BATTERY_CHAEGE_STATE_CONSUMING:
                lv_label_set_text(charge_icon, "");
                break;
            default:
                ;// do nothing
            }
            xSemaphoreGive(ui_mutex);

            last_battery_charge_state = battery_charge_state;
        }

        if (Button_WasLongPress(button_left)) {
            if (KEY_STATE_LOCKED == ui_get_key_state()) {
                ui_set_key_state(KEY_STATE_UNLOCKED);
                config_set_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_KEYS_LOCK, KEY_STATE_UNLOCKED);
            } else {
                ui_set_key_state(KEY_STATE_LOCKED);
                config_set_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_KEYS_LOCK, KEY_STATE_LOCKED);
                if (current_screen == setting_screen) {
                    xSemaphoreTake(ui_mutex, portMAX_DELAY);
                    lv_event_send(main_screen_tab_view, LV_EVENT_REFRESH, NULL);
                    lv_scr_load_anim(main_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 400, 0, false);
                    xSemaphoreGive(ui_mutex);
                    current_screen = main_screen;
                }
            }

            Button_WasPressed(button_left);
            Button_WasPressed(button_right);
            Button_WasPressed(button_middle);

            xTimerHandle handle = xTimerCreate("reset", pdMS_TO_TICKS(200), pdFALSE, NULL, stop_motor_and_delete_timer);
            if (handle != NULL) {
                if(xTimerStart(handle, 0) == pdPASS) {
                    ui_start_motor();
                }
            }
        }

        if (ui_get_key_state() != KEY_STATE_LOCKED) {
            if (Button_IsRelease(button_left) && Button_WasPressed(button_left)) {
                int32_t volume = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME);

                if (volume >= 100) {
                    volume = 0;
                } else {
                    volume += 20;
                    if (volume > 100) {
                        volume = 100;
                    }
                }
                config_set_integer_temporary(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME, volume);
                //config_set_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME, volume);

                xSemaphoreTake(ui_mutex, portMAX_DELAY);
                lv_slider_set_value(volume_slider, config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME), LV_ANIM_OFF);
                xSemaphoreGive(ui_mutex);

                ui_set_volume(volume);
            }

            if (Button_IsRelease(button_right) && Button_WasPressed(button_right)) {
                int32_t brightness = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_BRIGHTNESS);

                if (brightness >= 100) {
                    brightness = 30;
                } else {
                    brightness += 10;
                    if (brightness > 100) {
                        brightness = 100;
                    }
                }
                config_set_integer_temporary(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_BRIGHTNESS, brightness);
                //config_set_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_BRIGHTNESS, brightness);

                xSemaphoreTake(ui_mutex, portMAX_DELAY);
                lv_slider_set_value(brightness_slider, config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_BRIGHTNESS), LV_ANIM_OFF);
                xSemaphoreGive(ui_mutex);

                ui_set_brightness(brightness);
            }

            if (Button_IsRelease(button_middle) && Button_WasPressed(button_middle)) {
                if (current_screen == main_screen) {
                    xSemaphoreTake(ui_mutex, portMAX_DELAY);
                    lv_event_send(setting_screen_tab_view, LV_EVENT_REFRESH, NULL);
                    lv_scr_load_anim(setting_screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 400, 0, false);
                    xSemaphoreGive(ui_mutex);
                    current_screen = setting_screen;
                } else if (current_screen == setting_screen) {
                    xSemaphoreTake(ui_mutex, portMAX_DELAY);
                    lv_event_send(main_screen_tab_view, LV_EVENT_REFRESH, NULL);
                    lv_scr_load_anim(main_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 400, 0, false);
                    xSemaphoreGive(ui_mutex);
                    current_screen = main_screen;
                } else {
                    ;// do nothing
                }
            }
        }
    }
}

void ui_set_altitude(double altitude) {
    xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
    ui_altitude = altitude;
    xSemaphoreGive(ui_data_mutex);
}

void ui_set_speed(double speed) {
    xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
    ui_speed = speed;
    xSemaphoreGive(ui_data_mutex);
}

void ui_set_pressure(double pressure) {
    xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
    ui_pressure = pressure;
    xSemaphoreGive(ui_data_mutex);
}

void ui_set_temperature(double temperature) {
    xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
    ui_temperature = temperature;
    xSemaphoreGive(ui_data_mutex);
}

void ui_set_humidity(double humidity) {
    xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
    ui_humidity = humidity;
    xSemaphoreGive(ui_data_mutex);
}

void rotate_compass(double angle) {
    xSemaphoreTake(ui_data_mutex, portMAX_DELAY);
    lv_img_set_angle(compass_image, 1800 - angle * 10);
    xSemaphoreGive(ui_data_mutex);
}

void ui_set_volume(int32_t volume) {
    typedef enum {
        VOLUME_LEVEL_MUTE,
        VOLUME_LEVEL_MID,
        VOLUME_LEVEL_MAX,
        VOLUME_LEVEL_INVALID,
    } ui_volume_state_t;

    static ui_volume_state_t last_volume_state = VOLUME_LEVEL_INVALID;
    ui_volume_state_t volume_state;
    
    if (volume <= 0) {
        volume_state = VOLUME_LEVEL_MUTE;
    } else if (volume >= 100) {
        volume_state = VOLUME_LEVEL_MAX;
    } else {
        volume_state = VOLUME_LEVEL_MID;
    }

    if (volume_state != last_volume_state) {
        xSemaphoreTake(ui_mutex, portMAX_DELAY);
        switch (volume_state) {
        case VOLUME_LEVEL_MUTE:
            lv_label_set_text(volume_icon, "#ffffff " LV_SYMBOL_MUTE "#");
            break;
        case VOLUME_LEVEL_MID:
            lv_label_set_text(volume_icon, "#ffffff " LV_SYMBOL_VOLUME_MID "#");
            break;
        case VOLUME_LEVEL_MAX:
            lv_label_set_text(volume_icon, "#ffffff " LV_SYMBOL_VOLUME_MAX "#");
            break;
        default:
            ;// do nothing
        }
        xSemaphoreGive(ui_mutex);

        last_volume_state = volume_state;
    }
}

void ui_set_brightness(int32_t brightness) {
    static int32_t last_brightness = -1;
    brightness = ((brightness > 100) ? 100 : ((brightness < 0) ? 0 : brightness));
    if (brightness != last_brightness) {
        Core2ForAWS_Display_SetBrightness(brightness);
        last_brightness = brightness;
    }
}

static uint32_t motor_counter = 0;
void ui_start_motor(void) {
    motor_counter += 1;
    if (motor_counter == 1) {
        Core2ForAWS_Motor_SetStrength(127);
    } else {
        ; // do nothing
    }
}

void ui_stop_motor(void) {
    motor_counter -= 1;
    if (motor_counter == 0) {
        Core2ForAWS_Motor_SetStrength(0);
    } else {
        ; // do nothing
    }
}

static key_state_t ui_key_state = KEY_STATE_INVALID;
void ui_set_key_state(key_state_t state) {
    if (state != ui_key_state) {
        xSemaphoreTake(ui_mutex, portMAX_DELAY);
        if (state == KEY_STATE_LOCKED) {
            lv_label_set_text(lock_icon, LV_SYMBOL_LOCK);
        } else if (state == KEY_STATE_UNLOCKED) {
            lv_label_set_text(lock_icon, "");
        } else {
            ; // do nothing
        }
        xSemaphoreGive(ui_mutex);

        ui_key_state = state;
    }
}

key_state_t ui_get_key_state(void) {
    return ui_key_state;
}

void ui_set_bluetooth(bluetooth_state_t state) {
    static bluetooth_state_t ui_bt_state = BLUETOOTH_STATE_INVALID;
    if (state != ui_bt_state) {
        xSemaphoreTake(ui_mutex, portMAX_DELAY);
        switch (state) {
        case BLUETOOTH_STATE_OFF:
            lv_label_set_text(bluetooth_icon, LV_SYMBOL_BLUETOOTH);
            break;
        case BLUETOOTH_STATE_ADVERTISING:
        case BLUETOOTH_STATE_CONNECTED:
            lv_label_set_text(bluetooth_icon, "#ffff00 " LV_SYMBOL_BLUETOOTH "#");
            break;
        case BLUETOOTH_STATE_INFORMED:
            lv_label_set_text(bluetooth_icon, "#0000ff " LV_SYMBOL_BLUETOOTH "#");
            break;
        default:
            ; // do nothing
        }
        xSemaphoreGive(ui_mutex);

        ui_bt_state = state;
    }
}

void volume_slider_event_handler(lv_obj_t * obj, lv_event_t event) {
    if (event == LV_EVENT_VALUE_CHANGED) {
        int32_t volume = (int32_t)lv_slider_get_value(obj);
        volume = ((volume > 100) ? 100 : ((volume < 0) ? 0 : volume));
        config_set_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME, volume);
        xSemaphoreGive(ui_mutex);
        ui_set_volume(volume);
        xSemaphoreTake(ui_mutex, portMAX_DELAY);
    }
}

void brightness_slider_event_handler(lv_obj_t * obj, lv_event_t event) {
    if (event == LV_EVENT_VALUE_CHANGED) {
        int32_t brightness = (int32_t)lv_slider_get_value(obj);
        brightness = ((brightness > 100) ? 100 : ((brightness < 0) ? 0 : brightness));
        config_set_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_BRIGHTNESS, brightness);
        ui_set_brightness(brightness);
    }
}

void reset_button_event_handler(lv_obj_t * obj, lv_event_t event) {
    if(event == LV_EVENT_CLICKED) {
        nvs_flash_erase();
        ui_start_motor();
        vTaskDelay(pdMS_TO_TICKS(500));
        ui_stop_motor();
        esp_restart();
    }
}

void time_button_event_handler(lv_obj_t * obj, lv_event_t event) {
    if(event == LV_EVENT_CLICKED) {
        rtc_date_t datetime;
        BM8563_GetTime(&datetime);
        datetime.hour = (uint8_t)lv_roller_get_selected(hour_roller);
        datetime.minute = (uint8_t)lv_roller_get_selected(minute_roller);
        BM8563_SetTime(&datetime);

        xTimerHandle handle = xTimerCreate("reset", pdMS_TO_TICKS(200), pdFALSE, NULL, stop_motor_and_delete_timer);
        if (handle != NULL) {
            if(xTimerStart(handle, 0) == pdPASS) {
                ui_start_motor();
            }
        }
    }
}

void tempreature_spinbox_increment_event_handler(lv_obj_t * obj, lv_event_t event) {
    if(event == LV_EVENT_CLICKED) {
        lv_spinbox_increment(temprature_spinbox);
    }
}

void tempreature_spinbox_decrement_event_handler(lv_obj_t * obj, lv_event_t event) {
    if(event == LV_EVENT_CLICKED) {
        lv_spinbox_decrement(temprature_spinbox);
    }
}

void temperature_button_event_handler(lv_obj_t * obj, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
        int32_t value = lv_spinbox_get_value(temprature_spinbox);
        config_set_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_TEMPERATURE_ADJUSTMENT, value * 10);

        xTimerHandle handle = xTimerCreate("reset", pdMS_TO_TICKS(200), pdFALSE, NULL, stop_motor_and_delete_timer);
        if (handle != NULL) {
            if(xTimerStart(handle, 0) == pdPASS) {
                ui_start_motor();
            }
        }
    }
}

void bluetooth_switch_event_handler(lv_obj_t * obj, lv_event_t event) {
    if (event == LV_EVENT_VALUE_CHANGED) {
        bool state = lv_switch_get_state(bluetooth_switch);
        config_set_integer(CONFIG_NAMESPACE_BLUETOOTH, CONFIG_BLUETOOTH_ENABLE, (uint32_t)state);

        xTimerHandle handle = xTimerCreate("reset", pdMS_TO_TICKS(200), pdFALSE, NULL, stop_motor_and_delete_timer);
        if (handle != NULL) {
            if(xTimerStart(handle, 0) == pdPASS) {
                ui_start_motor();
            }
        }
    }
}

void tabview_and_tab_envent_handler(lv_obj_t * obj, lv_event_t event){
    if (obj == main_screen_tab_view) {
        xSemaphoreGive(ui_mutex);
        ui_set_volume(config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME));
        xSemaphoreTake(ui_mutex, portMAX_DELAY);
    } else if (obj == setting_screen_tab_view) {
        if (event == LV_EVENT_REFRESH || event == LV_EVENT_VALUE_CHANGED) {
            lv_obj_t * active = lv_tabview_get_tab(obj, lv_tabview_get_tab_act(obj));
            lv_event_send(active, LV_EVENT_REFRESH, NULL);
        }
    } else if (obj == time_tab) {
        rtc_date_t datetime;
        BM8563_GetTime(&datetime);
        lv_roller_set_selected(hour_roller, datetime.hour, LV_ANIM_OFF);
        lv_roller_set_selected(minute_roller, datetime.minute, LV_ANIM_OFF);
    } else if (obj == brightness_tab) {
        lv_slider_set_value(brightness_slider, config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_BRIGHTNESS), LV_ANIM_OFF);
    } else if (obj == temperature_tab) {
        lv_spinbox_set_value(temprature_spinbox, config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_TEMPERATURE_ADJUSTMENT) / 10);
    } else if (obj == bluetooth_tab) {
        if (config_get_integer(CONFIG_NAMESPACE_BLUETOOTH, CONFIG_BLUETOOTH_ENABLE)) {
            lv_switch_on(bluetooth_switch, LV_ANIM_OFF);
        } else {
            lv_switch_off(bluetooth_switch, LV_ANIM_OFF);
        }
    }
}