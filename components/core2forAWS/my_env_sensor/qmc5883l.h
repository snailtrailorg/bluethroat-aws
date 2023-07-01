#pragma once

#include "core2forAWS.h"
#include "i2c_device.h"

typedef struct {
    I2CDevice_t i2c_interface;
} qmc5883l_device_t;

qmc5883l_device_t * qmc5883l_init_device(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t freq, uint8_t device_addr);
esp_err_t qmc5883l_get_status(qmc5883l_device_t * device, uint8_t * status);
esp_err_t qmc5883l_fetch_result(qmc5883l_device_t * device, double * x, double * y, double * z);

