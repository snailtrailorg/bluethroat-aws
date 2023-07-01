#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32/rom/crc.h"
#include "sht3x.h"

#define TAG "SHT3X"

#ifdef CONFIG_SHT3X_DEVICE_DEBUG_INFO
#define log_i(format...) ESP_LOGI(TAG, format)
#else
#define log_i(format...)
#endif

#ifdef CONFIG_SHT3X_DEVICE_DEBUG_ERROR
#define log_e(format...) ESP_LOGE(TAG, format)
#else
#define log_e(format...)
#endif

#ifdef CONFIG_SHT3X_DEVICE_DEBUG_REG
#define log_reg(buffer, buffer_len) ESP_LOG_BUFFER_HEX(TAG, buffer, buffer_len)
#else
#define log_reg(buffer, buffer_len)
#endif

typedef union {
    uint8_t data_buf[6];
    struct {
        struct {
            uint8_t msb;
            uint8_t lsb;
            uint8_t crc;
        } __attribute__ ((aligned (1))) temperature;
        struct {
            uint8_t msb;
            uint8_t lsb;
            uint8_t crc;
        } __attribute__ ((aligned (1))) humidity;
    } __attribute__ ((aligned (1)));
} sht3x_result_t;

static uint8_t crc_table[] = {
        0x00, 0x31, 0x62, 0x53, 0xc4, 0xf5, 0xa6, 0x97, 0xb9, 0x88, 0xdb, 0xea, 0x7d, 0x4c, 0x1f, 0x2e, 
        0x43, 0x72, 0x21, 0x10, 0x87, 0xb6, 0xe5, 0xd4, 0xfa, 0xcb, 0x98, 0xa9, 0x3e, 0x0f, 0x5c, 0x6d, 
        0x86, 0xb7, 0xe4, 0xd5, 0x42, 0x73, 0x20, 0x11, 0x3f, 0x0e, 0x5d, 0x6c, 0xfb, 0xca, 0x99, 0xa8, 
        0xc5, 0xf4, 0xa7, 0x96, 0x01, 0x30, 0x63, 0x52, 0x7c, 0x4d, 0x1e, 0x2f, 0xb8, 0x89, 0xda, 0xeb, 
        0x3d, 0x0c, 0x5f, 0x6e, 0xf9, 0xc8, 0x9b, 0xaa, 0x84, 0xb5, 0xe6, 0xd7, 0x40, 0x71, 0x22, 0x13, 
        0x7e, 0x4f, 0x1c, 0x2d, 0xba, 0x8b, 0xd8, 0xe9, 0xc7, 0xf6, 0xa5, 0x94, 0x03, 0x32, 0x61, 0x50, 
        0xbb, 0x8a, 0xd9, 0xe8, 0x7f, 0x4e, 0x1d, 0x2c, 0x02, 0x33, 0x60, 0x51, 0xc6, 0xf7, 0xa4, 0x95, 
        0xf8, 0xc9, 0x9a, 0xab, 0x3c, 0x0d, 0x5e, 0x6f, 0x41, 0x70, 0x23, 0x12, 0x85, 0xb4, 0xe7, 0xd6, 
        0x7a, 0x4b, 0x18, 0x29, 0xbe, 0x8f, 0xdc, 0xed, 0xc3, 0xf2, 0xa1, 0x90, 0x07, 0x36, 0x65, 0x54, 
        0x39, 0x08, 0x5b, 0x6a, 0xfd, 0xcc, 0x9f, 0xae, 0x80, 0xb1, 0xe2, 0xd3, 0x44, 0x75, 0x26, 0x17, 
        0xfc, 0xcd, 0x9e, 0xaf, 0x38, 0x09, 0x5a, 0x6b, 0x45, 0x74, 0x27, 0x16, 0x81, 0xb0, 0xe3, 0xd2, 
        0xbf, 0x8e, 0xdd, 0xec, 0x7b, 0x4a, 0x19, 0x28, 0x06, 0x37, 0x64, 0x55, 0xc2, 0xf3, 0xa0, 0x91, 
        0x47, 0x76, 0x25, 0x14, 0x83, 0xb2, 0xe1, 0xd0, 0xfe, 0xcf, 0x9c, 0xad, 0x3a, 0x0b, 0x58, 0x69, 
        0x04, 0x35, 0x66, 0x57, 0xc0, 0xf1, 0xa2, 0x93, 0xbd, 0x8c, 0xdf, 0xee, 0x79, 0x48, 0x1b, 0x2a, 
        0xc1, 0xf0, 0xa3, 0x92, 0x05, 0x34, 0x67, 0x56, 0x78, 0x49, 0x1a, 0x2b, 0xbc, 0x8d, 0xde, 0xef, 
        0x82, 0xb3, 0xe0, 0xd1, 0x46, 0x77, 0x24, 0x15, 0x3b, 0x0a, 0x59, 0x68, 0xff, 0xce, 0x9d, 0xac
    };

