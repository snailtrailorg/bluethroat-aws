#include "qmc5883l.h"

qmc5883l_device_t * qmc5883l_init_device(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t freq, uint8_t device_addr) {
    qmc5883l_device_t * device = (qmc5883l_device_t *)malloc(sizeof(qmc5883l_device_t));
    if (device == NULL) {
        ESP_LOGE("QMC5883L", "Init QMC5883L device failed, can not alloc memory");
        return NULL;
    }

    device->i2c_interface = i2c_malloc_device(i2c_num, sda, scl, freq, device_addr);
    
    if (device->i2c_interface != NULL) {
        ESP_LOGI("QMC5883L", "New QMC5883L device initialized");
        esp_err_t return_value = i2c_write_byte(device->i2c_interface, 0x0b, 0x01);
        if (return_value != ESP_OK) {
            ESP_LOGE("QMC5883L", "qmc5883l_init_device->i2c_write_byte 0x0b faild");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

        return_value = i2c_write_byte(device->i2c_interface, 0x09, 0x1d);
        if (return_value != ESP_OK) {
            ESP_LOGE("QMC5883L", "qmc5883l_init_device->i2c_write_byte 0x09 faild");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

    } else {
        ESP_LOGE("QMC5883L", "New QMC5883L device initialization failed");
        free(device);
        return NULL;
    }

    return device;
}

esp_err_t qmc5883l_get_status(qmc5883l_device_t * device, uint8_t * status) {
    esp_err_t return_value = i2c_read_byte(device->i2c_interface, 0x06, status);

    if (return_value != ESP_OK) {
        ESP_LOGE("QMC5883L", "qmc5883l_get_status->i2c_read_byte 0x06 faild");
    }

    return return_value;
}

esp_err_t qmc5883l_fetch_result(qmc5883l_device_t * device, double * x, double * y, double * z) {
    uint8_t result[6];

    esp_err_t return_value = i2c_read_bytes(device->i2c_interface, 0x00, result, 6);
    if (return_value == ESP_OK) {

        int16_t x_raw = ((result[1]) << 8) | result[0];
        int16_t y_raw = ((result[3]) << 8) | result[2];
        int16_t z_raw = ((result[5]) << 8) | result[4];

        *x = (double)x_raw - 13839.5;
        *y = (double)y_raw + 9852.5;
        *z = (double)z_raw + 2532.0;

        //ESP_LOG_BUFFER_HEX("QMC5883L", result, 6);
        //ESP_LOGI("QMC5883L", "detect geomagnetic(x/y/z:), %d, %d, %d", x_raw, y_raw, z_raw);
    }

    return return_value;
}
