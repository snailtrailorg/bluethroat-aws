#include "esp_log.h"
#include "esp_err.h"

#include "qmp6988.h"

#define TAG "QMP6988"

#ifdef CONFIG_QMP6988_DEVICE_DEBUG_INFO
#define log_i(format...) ESP_LOGI(TAG, format)
#else
#define log_i(format...)
#endif

#ifdef CONFIG_QMP6988_DEVICE_DEBUG_ERROR
#define log_e(format...) ESP_LOGE(TAG, format)
#else
#define log_e(format...)
#endif

#ifdef CONFIG_QMP6988_DEVICE_DEBUG_REG
#define log_reg(buffer, buffer_len) ESP_LOG_BUFFER_HEX(TAG, buffer, buffer_len)
#else
#define log_reg(buffer, buffer_len)
#endif

qmp6988_device_t * qmp6988_init_device(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t freq, uint8_t device_addr) {
    qmp6988_device_t * device = (qmp6988_device_t *)malloc(sizeof(qmp6988_device_t));
    if (device == NULL) {
        log_e("Init QMP6988 device failed, can not alloc memory");
        return NULL;
    }

    device->i2c_interface = i2c_malloc_device(i2c_num, sda, scl, freq, device_addr);
    
    if (device->i2c_interface != NULL) {
        log_i("New QMP6988 device initialized");

        if (ESP_OK != qmp6988_check_chip_id(device)) {
            log_i("Check chip id failed");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        }

        log_i("Get compensation coefficients");
        esp_err_t ret_value = qmp6988_get_compensation_coefficients(device);
        if (ret_value != ESP_OK) {
            log_e("Get compensation coefficients failed");
            i2c_free_device(device->i2c_interface);
            free(device);
            return NULL;
        } else {
          return device;
        }
    } else {
        log_e("New QMP6988 device initialization failed");
        free(device);
        return NULL;
    }
}

void qmp6988_deinit_device(qmp6988_device_t * device) {
    if (device == NULL) {
        log_e("Deinit QMP6988 device with Invalid Parameter: device");
    } else {
        if (device->i2c_interface == NULL) {
            log_e("Deinit QMP6988 device with Invalid Parameter: i2c_interface");
        } else {
            i2c_free_device(device->i2c_interface);
        }

        free(device);
    }
}

esp_err_t qmp6988_software_reset(qmp6988_device_t * device) {
    esp_err_t return_value = i2c_write_byte(device->i2c_interface, QMP6988_REGISTER_RESET, QMP6988_RESET_COMMAND);

    if (return_value != ESP_OK) {
        log_e("qmp6988_software_reset->i2c_write_byte faild");
    }

    return return_value;
}