static uint8_t sht3x_calculate_crc(uint8_t msb, uint8_t lsb)
{
    uint8_t index = msb ^ 0xff;
    index = crc_table[index] ^ lsb;
    return crc_table[index];
}

static double sht3x_calculate_temperature(uint8_t msb, uint8_t lsb)
{
    uint16_t data = (((uint16_t)msb) << 8) | ((uint16_t)lsb);
    double temperature = (double)data * 175.0 / 65535.0 - 45;

    return temperature;
}

static double sht3x_calculate_humidity(uint8_t msb, uint8_t lsb)
{
    uint16_t data = (((uint16_t)msb) << 8) | ((uint16_t)lsb);
    double humidity = (double)data * 100.0 / 65535.0;

    return humidity;
}

sht3x_device_t * sht3x_init_device(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t freq, uint8_t device_addr)
{
    sht3x_device_t * device = (sht3x_device_t *)malloc(sizeof(sht3x_device_t));
    if (device == NULL) {
        log_e("Init SHT3X device failed, can not alloc memory");
        return NULL;
    }

    device->mode = SHT3X_MODE_SINGLE_SHOT;
    device->art_enabled = false;

    device->i2c_interface = i2c_malloc_device(i2c_num, sda, scl, freq, device_addr);
    
    if (device->i2c_interface != NULL) {
        log_i("New SHT3X device initialized");

        log_i("Reset sensor");
        sht3x_software_reset(device);

        return device;
    } else {
        log_e("New SHT3X device initialization failed");
        return NULL;
    }
}

void sht3x_deinit_device(sht3x_device_t * device)
{
    if (device == NULL) {
        log_e("Deinit SHT3X device with Invalid Parameter: device");
    } else {
        if (device->i2c_interface == NULL) {
            log_e("Deinit SHT3X device with Invalid Parameter: i2c_interface");
        } else {
            if (device->mode != SHT3X_MODE_SINGLE_SHOT) {
                sht3x_stop_periodic_measure(device->i2c_interface);
            }
            
            i2c_free_device(device->i2c_interface);
        }

        free(device);
    }
}

esp_err_t sht3x_software_reset(sht3x_device_t * device)
{
    esp_err_t return_value;

    if (device->mode != SHT3X_MODE_SINGLE_SHOT) {
        return_value = sht3x_stop_periodic_measure(device);

        if (return_value == ESP_OK) {
            device->mode = SHT3X_MODE_SINGLE_SHOT;
        }

        vTaskDelay(pdMS_TO_TICKS(1) + 1);
    }

    return_value = i2c_write_byte(device->i2c_interface, 0x30, 0xA2);

    if (return_value == ESP_OK && device->mode != SHT3X_MODE_SINGLE_SHOT) {
        device->mode = SHT3X_MODE_SINGLE_SHOT;
    }

    return return_value;
}

