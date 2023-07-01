#pragma once

#include <stdbool.h>
#include <nvs.h>
#include "core2forAWS.h"

#define OVERALL_TONE_LIFT_CYCLE_MAXIMUM             (1000)
#define OVERALL_TONE_SAMPLE_RATE                    (44100)

#define DEFAULT_TONE_LIFT_FREQUENCY_MINIMUM         (600)
#define DEFAULT_TONE_LIFT_FREQUENCY_MAXIMUM         (1800)
#define DEFAULT_TONE_LIFT_FREQUENCY_FACTOR          (400)
#define DEFAULT_TONE_LIFT_CYCLE_MINIMUM             (125)
#define DEFAULT_TONE_LIFT_CYCLE_MAXIMUM             (500)
#define DEFAULT_TONE_LIFT_CYCLE_FACTOR              (300)
#define DEFAULT_TONE_LIFT_DUTY_MINIMUM              (80)
#define DEFAULT_TONE_LIFT_DUTY_MAXIMUM              (200)
#define DEFAULT_TONE_LIFT_DUTY_FACTOR               (500)

#define DEFAULT_TONE_SINK_FREQUENCY_MINIMUM         (300)
#define DEFAULT_TONE_SINK_FREQUENCY_MAXIMUM         (800)
#define DEFAULT_TONE_SINK_FREQUENCY_FACTOR          (400)
#define DEFAULT_TONE_SINK_DUAL_TONE_FACTOR          (4)

#define DEFAULT_SAMPLING_RATE                       (44100)
#define DEFAULT_SAMPLING_BIT_DEPTH                  I2S_BITS_PER_SAMPLE_16BIT

#define DEFAULT_SPEED_AVERAGE_TIME_WINDOW           (50)
#define DEFAULT_SPEED_AVERAGE_ALTITUDE_WINDOW       (16)
#define DEFAULT_SPEED_THRESHOLD_LIFT_START          (25)
#define DEFAULT_SPEED_THRESHOLD_LIFT_STOP           (25)
#define DEFAULT_SPEED_THRESHOLD_SINK_START          (-180)
#define DEFAULT_SPEED_THRESHOLD_SINK_STOP           (-180)

#define DEFAULT_SYSTEM_VOLUME                       (0)
#define DEFAULT_SYSTEM_BRIGHTNESS                   (80)
#define DEFAULT_SYSTEM_TEMPERATURE_ADJUSTMENT       (-12500)
#define DEFAULT_SYSTEM_AUTO_POWEROFF_TIMEOUT        (300000)

#define CONFIG_STRING_MAX_LENGTH                    (32)

typedef struct {
    const char * name;
    nvs_type_t type;
    union {
        char string[CONFIG_STRING_MAX_LENGTH];
        int32_t integer;
    };
} config_item_t;

typedef enum {
    #ifdef DECLARE_CONFIG_SOUND_STRING
    #undef DECLARE_CONFIG_SOUND_STRING
    #endif
    #define DECLARE_CONFIG_SOUND_STRING(_index, _name, _type, ...) _index
    #ifdef DECLARE_CONFIG_SOUND_INTEGER
    #undef DECLARE_CONFIG_SOUND_INTEGER
    #endif
    #define DECLARE_CONFIG_SOUND_INTEGER(_index, _name, _type, _value) _index
    #include "config_sound.inc"
} config_sound_index_t;

typedef enum {
    #ifdef DECLARE_CONFIG_SPEED_STRING
    #undef DECLARE_CONFIG_SPEED_STRING
    #endif
    #define DECLARE_CONFIG_SPEED_STRING(_index, _name, _type, ...) _index
    #ifdef DECLARE_CONFIG_SPEED_INTEGER
    #undef DECLARE_CONFIG_SPEED_INTEGER
    #endif
    #define DECLARE_CONFIG_SPEED_INTEGER(_index, _name, _type, _value) _index
    #include "config_speed.inc"
} config_speed_index_t;

typedef enum {
    #ifdef DECLARE_CONFIG_SYSTEM_STRING
    #undef DECLARE_CONFIG_SYSTEM_STRING
    #endif
    #define DECLARE_CONFIG_SYSTEM_STRING(_index, _name, _type, ...) _index
    #ifdef DECLARE_CONFIG_SYSTEM_INTEGER
    #undef DECLARE_CONFIG_SYSTEM_INTEGER
    #endif
    #define DECLARE_CONFIG_SYSTEM_INTEGER(_index, _name, _type, _value) _index
    #include "config_system.inc"
} config_system_index_t;

typedef enum {
    #ifdef DECLARE_CONFIG_BLUETOOTH_STRING
    #undef DECLARE_CONFIG_BLUETOOTH_STRING
    #endif
    #define DECLARE_CONFIG_BLUETOOTH_STRING(_index, _name, _type, ...) _index
    #ifdef DECLARE_CONFIG_BLUETOOTH_INTEGER
    #undef DECLARE_CONFIG_BLUETOOTH_INTEGER
    #endif
    #define DECLARE_CONFIG_BLUETOOTH_INTEGER(_index, _name, _type, _value) _index
    #include "config_bluetooth.inc"
} config_bluetooth_index_t;

typedef struct {
    const char * name;
    config_item_t * config_items;
    uint32_t item_count;
} config_namespace_t;

typedef enum {
    #ifdef DECLARE_CONFIG_NAMESPACE
    #undef DECLARE_CONFIG_NAMESPACE
    #endif
    #define DECLARE_CONFIG_NAMESPACE(_index, _name, _items, _count) _index
    #include "config_namespace.inc"
} config_namespace_index_t;

extern config_namespace_t config_namespace[];
extern config_item_t config_sound_items[];
extern config_item_t config_speed_items[];
extern config_item_t config_system_items[];
extern config_item_t config_bluetooth_items[];

void config_load_all_namespace(void);
esp_err_t config_load_item(int namespace_index, int index);
esp_err_t config_save_item(int namespace_index, int index);

esp_err_t _config_get_integer(int namespace_index, int index, int32_t * integer);
esp_err_t _config_get_string(int namespace_index, int index, char * string, size_t size);
int32_t config_get_integer(int namespace_index, int index);
char * config_get_malloc_string(int namespace_index, int index);

esp_err_t _config_set_integer(int namespace_index, int index, int32_t integer, bool save_to_nvs);
esp_err_t _config_set_string(int namespace_index, int index, const char * string, bool save_to_nvs);
#define config_set_integer(_namespace_index, _index, _integer) _config_set_integer(_namespace_index, _index, _integer, true)
#define config_set_string(_namespace_index, _index, _string) _config_set_integer(_namespace_index, _index, _string, true)
#define config_set_integer_temporary(_namespace_index, _index, _integer) _config_set_integer(_namespace_index, _index, _integer, false)
#define config_set_string_temporary(_namespace_index, _index, _string) _config_set_integer(_namespace_index, _index, _string, false)
/*
void config_copy(bluethroat_parameters_t * target);
int32_t config_get_volume(void);
void config_set_volume(int32_t volume);
int32_t config_get_brightness(void);
void config_set_brightness(int32_t brightness);
int32_t config_get_sampling_rate(void);
int32_t config_get_sampling_bit_depth(void);
int32_t config_get_speed_altitude_window(void);
*/