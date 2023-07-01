#include "config.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "core2forAWS.h"

config_item_t config_sound_items[] = {
    #ifdef DECLARE_CONFIG_SOUND_STRING
    #undef DECLARE_CONFIG_SOUND_STRING
    #endif
    #define DECLARE_CONFIG_SOUND_STRING(_index, _name, _type, ...) {.name = _name, .type = _type, .string = __VA_ARGS__}
    #ifdef DECLARE_CONFIG_SOUND_INTEGER
    #undef DECLARE_CONFIG_SOUND_INTEGER
    #endif
    #define DECLARE_CONFIG_SOUND_INTEGER(_index, _name, _type, _value) {.name = _name, .type = _type, .integer = _value}
    #include "config_sound.inc"
};

config_item_t config_speed_items[] = {
    #ifdef DECLARE_CONFIG_SPEED_STRING
    #undef DECLARE_CONFIG_SPEED_STRING
    #endif
    #define DECLARE_CONFIG_SPEED_STRING(_index, _name, _type, ...) {.name = _name, .type = _type, .string = __VA_ARGS__}
    #ifdef DECLARE_CONFIG_SPEED_INTEGER
    #undef DECLARE_CONFIG_SPEED_INTEGER
    #endif
    #define DECLARE_CONFIG_SPEED_INTEGER(_index, _name, _type, _value) {.name = _name, .type = _type, .integer = _value}
    #include "config_speed.inc"
};

config_item_t config_system_items[] = {
    #ifdef DECLARE_CONFIG_SYSTEM_STRING
    #undef DECLARE_CONFIG_SYSTEM_STRING
    #endif
    #define DECLARE_CONFIG_SYSTEM_STRING(_index, _name, _type, ...) {.name = _name, .type = _type, .string = __VA_ARGS__}
    #ifdef DECLARE_CONFIG_SYSTEM_INTEGER
    #undef DECLARE_CONFIG_SYSTEM_INTEGER
    #endif
    #define DECLARE_CONFIG_SYSTEM_INTEGER(_index, _name, _type, _value) {.name = _name, .type = _type, .integer = _value}
    #include "config_system.inc"
};

config_item_t config_bluetooth_items[] = {
    #ifdef DECLARE_CONFIG_BLUETOOTH_STRING
    #undef DECLARE_CONFIG_BLUETOOTH_STRING
    #endif
    #define DECLARE_CONFIG_BLUETOOTH_STRING(_index, _name, _type, ...) {.name = _name, .type = _type, .string = {__VA_ARGS__}}
    #ifdef DECLARE_CONFIG_BLUETOOTH_INTEGER
    #undef DECLARE_CONFIG_BLUETOOTH_INTEGER
    #endif
    #define DECLARE_CONFIG_BLUETOOTH_INTEGER(_index, _name, _type, _value) {.name = _name, .type = _type, .integer = _value}
    #include "config_bluetooth.inc"
};

config_namespace_t config_namespace[] = {
#ifdef DECLARE_CONFIG_NAMESPACE
#undef DECLARE_CONFIG_NAMESPACE
#endif
#define DECLARE_CONFIG_NAMESPACE(_index, _name, _items, _count) {.name = _name, .config_items = _items, .item_count = _count}
#include "config_namespace.inc"
};

static SemaphoreHandle_t config_mutex = NULL;