esp_err_t sht3x_do_single_shot_measure(sht3x_device_t * device, sht3x_clock_stretching_t clock_stretching, sht3x_repeatability_t repeatability, double * temperature, double * humidity)
{
    if (device->mode != SHT3X_MODE_SINGLE_SHOT) {
        log_i("sht3x_do_single_shot_measure mode mismatch");
        return SHT3X_ERROR_MODE_MISMATCH;
    }

    uint8_t cmd_msb = 0, cmd_lsb = 0;
    switch (clock_stretching) {
    case SHT3X_CLOCK_STRETCHING_ENABLED:
        cmd_msb = 0x2C;
        switch (repeatability) {
        case SHT3X_REPEATABILITY_HIGH:
            cmd_lsb = 0x06;
            break;
        case SHT3X_REPEATABILITY_MEDIUM:
            cmd_lsb = 0x0D;
            break;
        case SHT3X_REPEATABILITY_LOW:
            cmd_lsb = 0x10;
            break;
        default:
            log_e("sht3x_do_single_shot_measure invalid parameter");
        }
        break;
    case SHT3X_CLOCK_STRETCHING_DISABLED:
        cmd_msb = 0x24;
        switch (repeatability) {
        case SHT3X_REPEATABILITY_HIGH:
            cmd_lsb = 0x00;
            break;
        case SHT3X_REPEATABILITY_MEDIUM:
            cmd_lsb = 0x0B;
            break;
        case SHT3X_REPEATABILITY_LOW:
            cmd_lsb = 0x16;
            break;
        default:
            log_e("sht3x_do_single_shot_measure invalid parameter");
        }
        break;
    default:
        log_e("sht3x_do_single_shot_measure invalid parameter");
    }

    esp_err_t return_value = i2c_write_byte(device->i2c_interface, cmd_msb, cmd_lsb);

    if (return_value != ESP_OK) {
        log_e("sht3x_do_single_shot_measure->i2c_write_byte returns %d", return_value);
        return return_value;
    }

    if (clock_stretching == SHT3X_CLOCK_STRETCHING_DISABLED) {
        vTaskDelay(pdMS_TO_TICKS((repeatability == SHT3X_REPEATABILITY_LOW) ? 5 : (repeatability == SHT3X_REPEATABILITY_MEDIUM) ? 7 : 16) + 1);
    }

    sht3x_result_t result;
    return_value = i2c_read_bytes(device->i2c_interface, I2C_NO_REG, result.data_buf, 6);

    if (return_value == ESP_OK) {
        if (sht3x_calculate_crc(result.temperature.msb, result.temperature.lsb) != result.temperature.crc ||
            sht3x_calculate_crc(result.humidity.msb, result.humidity.lsb) != result.humidity.crc) {
                return_value = SHT3X_ERROR_CRC_FAIL;
        } else {
            if (temperature) {
                (*temperature) = sht3x_calculate_temperature(result.temperature.msb, result.temperature.lsb);
            }
            if (humidity) {
                (*humidity) = sht3x_calculate_humidity(result.humidity.msb, result.humidity.lsb);
            }
        }
    } else {
        log_e("sht3x_do_single_shot_measure->i2c_read_bytes returns %d", return_value);
    }

    return return_value;
}

esp_err_t sht3x_enable_art(sht3x_device_t * device)
{
    esp_err_t return_value = i2c_write_byte(device->i2c_interface, 0x2B, 0x32);

    if (return_value == ESP_OK) {
        device->art_enabled = false;
    } else {
        log_e("sht3x_enable_art->i2c_write_byte returns %d", return_value);
    }

    return return_value;
}