esp_err_t qmp6988_check_chip_id(qmp6988_device_t * device) {
    uint8_t chip_id = 0;

    esp_err_t return_value = i2c_read_byte(device->i2c_interface, QMP6988_REGISTER_CHIP_ID, &chip_id);

    if (return_value != ESP_OK) {
        log_e("qmp6988_check_chip_id->i2c_read_byte faild");
        return return_value;
    }

    if (chip_id != QMP6988_CHIP_ID) {
        log_e("qmp6988_check_chip_id, invalid id: 0x%2.2X", chip_id);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t qmp6988_get_status(qmp6988_device_t * device, uint8_t * status) {
    esp_err_t return_value = i2c_read_byte(device->i2c_interface, QMP6988_REGISTER_DEVICE_STATUS, status);

    if (return_value != ESP_OK) {
        log_e("qmp6988_get_status->i2c_read_byte faild");
    }

    return return_value;
}

esp_err_t qmp6988_get_compensation_coefficients(qmp6988_device_t * device) {
    compensation_coefficients_registers_t coe_regs;

    esp_err_t return_value = i2c_read_bytes(device->i2c_interface, QMP6988_COMPENSATION_COES_START, coe_regs.data_array, QMP6988_COMPENSATION_COES_LENGTH);

    if (return_value != ESP_OK) {
        log_e("qmp6988_get_compensation_coefficients->i2c_read_bytes faild");
        return return_value;
    }

    device->coes.a0  = 1.0 * (((int32_t)((coe_regs.a0_1  << 24) | (coe_regs.a0_0  << 16) | (coe_regs.a0_ex  << 12))) >> 12) / 16.0f;
    device->coes.b00 = 1.0 * (((int32_t)((coe_regs.b00_1 << 24) | (coe_regs.b00_0 << 16) | (coe_regs.b00_ex << 12))) >> 12) / 16.0f;

    device->coes.a1  = -6.30e-03f + (+4.30e-04f) * ((int16_t)((coe_regs.a1_1  << 8) | coe_regs.a1_0 )) / 32767.0f;
    device->coes.a2  = -1.90e-11f + (+1.20e-10f) * ((int16_t)((coe_regs.a2_1  << 8) | coe_regs.a2_0 )) / 32767.0f;
    device->coes.bt1 = +1.00e-01f + (+9.10e-02f) * ((int16_t)((coe_regs.bt1_1 << 8) | coe_regs.bt1_0)) / 32767.0f;
    device->coes.bt2 = +1.20e-08f + (+1.20e-06f) * ((int16_t)((coe_regs.bt2_1 << 8) | coe_regs.bt2_0)) / 32767.0f;
    device->coes.bp1 = +3.30E-02f + (+1.90e-02f) * ((int16_t)((coe_regs.bp1_1 << 8) | coe_regs.bp1_0)) / 32767.0f;
    device->coes.b11 = +2.10e-07f + (+1.40e-07f) * ((int16_t)((coe_regs.b11_1 << 8) | coe_regs.b11_0)) / 32767.0f;
    device->coes.bp2 = -6.30e-10f + (+3.50e-10f) * ((int16_t)((coe_regs.bp2_1 << 8) | coe_regs.bp2_0)) / 32767.0f;
    device->coes.b12 = +2.90e-13f + (+7.60e-13f) * ((int16_t)((coe_regs.b12_1 << 8) | coe_regs.b12_0)) / 32767.0f;
    device->coes.b21 = +2.10e-15f + (+1.20e-14f) * ((int16_t)((coe_regs.b21_1 << 8) | coe_regs.b21_0)) / 32767.0f;
    device->coes.bp3 = +1.30e-16f + (+7.90e-17f) * ((int16_t)((coe_regs.bp3_1 << 8) | coe_regs.bp3_0)) / 32767.0f;

    log_i("compensation coefficients registers:");
    log_reg(coe_regs.data_array, QMP6988_COMPENSATION_COES_LENGTH);
    log_i("coes.a0: %e, coes.a1: %e, coes.a2: %e", device->coes.a0, device->coes.a1, device->coes.a2);
    log_i("device->coes.b00 \t%e\n, device->coes.bt1 \t%e\n, device->coes.bt2 \t%e\n, device->coes.bp1 \t%e\n, device->coes.b11 \t%e\n, device->coes.bp2 \t%e\n, device->coes.b12 \t%e\n, device->coes.b21 \t%e\n, device->coes.bp3 \t%e\n", 
    device->coes.b00,
    device->coes.bt1,
    device->coes.bt2,
    device->coes.bp1,
    device->coes.b11,
    device->coes.bp2,
    device->coes.b12,
    device->coes.b21,
    device->coes.bp3);

    return ESP_OK;
}

esp_err_t qmp6988_set_standby(qmp6988_device_t * device, uint8_t standby_time) {
    io_setup_register_t io_setup_reg;

    esp_err_t return_value = i2c_read_byte(device->i2c_interface, QMP6988_REGISTER_IO_SETUP, &(io_setup_reg.data));

    if (return_value != ESP_OK) {
        log_e("qmp6988_get_compensation_coefficients->i2c_read_bytes faild");
        return return_value;
    }

    io_setup_reg.standby_time = standby_time;

    return_value = i2c_write_byte(device->i2c_interface, QMP6988_REGISTER_IO_SETUP, io_setup_reg.data);

    if (return_value != ESP_OK) {
        log_e("qmp6988_get_compensation_coefficients->i2c_write_byte faild");
    }

    return return_value;
}

esp_err_t qmp6988_set_oversampling(qmp6988_device_t * device, uint8_t temperature_oversamping, uint8_t pressure_oversampling) {
    measurement_control_register_t control_reg;

    esp_err_t return_value = i2c_read_byte(device->i2c_interface, QMP6988_REGISTER_MEASUREMENT_CONTROL, &(control_reg.data));

    if (return_value != ESP_OK) {
        log_e("qmp6988_set_oversampling->i2c_read_bytes faild");
        return return_value;
    }

    control_reg.pressure_oversamping = pressure_oversampling;
    control_reg.temperature_oversampling = temperature_oversamping;

    return_value = i2c_write_byte(device->i2c_interface, QMP6988_REGISTER_MEASUREMENT_CONTROL, control_reg.data);

    if (return_value != ESP_OK) {
        log_e("qmp6988_set_oversampling->i2c_write_byte faild");
    }

    return return_value;
}

esp_err_t qmp6988_set_master_code(qmp6988_device_t * device, uint8_t master_code) {
    i2c_setting_register_t i2c_setting_reg;

    esp_err_t return_value = i2c_read_byte(device->i2c_interface, QMP6988_REGISTER_I2C_SETTING, &(i2c_setting_reg.data));

    if (return_value != ESP_OK) {
        log_e("qmp6988_set_master_code->i2c_read_bytes faild");
        return return_value;
    }

    i2c_setting_reg.master_code = master_code;

    return_value = i2c_write_byte(device->i2c_interface, QMP6988_REGISTER_I2C_SETTING, i2c_setting_reg.data);

    if (return_value != ESP_OK) {
        log_e("qmp6988_set_master_code->i2c_write_byte faild");
    }

    return return_value;
}

esp_err_t qmp6988_set_iir_response_depth(qmp6988_device_t * device, uint8_t response_depth) {
    iir_filter_register_t iir_filter_reg;

    esp_err_t return_value = i2c_read_byte(device->i2c_interface, QMP6988_REGISTER_IIR_FILTER, &(iir_filter_reg.data));

    if (return_value != ESP_OK) {
        log_e("qmp6988_set_iir_response_depth->i2c_read_bytes faild");
        return return_value;
    }

    iir_filter_reg.response_depth = response_depth;

    return_value = i2c_write_byte(device->i2c_interface, QMP6988_REGISTER_IIR_FILTER, iir_filter_reg.data);

    if (return_value != ESP_OK) {
        log_e("qmp6988_set_iir_response_depth->i2c_write_byte faild");
    }

    return return_value;
}

static esp_err_t qmp6988_set_power_mode(qmp6988_device_t * device, uint8_t power_mode) {
    measurement_control_register_t control_reg;

    esp_err_t return_value = i2c_read_byte(device->i2c_interface, QMP6988_REGISTER_MEASUREMENT_CONTROL, &(control_reg.data));

    if (return_value != ESP_OK) {
        log_e("qmp6988_set_power_mode->i2c_read_bytes faild");
        return return_value;
    }

    control_reg.power_mode = power_mode;

    return_value = i2c_write_byte(device->i2c_interface, QMP6988_REGISTER_MEASUREMENT_CONTROL, control_reg.data);

    if (return_value != ESP_OK) {
        log_e("qmp6988_set_power_mode->i2c_write_byte faild");
    }

    return return_value;
}

esp_err_t qmp6988_do_single_shot_measure(qmp6988_device_t * device, double * temperature, double * pressure) {
    esp_err_t return_value = qmp6988_set_power_mode(device, QMP6988_POWER_MODE_FORCED);

    if (return_value != ESP_OK) {
        log_e("qmp6988_do_single_shot_measure->qmp6988_set_power_mode faild");
        return return_value;
    }

    return qmp6988_fetch_result(device, temperature, pressure);
}

esp_err_t qmp6988_start_periodic_measure(qmp6988_device_t * device) {
    esp_err_t return_value = qmp6988_set_power_mode(device, QMP6988_POWER_MODE_NORMAL);

    if (return_value != ESP_OK) {
        log_e("qmp6988_start_periodic_measure->qmp6988_set_power_mode faild");
        return return_value;
    }

    return return_value;
}

esp_err_t qmp6988_stop_periodic_measure(qmp6988_device_t * device) {
    esp_err_t return_value = qmp6988_set_power_mode(device, QMP6988_POWER_MODE_SLEEP);

    if (return_value != ESP_OK) {
        log_e("qmp6988_stop_periodic_measure->qmp6988_set_power_mode faild");
        return return_value;
    }

    return return_value;
}

esp_err_t qmp6988_fetch_result(qmp6988_device_t * device, double * temperature, double * pressure) {
    measurement_result_registers_t otps;

    esp_err_t return_value = i2c_read_bytes(device->i2c_interface, QMP6988_REGISTER_RESULT_START, otps.data_array, QMP6988_REGISTER_RESULT_LENGTH);

    if (return_value != ESP_OK) {
        log_e("qmp6988_fetch_result->i2c_read_bytes faild");
    } else {
        int32_t t_raw = (((uint32_t)otps.t_txd2 << 16) | ((uint32_t)otps.t_txd1 << 8) | (uint32_t)otps.t_txd0) - QMP6988_RESULT_ADJUSTMENT;
        int32_t p_raw = (((uint32_t)otps.p_txd2 << 16) | ((uint32_t)otps.p_txd1 << 8) | (uint32_t)otps.p_txd0) - QMP6988_RESULT_ADJUSTMENT;

        //log_i("t_raw: %d, p_raw: %d", t_raw, p_raw);

        double t_res = device->coes.a0 + device->coes.a1 * t_raw + device->coes.a2 * t_raw * t_raw;

        double p_res =
            device->coes.b00 +
            device->coes.bt1 * t_res +
            device->coes.bp1 * p_raw +
            device->coes.b11 * t_res * p_raw +
            device->coes.bt2 * t_res * t_res +
            device->coes.bp2 * p_raw * p_raw +
            device->coes.b12 * p_raw * t_res * t_res +
            device->coes.b21 * p_raw * p_raw * t_res +
            device->coes.bp3 * p_raw * p_raw * p_raw ;

        log_i("t_res: %f, p_res: %f", t_res, p_res);

        if (temperature) {
            (*temperature) = t_res / 256.0;
        }
        if (pressure) {
            *pressure = p_res;
        }
    }

    return return_value;
}
