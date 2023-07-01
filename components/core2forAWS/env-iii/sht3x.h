#pragma once

#include "core2forAWS.h"
#include "i2c_device.h"

#define ESP32_I2C_STANDARD_FREQUENCY        (100000)
#define ESP32_I2C_HIGH_FREQUENCY            (400000)
#define ESP32_I2C_HIGHEST_FREQUENCY         (5000000)
#define SHT3X_I2C_HIGHEST_FREQUENCY         (1000000)

#define SHT3X_I2C_ADDRESS_PIN2_LOW          (0x44)
#define SHT3X_I2C_ADDRESS_PIN2_HIGH         (0x45)

#define SHT3X_ERROR_MODE_MISMATCH           (0x10000001)
#define SHT3X_ERROR_CRC_FAIL                (0x10000002)

typedef enum {
    SHT3X_MODE_SINGLE_SHOT,
    SHT3X_MODE_PERIODIC,
} sht3x_mode_t;

typedef enum {
    SHT3X_CLOCK_STRETCHING_ENABLED,
    SHT3X_CLOCK_STRETCHING_DISABLED,
} sht3x_clock_stretching_t;

typedef enum {
    SHT3X_REPEATABILITY_LOW,
    SHT3X_REPEATABILITY_MEDIUM,
    SHT3X_REPEATABILITY_HIGH,
} sht3x_repeatability_t;

typedef enum {
    SHT3X_ACQUISITION_FREQUENCY_HALF,
    SHT3X_ACQUISITION_FREQUENCY_ONE,
    SHT3X_ACQUISITION_FREQUENCY_TWO,
    SHT3X_ACQUISITION_FREQUENCY_FOUR,
    SHT3X_ACQUISITION_FREQUENCY_TEN,
} sht3x_acquisition_frequency_t;

typedef struct {
    I2CDevice_t i2c_interface;
    sht3x_mode_t mode;
    bool art_enabled;
} sht3x_device_t;
 
sht3x_device_t * sht3x_init_device(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t freq, uint8_t device_addr);
void sht3x_deinit_device(sht3x_device_t * device);

esp_err_t sht3x_software_reset(sht3x_device_t * device);

esp_err_t sht3x_do_single_shot_measure(sht3x_device_t * device, sht3x_clock_stretching_t clock_stretching, sht3x_repeatability_t repeatability, double * temperature, double * humidity);

esp_err_t sht3x_enable_art(sht3x_device_t * device);
esp_err_t sht3x_start_periodic_measure(sht3x_device_t * device, sht3x_acquisition_frequency_t acquisition_frequency, sht3x_repeatability_t repeatability);
esp_err_t sht3x_stop_periodic_measure(sht3x_device_t * device);
esp_err_t sht3x_fetch_result(sht3x_device_t * device, double * temperature, double * humidity);

esp_err_t sht3x_enable_heater(sht3x_device_t * device, bool enable);
esp_err_t sht3x_read_heater_status(sht3x_device_t * device, uint16_t status);
esp_err_t sht3x_clear_heater_status(sht3x_device_t * device);
