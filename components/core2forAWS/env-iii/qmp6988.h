#pragma once

#include "core2forAWS.h"
#include "i2c_device.h"

#define QMP6988_I2C_STANDARD_FREQUENCY          (100000)
#define QMP6988_I2C_FAST_FREQUENCY              (400000)
#define QMP6988_I2C_HIGHEST_FREQUENCY           (3400000)

#define QMP6988_I2C_ADDRESS_SDO_LOW             (0x70)
#define QMP6988_I2C_ADDRESS_SDO_HIGH            (0x56)

/* Compensation coefficients registers */
#define QMP6988_COMPENSATION_COES_START         (0xA0) /* QMP6988 compensation coefficients */
#define QMP6988_COMPENSATION_COES_LENGTH        (25)

typedef union {
    uint8_t data_array[QMP6988_COMPENSATION_COES_LENGTH];
    struct {
        uint8_t b00_1;
        uint8_t b00_0;
        uint8_t bt1_1;
        uint8_t bt1_0;
        uint8_t bt2_1;
        uint8_t bt2_0;
        uint8_t bp1_1;
        uint8_t bp1_0;
        uint8_t b11_1;
        uint8_t b11_0;
        uint8_t bp2_1;
        uint8_t bp2_0;
        uint8_t b12_1;
        uint8_t b12_0;
        uint8_t b21_1;
        uint8_t b21_0;
        uint8_t bp3_1;
        uint8_t bp3_0;
        uint8_t a0_1;
        uint8_t a0_0;
        uint8_t a1_1;
        uint8_t a1_0;
        uint8_t a2_1;
        uint8_t a2_0;
        struct {
            uint8_t a0_ex : 4;
            uint8_t b00_ex : 4;
        };
    };
} compensation_coefficients_registers_t;

typedef struct {
    double a0;
    double a1;
    double a2;
    double b00;
    double bt1;
    double bt2;
    double bp1;
    double b11;
    double bp2;
    double b12;
    double b21;
    double bp3;
} compensation_coefficients_t;

/* Chip ID register & value */
#define QMP6988_REGISTER_CHIP_ID                (0xD1)
#define QMP6988_CHIP_ID                         (0x5C)

/* Reset register & command */
#define QMP6988_REGISTER_RESET                  (0xE0) /* Device reset register */
#define QMP6988_RESET_COMMAND                   (0xE6)

/* IIR(Infinite Impulse Response) filter coefficient setting register*/
#define QMP6988_REGISTER_IIR_FILTER             (0xF1)

#define QMP6988_IIR_RESPONSE_DEPTH_OFF          (0x00)
#define QMP6988_IIR_RESPONSE_DEPTH_02           (0x01)
#define QMP6988_IIR_RESPONSE_DEPTH_04           (0x02)
#define QMP6988_IIR_RESPONSE_DEPTH_08           (0x03)
#define QMP6988_IIR_RESPONSE_DEPTH_16           (0x04)
#define QMP6988_IIR_RESPONSE_DEPTH_32           (0x05)

typedef union {
    uint8_t data;
    struct {
        uint8_t response_depth : 3;
        uint8_t : 5;
    };
} iir_filter_register_t;

/* I2C setting register */
#define QMP6988_REGISTER_I2C_SETTING            (0xF2)

#define QMP6998_I2C_MASTER_CODE_08              (0x00)
#define QMP6998_I2C_MASTER_CODE_09              (0x01)
#define QMP6998_I2C_MASTER_CODE_0A              (0x02)
#define QMP6998_I2C_MASTER_CODE_0B              (0x03)
#define QMP6998_I2C_MASTER_CODE_0C              (0x04)
#define QMP6998_I2C_MASTER_CODE_0D              (0x05)
#define QMP6998_I2C_MASTER_CODE_0E              (0x06)
#define QMP6998_I2C_MASTER_CODE_0F              (0x07)

typedef union {
    uint8_t data;
    struct {
        uint8_t master_code : 3;
        uint8_t : 5;
    };
} i2c_setting_register_t;

/* Device status register */
#define QMP6988_REGISTER_DEVICE_STATUS          (0xF3) /* Device status register */

#define QMP6988_DEVICE_STATUS_READY             (0x00)
#define QMP6988_DEVICE_STATUS_MEASURING         (0x10)

typedef union {
    uint8_t data;
    struct {
        uint8_t otp_updating : 1;
        uint8_t : 2;
        uint8_t measuring : 1;
        uint8_t : 4;
    };
} device_status_register_t;

/* Measurement control register*/
#define QMP6988_REGISTER_MEASUREMENT_CONTROL    (0xF4) /* Measurement Condition Control Register */

