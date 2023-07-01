#include "core2forAWS.h"

#define DASHBOARD_TAB_NAME      "dashboard"
#define MOTION_TAB_NAME         "motion"
#define COMPASS_TAB_NAME        "compass"

#define BRIGHTNESS_TAB_NAME     "brightness"
#define VOLUME_TAB_NAME         "volume"

void screen_init();
void ui_loop(void * arguemnt);
void ui_set_altitude(double altitude);
void ui_set_speed(double speed);
void ui_set_pressure(double pressure);
void ui_set_temperature(double temperature);
void ui_set_humidity(double humidity);
void rotate_compass(double angle);
void ui_set_volume(int32_t volume);
void ui_set_brightness(int32_t brightness);
void ui_start_motor(void);
void ui_stop_motor(void);
void volume_slider_event_handler(lv_obj_t * obj, lv_event_t event);
void brightness_slider_event_handler(lv_obj_t * obj, lv_event_t event);
void reset_button_event_handler(lv_obj_t * obj, lv_event_t event);
void time_button_event_handler(lv_obj_t * obj, lv_event_t event);
void tempreature_spinbox_increment_event_handler(lv_obj_t * obj, lv_event_t event);
void tempreature_spinbox_decrement_event_handler(lv_obj_t * obj, lv_event_t event);
void temperature_button_event_handler(lv_obj_t * obj, lv_event_t event);
void tabview_and_tab_envent_handler(lv_obj_t * obj, lv_event_t event);
void bluetooth_switch_event_handler(lv_obj_t * obj, lv_event_t event);

typedef enum {
    BLUETOOTH_STATE_OFF,
    BLUETOOTH_STATE_ADVERTISING,
    BLUETOOTH_STATE_CONNECTED,
    BLUETOOTH_STATE_INFORMED,
    BLUETOOTH_STATE_INVALID,
} bluetooth_state_t;

void ui_set_bluetooth(bluetooth_state_t state);

typedef enum {
    KEY_STATE_UNLOCKED,
    KEY_STATE_LOCKED,
    KEY_STATE_INVALID,
} key_state_t;

void ui_set_key_state(key_state_t lock);
key_state_t ui_get_key_state(void);