void config_load_all_namespace(void) {
    if (config_mutex == NULL) {
        config_mutex = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(config_mutex, portMAX_DELAY);

    for (int i=0; config_namespace[i].name != NULL; i++) {
        for (int j=0; config_namespace[i].config_items[j].name != NULL; j++) {
            xSemaphoreGive(config_mutex);
            config_load_item(i, j);
            xSemaphoreTake(config_mutex, portMAX_DELAY);
        }
    }

    xSemaphoreGive(config_mutex);
}

esp_err_t config_load_item(int namespace_index, int index) {
    nvs_handle_t handle;
    size_t size = CONFIG_STRING_MAX_LENGTH;
    esp_err_t ret;

    xSemaphoreTake(config_mutex, portMAX_DELAY);
    if (namespace_index >= CONFIG_NAMESPACE_ANY || index >= config_namespace[namespace_index].item_count) {
        ESP_LOGI("CONFIG", "config_load_item invalid parameter namespace_index:%d, index:%d", namespace_index, index);
        ret = ESP_FAIL;
    } else {
        ret = nvs_open(config_namespace[namespace_index].name, NVS_READWRITE, &handle);
        if (ret == ESP_OK) {
            config_item_t * item = &(config_namespace[namespace_index].config_items[index]);
            switch (item->type) {
            case NVS_TYPE_I32:
                ret = nvs_get_i32(handle, item->name, &(item->integer));
                if (ret == ESP_ERR_NVS_NOT_FOUND) {
                    ret = nvs_set_i32(handle, item->name, item->integer);
                    if (ret != ESP_OK) {
                        ESP_LOGI("CONFIG", "config_load_item->nvs_set_i32 %s to %d return %x", item->name, item->integer, ret);
                    } else {
                        ret = nvs_commit(handle);
                        if (ret != ESP_OK) {
                            ESP_LOGI("COMFIG", "nvs_commit return %x", ret);
                        }
                    }
                } else if (ret != ESP_OK) {
                    ESP_LOGI("CONFIG", "config_load_item->nvs_get_i32 %s return %x", item->name, ret);
                } else {
                    // success,do nothing
                }
                break;
            case NVS_TYPE_STR:
                ret = nvs_get_str(handle, item->name, item->string, &size);
                if (ret == ESP_ERR_NVS_NOT_FOUND || ret == ESP_ERR_NVS_INVALID_LENGTH) {
                    ret = nvs_set_str(handle, item->name, item->string);
                    if (ret != ESP_OK) {
                        ESP_LOGI("CONFIG", "config_load_item->nvs_set_str %s to %s return %x", item->name, item->string, ret);
                    } else {
                        ret = nvs_commit(handle);
                        if (ret != ESP_OK) {
                            ESP_LOGI("COMFIG", "nvs_commit return %x", ret);
                        }
                    }
                } else if (ret != ESP_OK) {
                    ESP_LOGI("CONFIG", "config_load_item->nvs_get_str %s return %x", item->name, ret);
                } else {
                    // success,do nothing
                }
                break;
            default:
                ESP_LOGI("CONFIG", "unsupported tiem type %d", item->type);
                ret = ESP_FAIL;
            }

            nvs_close(handle);
        } else {
            ESP_LOGI("CONFIG", "config_load_item->nvs_open \"%s\" in read-write mode return %x", config_namespace[namespace_index].name, ret);
        }
    }

    ESP_LOGI("CONFIG", "load item %s %s %d", config_namespace[namespace_index].name, config_namespace[namespace_index].config_items[index].name, config_namespace[namespace_index].config_items[index].integer);

    xSemaphoreGive(config_mutex);

    return ret;
}

esp_err_t config_save_item(int namespace_index, int index) {
    nvs_handle_t handle;
    esp_err_t ret;

    xSemaphoreTake(config_mutex, portMAX_DELAY);
    if (namespace_index >= CONFIG_NAMESPACE_ANY || index >= config_namespace[namespace_index].item_count) {
        ESP_LOGI("CONFIG", "config_save_item invalid parameter namespace_index:%d, index:%d", namespace_index, index);
        ret = ESP_FAIL;
    } else {
        ret = nvs_open(config_namespace[namespace_index].name, NVS_READWRITE, &handle);
        if (ret == ESP_OK) {
            config_item_t * item = &(config_namespace[namespace_index].config_items[index]);
            switch (item->type) {
            case NVS_TYPE_I32:
                ret = nvs_set_i32(handle, item->name, item->integer);
                if (ret != ESP_OK) {
                    ESP_LOGI("CONFIG", "config_save_item->nvs_set_i32 %s to %x return %x", item->name, item->integer, ret);
                } else {
                    ret = nvs_commit(handle);
                    if (ret != ESP_OK) {
                        ESP_LOGI("COMFIG", "nvs_commit return %x", ret);
                    }
                }
                break;
            case NVS_TYPE_STR:
                if (strlen(item->string) >= CONFIG_STRING_MAX_LENGTH) {
                    ESP_LOGI("CONFIG", "config_save_item parameter %s too long", item->string);
                    ret = ESP_FAIL;
                } else {
                    ret = nvs_set_str(handle, item->name, item->string);
                    if (ret != ESP_OK) {
                        ESP_LOGI("CONFIG", "config_save_item->nvs_set_str %s to %s return %x", item->name, item->string, ret);
                    } else {
                        ret = nvs_commit(handle);
                        if (ret != ESP_OK) {
                            ESP_LOGI("COMFIG", "nvs_commit return %x", ret);
                        }
                    }
                }
                break;
            default:
                ESP_LOGI("CONFIG", "unsupported tiem type %d", item->type);
                ret = ESP_FAIL;
            }

            nvs_close(handle);
        } else {
            ESP_LOGI("CONFIG", "config_load_item->nvs_open \"%s\" in read-write mode return %x", config_namespace[namespace_index].name, ret);
        }
    }
    xSemaphoreGive(config_mutex);

    return ret;
}

esp_err_t _config_get_integer(int namespace_index, int index, int32_t * integer) {
    esp_err_t ret;
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    if (namespace_index >= CONFIG_NAMESPACE_ANY || index >= config_namespace[namespace_index].item_count) {
        ESP_LOGI("CONFIG", "config_get_integer invalid parameter namespace_index:%d, index:%d", namespace_index, index);
        ret = ESP_FAIL;
    } else if (config_namespace[namespace_index].config_items[index].type != NVS_TYPE_I32) {
        ESP_LOGI("CONFIG", "config_get_integer invalid type, type of namespace:%d index:%d is %d", namespace_index, index, config_namespace[namespace_index].config_items[index].type);
        ret = ESP_FAIL;
    } else {
        *integer = config_namespace[namespace_index].config_items[index].integer;
        ret = ESP_OK;
    }
    xSemaphoreGive(config_mutex);

    return ret;
}

esp_err_t _config_get_string(int namespace_index, int index, char * string, size_t size) {
    esp_err_t ret;
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    if (namespace_index >= CONFIG_NAMESPACE_ANY || index >= config_namespace[namespace_index].item_count) {
        ESP_LOGI("CONFIG", "config_get_string invalid parameter namespace_index:%d, index:%d", namespace_index, index);
        ret = ESP_FAIL;
    } else if (config_namespace[namespace_index].config_items[index].type != NVS_TYPE_STR) {
        ESP_LOGI("CONFIG", "config_get_string invalid type, type of namespace:%d index:%d is %d", namespace_index, index, config_namespace[namespace_index].config_items[index].type);
        ret = ESP_FAIL;
    } else if (size < strlen(config_namespace[namespace_index].config_items[index].string) + 1) {
        ESP_LOGI("CONFIG", "config_get_string nsufficient space:%d", size);
        ret = ESP_FAIL;
    } else {
        strcpy(string, config_namespace[namespace_index].config_items[index].string);
        ret = ESP_OK;
    }
    xSemaphoreGive(config_mutex);

    return ret;
}

int32_t config_get_integer(int namespace_index, int index) {
    int32_t integer;
    esp_err_t ret = _config_get_integer(namespace_index, index, &integer);
    assert(ret == ESP_OK);
    return integer;
}

char * config_get_malloc_string(int namespace_index, int index) {
    char * string = malloc(sizeof(char) * CONFIG_STRING_MAX_LENGTH);
    esp_err_t ret = _config_get_string(namespace_index, index, string, CONFIG_STRING_MAX_LENGTH);
    assert(ret == ESP_OK);
    return string;
}

esp_err_t _config_set_integer(int namespace_index, int index, int32_t integer, bool save_to_nvs) {
    esp_err_t ret;
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    if (namespace_index >= CONFIG_NAMESPACE_ANY || index >= config_namespace[namespace_index].item_count) {
        ESP_LOGI("CONFIG", "config_set_integer invalid parameter namespace_index:%d, index:%d", namespace_index, index);
        ret = ESP_FAIL;
    } else if (config_namespace[namespace_index].config_items[index].type != NVS_TYPE_I32) {
        ESP_LOGI("CONFIG", "config_set_integer invalid type, type of namespace:%d index:%d is %d", namespace_index, index, config_namespace[namespace_index].config_items[index].type);
        ret = ESP_FAIL;
    } else {
        config_namespace[namespace_index].config_items[index].integer = integer;
        if (save_to_nvs) {
            xSemaphoreGive(config_mutex);
            ret = config_save_item(namespace_index, index);
            xSemaphoreTake(config_mutex, portMAX_DELAY);
        } else {
            ret = ESP_OK;
        }
    }
    xSemaphoreGive(config_mutex);

    return ret;
}

esp_err_t _config_set_string(int namespace_index, int index, const char * string, bool save_to_nvs) {
    esp_err_t ret;
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    if (namespace_index >= CONFIG_NAMESPACE_ANY || index >= config_namespace[namespace_index].item_count) {
        ESP_LOGI("CONFIG", "config_set_integer invalid parameter namespace_index:%d, index:%d", namespace_index, index);
        ret = ESP_FAIL;
    } else if (config_namespace[namespace_index].config_items[index].type != NVS_TYPE_STR) {
        ESP_LOGI("CONFIG", "config_set_string invalid type, type of namespace:%d index:%d is %d", namespace_index, index, config_namespace[namespace_index].config_items[index].type);
        ret = ESP_FAIL;
    } else if (strlen(string) >= CONFIG_STRING_MAX_LENGTH) {
        ESP_LOGI("CONFIG", "config_set_string value %s is too long", string);
        ret = ESP_FAIL;
    } else {
        strcpy(config_namespace[namespace_index].config_items[index].string, string);
        if (save_to_nvs) {
            xSemaphoreGive(config_mutex);
            ret = config_save_item(namespace_index, index);
            xSemaphoreTake(config_mutex, portMAX_DELAY);
        } else {
            ret = ESP_OK;
        }
    }
    xSemaphoreGive(config_mutex);

    return ret;
}

/*
void config_copy(bluethroat_parameters_t * target) {
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    memcpy(target, &bluethroat_parameters, sizeof(bluethroat_parameters_t));
    xSemaphoreGive(config_mutex);
}

int32_t config_get_volume(void) {
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    int32_t volume = bluethroat_parameters.system.volume;
    xSemaphoreGive(config_mutex);

    return volume;
}

void config_set_volume(int32_t volume) {
    nvs_handle_t nvs_handle;

    xSemaphoreTake(config_mutex, portMAX_DELAY);
    bluethroat_parameters.system.volume = (volume > 100) ? 100 : (volume < 0) ? 0 : volume;
    if (ESP_OK == nvs_open("system", NVS_READONLY, &nvs_handle)) {
        nvs_set_i32(nvs_handle, "volume", bluethroat_parameters.system.volume);
        nvs_close(nvs_handle);
    }

    xSemaphoreGive(config_mutex);
}

int32_t config_get_brightness(void) {
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    int32_t brightness = bluethroat_parameters.system.brightness;
    xSemaphoreGive(config_mutex);

    return brightness;
}

void config_set_brightness(int32_t brightness) {
    nvs_handle_t nvs_handle;

    xSemaphoreTake(config_mutex, portMAX_DELAY);
    bluethroat_parameters.system.brightness = (brightness > 100) ? 100 : (brightness < 0) ? 0 : brightness;
    if (ESP_OK == nvs_open("system", NVS_READONLY, &nvs_handle)) {
        esp_err_t ret = nvs_set_i32(nvs_handle, "brightness", bluethroat_parameters.system.brightness);
        ESP_LOGI("CONFIG", "ret:%d, value:%d", ret, brightness);
        nvs_close(nvs_handle);
    } else {
        ESP_LOGI("CONFIG", "nvs_open failed");
    }

    xSemaphoreGive(config_mutex);
}

int32_t config_get_sampling_rate(void) {
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    int32_t sampling_rate = bluethroat_parameters.sampling.rate;
    xSemaphoreGive(config_mutex);

    return sampling_rate;
}

int32_t config_get_sampling_bit_depth(void) {
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    int32_t sampling_bit_depth = bluethroat_parameters.sampling.bit_depth;
    xSemaphoreGive(config_mutex);

    return sampling_bit_depth;
}

int32_t config_get_speed_altitude_window(void) {
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    int32_t speed_altitude_window = bluethroat_parameters.speed.altitude_window;
    xSemaphoreGive(config_mutex);

    return speed_altitude_window;

}
*/