#define QMP6988_POWER_MODE_SLEEP                (0x00)
#define QMP6988_POWER_MODE_FORCED               (0x01)
#define QMP6988_POWER_MODE_NORMAL               (0x03)

#define QMP6988_OVERSAMPLING_COUNT_SKIPPED      (0x00)
#define QMP6988_OVERSAMPLING_COUNT_01           (0x01)
#define QMP6988_OVERSAMPLING_COUNT_02           (0x02)
#define QMP6988_OVERSAMPLING_COUNT_04           (0x03)
#define QMP6988_OVERSAMPLING_COUNT_08           (0x04)
#define QMP6988_OVERSAMPLING_COUNT_16           (0x05)
#define QMP6988_OVERSAMPLING_COUNT_32           (0x06)
#define QMP6988_OVERSAMPLING_COUNT_64           (0x07)

typedef union {
    uint8_t data;
    struct {
        uint8_t power_mode : 2;
        uint8_t pressure_oversamping : 3;
        uint8_t temperature_oversampling : 3;
    };
} measurement_control_register_t;

/* IO setup register */
#define QMP6988_REGISTER_IO_SETUP               (0xF5)

#define QMP6988_SPI_3WIRE                       (0x00)
#define QMP6988_SPI_4WIRE                       (0x01)

#define QMP6988_SPI_3WIRE_SDI_OUTPUT_LOHIZ      (0x00)
#define QMP6988_SPI_3WIRE_SDI_OUTPUT_LOHI       (0x01)

#define QMP6998_MEASUREMENT_STANDBY_1MS         (0x00)
#define QMP6998_MEASUREMENT_STANDBY_5MS         (0x01)
#define QMP6998_MEASUREMENT_STANDBY_50MS        (0x02)
#define QMP6998_MEASUREMENT_STANDBY_250MS       (0x03)
#define QMP6998_MEASUREMENT_STANDBY_500MS       (0x04)
#define QMP6998_MEASUREMENT_STANDBY_1000MS      (0x05)
#define QMP6998_MEASUREMENT_STANDBY_2000MS      (0x06)
#define QMP6998_MEASUREMENT_STANDBY_4000MS      (0x07)

typedef union {
    uint8_t data;
    struct {
        uint8_t spi_wire_number : 1;
        uint8_t : 1;
        uint8_t spi_3wire_sdi_output_mode : 1;
        uint8_t : 2;
        uint8_t standby_time : 3;
    };
} io_setup_register_t;

/* Data registers */
#define QMP6988_REGISTER_RESULT_START           (0xF7)
#define QMP6988_REGISTER_RESULT_LENGTH          (6)
#define QMP6988_RESULT_ADJUSTMENT               ((uint32_t)1 << 23)

typedef union {
    uint8_t data_array[QMP6988_REGISTER_RESULT_LENGTH];
    struct {
        uint8_t p_txd2;
        uint8_t p_txd1;
        uint8_t p_txd0;
        uint8_t t_txd2;
        uint8_t t_txd1;
        uint8_t t_txd0;
    };
} measurement_result_registers_t;

typedef struct {
    I2CDevice_t i2c_interface;
    compensation_coefficients_t coes;
} qmp6988_device_t;

qmp6988_device_t * qmp6988_init_device(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t freq, uint8_t device_addr);
void qmp6988_deinit_device(qmp6988_device_t * device);

esp_err_t qmp6988_software_reset(qmp6988_device_t * device);
esp_err_t qmp6988_check_chip_id(qmp6988_device_t * device);
esp_err_t qmp6988_get_status(qmp6988_device_t * device, uint8_t * status);
esp_err_t qmp6988_get_compensation_coefficients(qmp6988_device_t * device);

esp_err_t qmp6988_set_standby(qmp6988_device_t * device, uint8_t standby_time);
esp_err_t qmp6988_set_oversampling(qmp6988_device_t * device, uint8_t temperature_oversamping, uint8_t pressure_oversampling);
esp_err_t qmp6988_set_master_code(qmp6988_device_t * device, uint8_t master_code);
esp_err_t qmp6988_set_iir_response_depth(qmp6988_device_t * device, uint8_t response_depth);

esp_err_t qmp6988_do_single_shot_measure(qmp6988_device_t * device, double * temperature, double * pressure);
esp_err_t qmp6988_start_periodic_measure(qmp6988_device_t * device);
esp_err_t qmp6988_stop_periodic_measure(qmp6988_device_t * device);
esp_err_t qmp6988_fetch_result(qmp6988_device_t * device, double * temperature, double * pressure);
