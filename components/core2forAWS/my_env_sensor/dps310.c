#include "esp_log.h"
#include "esp_err.h"
#include <math.h>

#include "dps310.h"

#define TAG "DPS310"

#ifdef CONFIG_DPS310_DEVICE_DEBUG_INFO
#define log_i(format...) ESP_LOGI(TAG, format)
#else
#define log_i(format...)
#endif

#ifdef CONFIG_DPS310_DEVICE_DEBUG_ERROR
#define log_e(format...) ESP_LOGE(TAG, format)
#else
#define log_e(format...)
#endif

#ifdef CONFIG_DPS310_DEVICE_DEBUG_REG
#define log_reg(buffer, buffer_len) ESP_LOG_BUFFER_HEX(TAG, buffer, buffer_len)
#else
#define log_reg(buffer, buffer_len)
#endif

dps310_device_t * dps310_init_device(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t freq, uint8_t device_addr) {
    dps310_device_t * device = (dps310_device_t *)malloc(sizeof(dps310_device_t));
    if (device == NULL) {
        log_e("Init DSP310 device failed, can not alloc memory");
        return NULL;
    }

    device->i2c_interface = i2c_malloc_device(i2c_num, sda, scl, freq, device_addr);
    
    if (device->i2c_interface != NULL) {
        log_i("New DSP310 device initialized");

        uint8_t meas_cfg = 0;
        esp_err_t return_value;
        while ((return_value = i2c_read_byte(device->i2c_interface, DPS310_REG_MEAS_CFG, &meas_cfg)) != ESP_OK ||
            MEAS_CFG_SENSOR_RDY != (meas_cfg & MEAS_CFG_SENSOR_RDY) ||
            MEAS_CFG_COEF_RDY != (meas_cfg & MEAS_CFG_COEF_RDY)) {
            if (return_value != ESP_OK) {
                log_i("Read chip status failed");
                i2c_free_device(device->i2c_interface);
                free(device);
                return NULL;
            } else {
                log_i("Waiting for chip status ready");
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        };

        if (ESP_OK != dps310_check_chip_id(device)) {
            log_i("Check chip id failed");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

        log_i("Get compensation coefficients");
        return_value = dps310_get_compensation_coefficients(device);
        if (return_value != ESP_OK) {
            log_e("Get compensation coefficients failed");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

        device->sf.psf = SCALE_FACTOR_PRC_64;
        return_value = i2c_write_byte(device->i2c_interface, DPS310_REG_PRS_CFG, PRS_CFG_PM_RATE_8 | PRS_CFG_PM_PRC_64);
        if (return_value != ESP_OK) {
            log_e("dps310_init_device->i2c_write_byte faild");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

        uint8_t temperature_source = 0;
        return_value = i2c_read_byte(device->i2c_interface, DPS310_REG_COEF_SRCE, &temperature_source);
        if (return_value != ESP_OK) {
            log_e("dps310_init_device->i2c_read_bytes DPS310_REG_COEF_SRCE faild");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }
        temperature_source &= TMP_CFG_TMP_EXT_MASK;

        device->sf.tsf = SCALE_FACTOR_PRC_32;
        return_value = i2c_write_byte(device->i2c_interface, DPS310_REG_TMP_CFG, temperature_source | TMP_CFG_TMP_RATE_1 | TMP_CFG_TMP_PRC_32);
        if (return_value != ESP_OK) {
            log_e("dps310_init_device->i2c_write_byte DPS310_REG_TMP_CFG faild");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

        return_value = i2c_write_byte(device->i2c_interface, DPS310_REG_CFG_REG, CFG_REG_P_SHIFT | CFG_REG_T_SHIFT);
        if (return_value != ESP_OK) {
            log_e("dps310_init_device->i2c_write_byte DPS310_REG_CFG_REG faild");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

        return_value = i2c_write_byte(device->i2c_interface, DPS310_REG_MEAS_CFG, MEAS_CFG_MEAS_CTRL_PRS | MEAS_CFG_MEAS_CTRL_TMP | MEAS_CFG_MEAS_CTRL_BACKGROUND);
        if (return_value != ESP_OK) {
            log_e("dps310_init_device->i2c_write_byte DPS310_REG_MEAS_CFG faild");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

    } else {
        log_e("New DSP310 device initialization failed");
        free(device);
        return NULL;
    }

    return device;
}

void dps310_deinit_device(dps310_device_t * device) {
    if (device == NULL) {
        log_e("Deinit DSP310 device with Invalid Parameter: device");
    } else {
        if (device->i2c_interface == NULL) {
            log_e("Deinit DSP310 device with Invalid Parameter: i2c_interface");
        } else {
            i2c_free_device(device->i2c_interface);
        }

        free(device);
    }
}

esp_err_t dps310_software_reset(dps310_device_t * device) {
    esp_err_t return_value = i2c_write_byte(device->i2c_interface, 0x0c, 0x09);

    if (return_value != ESP_OK) {
        log_e("dps310_software_reset->i2c_write_byte faild");
    }

    return return_value;
}

esp_err_t dps310_check_chip_id(dps310_device_t * device) {
    uint8_t chip_id = 0;

    esp_err_t return_value = i2c_read_byte(device->i2c_interface, DPS310_REG_PRO_ID, &chip_id);

    if (return_value != ESP_OK) {
        log_e("dps310_check_chip_id->i2c_read_byte faild");
        return return_value;
    }

    if (chip_id != CHIP_AND_REVISION_ID) {
        log_e("dps310_check_chip_id, invalid id: 0x%2.2X", chip_id);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t dps310_get_status(dps310_device_t * device, uint8_t * status) {
    esp_err_t return_value = i2c_read_byte(device->i2c_interface, DPS310_REG_MEAS_CFG, status);

    if (return_value != ESP_OK) {
        log_e("dps310_get_status->i2c_read_byte faild");
    }

    return return_value;
}

esp_err_t dps310_get_compensation_coefficients(dps310_device_t * device) {
    uint8_t coef[18];
    int32_t coef_c0, coef_c1, coef_c00, coef_c10, coef_c01, coef_c11, coef_c20, coef_c21, coef_c30;

    esp_err_t return_value = i2c_read_bytes(device->i2c_interface, DPS310_REG_COEF_C0M, coef, 18);
    if (return_value != ESP_OK) {
        log_e("dps310_get_compensation_coefficients->i2c_read_bytes faild");
        return return_value;
    }

    coef_c0 = (coef[0] << 4) + ((coef[1] >> 4) & 0x0f);
    if (coef_c0 & 0x000000800) coef_c0 -= 0x00001000;
    device->coes.c0 = (double)coef_c0;

    coef_c1 = ((coef[1] & 0x0f) << 8) + coef[2];
    if (coef_c1 > 0x000000800) coef_c1 -= 0x00001000;
    device->coes.c1 = (double)coef_c1;

    coef_c00 = (coef[3] << 12) + (coef[4] << 4) + ((coef[5] >> 4) & 0x0f);
    if (coef_c00 & 0x00080000) coef_c00 -= 0x00100000;
    device->coes.c00 = (double)coef_c00;

    coef_c10 = ((coef[5] & 0x0f) << 16) + (coef[6] << 8) + coef[7];
    if (coef_c10 & 0x00080000) coef_c10 -= 0x00100000;
    device->coes.c10 = (double)coef_c10;

    coef_c01 = (coef[8] << 8) + coef[9];
    if (coef_c01 & 0x00008000) coef_c01 -= 0x00010000;
    device->coes.c01 = (double)coef_c01;

    coef_c11 = (coef[10] << 8) + coef[11];
    if (coef_c11 & 0x00008000) coef_c11 -= 0x00010000;
    device->coes.c11 = (double)coef_c11;

    coef_c20 = (coef[12] << 8) + coef[13];
    if (coef_c20 & 0x00008000) coef_c20 -= 0x00010000;
    device->coes.c20 = (double)coef_c20;

    coef_c21 = (coef[14] << 8) + coef[15];
    if (coef_c21 & 0x00008000) coef_c21 -= 0x00010000;
    device->coes.c21 = (double)coef_c21;

    coef_c30 = (coef[16] << 8) + coef[17];
    if (coef_c30 & 0x00008000) coef_c30 -= 0x00010000;
    device->coes.c30 = (double)coef_c30;

    ESP_LOG_BUFFER_HEX("DPS310", coef, 18);
    ESP_LOGE("DPS310", "c0: 0x%8.8X, c1: 0x%8.8X, c00: 0x%8.8X, c10: 0x%8.8X, c01: 0x%8.8X, c11: 0x%8.8X, c20: 0x%8.8X, c21: 0x%8.8X, c30: 0x%8.8X", coef_c0, coef_c1, coef_c00, coef_c10, coef_c01, coef_c11, coef_c20, coef_c21, coef_c30);
    ESP_LOGE("DPS310", "c0: %d, c1: %d, c00: %d, c10: %d, c01: %d, c11: %d, c20: %d, c21: %d, c30: %d", coef_c0, coef_c1, coef_c00, coef_c10, coef_c01, coef_c11, coef_c20, coef_c21, coef_c30);

    return ESP_OK;
}

esp_err_t dps310_fetch_result(dps310_device_t * device, double * temperature, double * pressure) {
    uint8_t result_reg[6];
    esp_err_t return_value = i2c_read_bytes(device->i2c_interface, DPS310_REG_PSR_B2, result_reg, 6);

    ESP_LOG_BUFFER_HEX("DPS310", result_reg, 6);

    if (return_value == ESP_OK)
    {
        int32_t raw_temperature = (result_reg[3] << 16) + (result_reg[4] << 8) + result_reg[5];
        if (raw_temperature > 0x007fffff) raw_temperature -= 0x01000000;
        int32_t raw_pressure = (result_reg[0] << 16) + (result_reg[1] << 8) + result_reg[2];
        if (raw_pressure > 0x007fffff) raw_pressure -= 0x01000000;

        ESP_LOGE("DPS310", "raw_temperature: 0x%8.8X, raw_pressure: 0x%8.8X", raw_temperature, raw_pressure);

        double scaled_temperature = (double)raw_temperature / device->sf.tsf;
        double scaled_pressure = (double)raw_pressure / device->sf.psf;

        ESP_LOGE("DPS310", "scaled_temperature: %f, scaled_pressure: %f", scaled_temperature, scaled_pressure);
        
        double compensated_temperature = 0.5 * device->coes.c0 + scaled_temperature * device->coes.c1;
        double compensated_pressure = device->coes.c00 + scaled_pressure * (device->coes.c10 + scaled_pressure *(device->coes.c20 + scaled_pressure * device->coes.c30)) +
            scaled_temperature * device->coes.c01 + scaled_temperature * scaled_pressure * (device->coes.c11 + scaled_pressure * device->coes.c21);

        *temperature = compensated_temperature;
        *pressure = compensated_pressure;
    }

    ESP_LOGE("DPS310", "temperature: %f, pressure: %f", *temperature, *pressure);

    return return_value;
}

double dps310_calculate_altitude(double reference_pressure, double pressure, double temperature) {
  return (pow(reference_pressure / pressure, 1 / 5.257) - 1) * (temperature + 273.15) / 0.0065;
}