esp_err_t sht3x_start_periodic_measure(sht3x_device_t * device, sht3x_acquisition_frequency_t acquisition_frequency, sht3x_repeatability_t repeatability)
{
    if (device->mode != SHT3X_MODE_SINGLE_SHOT) {
        log_i("sht3x_start_periodic_measure mode mismatch");
        return SHT3X_ERROR_MODE_MISMATCH;
    }

    uint8_t cmd_msb = 0, cmd_lsb = 0;
    switch (acquisition_frequency) {
    case SHT3X_ACQUISITION_FREQUENCY_HALF:
        cmd_msb = 0x20;
        switch (repeatability) {
        case SHT3X_REPEATABILITY_HIGH:
            cmd_lsb = 0x32;
            break;
        case SHT3X_REPEATABILITY_MEDIUM:
            cmd_lsb = 0x24;
            break;
        case SHT3X_REPEATABILITY_LOW:
            cmd_lsb = 0x2F;
            break;
        default:
            log_e("sht3x_start_periodic_measure invalid parameter");
        }
        break;
    case SHT3X_ACQUISITION_FREQUENCY_ONE:
        cmd_msb = 0x21;
        switch (repeatability) {
        case SHT3X_REPEATABILITY_HIGH:
            cmd_lsb = 0x30;
            break;
        case SHT3X_REPEATABILITY_MEDIUM:
            cmd_lsb = 0x26;
            break;
        case SHT3X_REPEATABILITY_LOW:
            cmd_lsb = 0x2D;
            break;
        default:
            log_e("sht3x_start_periodic_measure invalid parameter");
        }
        break;
    case SHT3X_ACQUISITION_FREQUENCY_TWO:
        cmd_msb = 0x22;
        switch (repeatability) {
        case SHT3X_REPEATABILITY_HIGH:
            cmd_lsb = 0x36;
            break;
        case SHT3X_REPEATABILITY_MEDIUM:
            cmd_lsb = 0x20;
            break;
        case SHT3X_REPEATABILITY_LOW:
            cmd_lsb = 0x2B;
            break;
        default:
            log_e("sht3x_start_periodic_measure invalid parameter");
        }
        break;
    case SHT3X_ACQUISITION_FREQUENCY_FOUR:
        cmd_msb = 0x23;
        switch (repeatability) {
        case SHT3X_REPEATABILITY_HIGH:
            cmd_lsb = 0x34;
            break;
        case SHT3X_REPEATABILITY_MEDIUM:
            cmd_lsb = 0x22;
            break;
        case SHT3X_REPEATABILITY_LOW:
            cmd_lsb = 0x29;
            break;
        default:
            log_e("sht3x_start_periodic_measure invalid parameter");
        }
        break;
    case SHT3X_ACQUISITION_FREQUENCY_TEN:
        cmd_msb = 0x27;
        switch (repeatability) {
        case SHT3X_REPEATABILITY_HIGH:
            cmd_lsb = 0x37;
            break;
        case SHT3X_REPEATABILITY_MEDIUM:
            cmd_lsb = 0x21;
            break;
        case SHT3X_REPEATABILITY_LOW:
            cmd_lsb = 0x2A;
            break;
        default:
            log_e("sht3x_start_periodic_measure invalid parameter");
        }
        break;
    default:
        log_e("sht3x_start_periodic_measure invalid parameter");
    }

    esp_err_t return_value = i2c_write_byte(device->i2c_interface, cmd_msb, cmd_lsb);

    if (return_value == ESP_OK) {
        device->mode = SHT3X_MODE_PERIODIC;
    } else {
        log_e("sht3x_start_periodic_measure->i2c_write_byte returns %d", return_value);
    }

    return return_value;
}

esp_err_t sht3x_stop_periodic_measure(sht3x_device_t * device)
{
    if (device->mode == SHT3X_MODE_SINGLE_SHOT) {
        log_i("sht3x_start_periodic_measure mode mismatch");
        return SHT3X_ERROR_MODE_MISMATCH;
    }

    esp_err_t return_value = i2c_write_byte(device->i2c_interface, 0x30, 0x93);

    if (return_value == ESP_OK) {
        device->mode = SHT3X_MODE_SINGLE_SHOT;
    } else {
        log_e("sht3x_stop_periodic_measure->i2c_write_byte returns %d", return_value);
    }

    return return_value;
}

esp_err_t sht3x_fetch_result(sht3x_device_t * device, double * temperature, double * humidity)
{
    esp_err_t return_value = i2c_write_byte(device->i2c_interface, 0xE0, 0x00);

    if (return_value != ESP_OK) {
        log_e("sht3x_fetch_result->i2c_write_byte returns %d", return_value);
        return return_value;
    }

    sht3x_result_t result;
    return_value = i2c_read_bytes(device->i2c_interface, I2C_NO_REG, result.data_buf, 6);

    if (return_value == ESP_OK) {
        if (sht3x_calculate_crc(result.temperature.msb, result.temperature.lsb) != result.temperature.crc ||
            sht3x_calculate_crc(result.humidity.msb, result.humidity.lsb) != result.humidity.crc) {
                log_e("cale value:%x, %x", sht3x_calculate_crc(result.temperature.msb, result.temperature.lsb), result.temperature.crc);
                return_value = SHT3X_ERROR_CRC_FAIL;
        } else {
            if (temperature) {
                (*temperature) = sht3x_calculate_temperature(result.temperature.msb, result.temperature.lsb);
            }
            if (humidity) {
                (*humidity) = sht3x_calculate_humidity(result.humidity.msb, result.humidity.lsb);
            }
        }
    } else {
        log_e("sht3x_fetch_result->i2c_read_bytes returns %d", return_value);
    }

    return return_value;
}

esp_err_t sht3x_enable_heater(sht3x_device_t * device, bool enable)
{
    return ESP_OK;
}

esp_err_t sht3x_read_heater_status(sht3x_device_t * device, uint16_t status)
{
    return ESP_OK;
}

esp_err_t sht3x_clear_heater_status(sht3x_device_t * device)
{
    return ESP_OK;
}
