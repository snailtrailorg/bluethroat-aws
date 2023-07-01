#pragma once

#include "core2forAWS.h"
#include "i2c_device.h"

#define DPS310_SDOPIN_PULLDOWN

#ifdef DPS310_SDOPIN_PULLDOWN
#define DPS310_I2C_SLAVE_ADDR          0x76
#else
#define DPS310_I2C_SLAVE_ADDR          0x77
#endif

#define DPS310_REG_PSR_B2              0x00
#define DPS310_REG_PSR_B1              0x01
#define DPS310_REG_PSR_B0              0x02

#define DPS310_REG_TMP_B2              0x03
#define DPS310_REG_TMP_B1              0x04
#define DPS310_REG_TMP_B0              0x05

#define DPS310_REG_PRS_CFG             0x06
#define DPS310_REG_TMP_CFG             0x07
#define DPS310_REG_MEAS_CFG            0x08
#define DPS310_REG_CFG_REG             0x09
#define DPS310_REG_INT_STS             0x0A
#define DPS310_REG_FIFO_STS            0x0B
#define DPS310_REG_RESET               0x0C
#define DPS310_REG_PRO_ID              0x0D

#define DPS310_REG_COEF_C0M            0x10
#define DPS310_REG_COEF_C0L_C1M        0x11
#define DPS310_REG_COEF_C1L            0x12
#define DPS310_REG_COEF_C00U           0x13
#define DPS310_REG_COEF_C00M           0x14
#define DPS310_REG_COEF_C00L_C10U      0x15
#define DPS310_REG_COEF_C10M           0x16
#define DPS310_REG_COEF_C10L           0x17
#define DPS310_REG_COEF_C01M           0x18
#define DPS310_REG_COEF_C01L           0x19
#define DPS310_REG_COEF_C11M           0x1A
#define DPS310_REG_COEF_C11L           0x1B
#define DPS310_REG_COEF_C20M           0x1C
#define DPS310_REG_COEF_C20L           0x1D
#define DPS310_REG_COEF_C21M           0x1E
#define DPS310_REG_COEF_C21L           0x1F
#define DPS310_REG_COEF_C30M           0x20
#define DPS310_REG_COEF_C30L           0x21

#define DPS310_REG_COEF_SRCE           0x28

// Pressure measurement rate
#define PRS_CFG_PM_RATE_1                 0x00
#define PRS_CFG_PM_RATE_2                 0x10
#define PRS_CFG_PM_RATE_4                 0x20
#define PRS_CFG_PM_RATE_8                 0x30
#define PRS_CFG_PM_RATE_16                0x40
#define PRS_CFG_PM_RATE_32                0x50
#define PRS_CFG_PM_RATE_64                0x60
#define PRS_CFG_PM_RATE_128               0x70

// Pressure oversampling rate
#define PRS_CFG_PM_PRC_1                  0x00
#define PRS_CFG_PM_PRC_2                  0x01
#define PRS_CFG_PM_PRC_4                  0x02
#define PRS_CFG_PM_PRC_8                  0x03
#define PRS_CFG_PM_PRC_16                 0x04
#define PRS_CFG_PM_PRC_32                 0x05
#define PRS_CFG_PM_PRC_64                 0x06
#define PRS_CFG_PM_PRC_128                0x07

// Temperature measurement source
#define TMP_CFG_TMP_EXT_ASIC              0x00
#define TMP_CFG_TMP_EXT_MEMS              0x80
#define TMP_CFG_TMP_EXT_MASK              0x80

// Temperature measurement rate
#define TMP_CFG_TMP_RATE_1                0x00
#define TMP_CFG_TMP_RATE_2                0x10
#define TMP_CFG_TMP_RATE_4                0x20
#define TMP_CFG_TMP_RATE_8                0x30
#define TMP_CFG_TMP_RATE_16               0x40
#define TMP_CFG_TMP_RATE_32               0x50
#define TMP_CFG_TMP_RATE_64               0x60
#define TMP_CFG_TMP_RATE_128              0x70

// Temperature oversampling (precision)
#define TMP_CFG_TMP_PRC_1                 0x00
#define TMP_CFG_TMP_PRC_2                 0x01
#define TMP_CFG_TMP_PRC_4                 0x02
#define TMP_CFG_TMP_PRC_8                 0x03
#define TMP_CFG_TMP_PRC_16                0x04
#define TMP_CFG_TMP_PRC_32                0x05
#define TMP_CFG_TMP_PRC_64                0x06
#define TMP_CFG_TMP_PRC_128               0x07

// Sensor Operating Status
#define MEAS_CFG_COEF_RDY                 0x80
#define MEAS_CFG_SENSOR_RDY               0x40
#define MEAS_CFG_TMP_RDY                  0x20
#define MEAS_CFG_PRS_RDY                  0x10
#define MEAS_CFG_PRS_RDY_MASK             0x10

// Sensor Operating Mode
#define MEAS_CFG_MEAS_CTRL_STOP           0x00
#define MEAS_CFG_MEAS_CTRL_PRS            0x01
#define MEAS_CFG_MEAS_CTRL_TMP            0x02
#define MEAS_CFG_MEAS_CTRL_BACKGROUND     0x04

// Interrupt and FIFO configuration
#define CFG_REG_INT_HL                    0x80
#define CFG_REG_INT_FIFO                  0x40
#define CFG_REG_INT_TMP                   0x20
#define CFG_REG_INT_PRS                   0x10
#define CFG_REG_T_SHIFT                   0x08
#define CFG_REG_P_SHIFT                   0x04
#define CFG_REG_FIFO_EN                   0x02
#define CFG_REG_SPI_MODE_3WIRE            0x01

// Interrupt status
#define INT_STS_FIFO_FULL                 0x04
#define INT_STS_TMP_READY                 0x02
#define INT_STS_PRS_READY                 0x01

// FIFO status
#define FIFO_STS_FIFO_FULL                0x02
#define FIFO_STS_FIFO_EMPTY               0x01

// Flush FIFO or generate software reset
#define RESET_FIFO_FLUSH_CM               0x80
#define RESET_RESET_CMD                   0x05

// Temperature Coeicients Source
#define TMP_COEF_SRCE_ASIA                0x00
#define TMP_COEF_SRCE_MEMS                0x80
#define TMP_COEF_SRCE_MASK                0x80

// Chip and revision ID
#define CHIP_AND_REVISION_ID              0x10

//Compensation Scale Factors
#define SCALE_FACTOR_PRC_1                524288
#define SCALE_FACTOR_PRC_2                1572864
#define SCALE_FACTOR_PRC_4                3670016
#define SCALE_FACTOR_PRC_8                7864320
#define SCALE_FACTOR_PRC_16               253952
#define SCALE_FACTOR_PRC_32               516096
#define SCALE_FACTOR_PRC_64               1040384
#define SCALE_FACTOR_PRC_128              2088960

typedef struct {
    double c0;
    double c1;
    double c00;
    double c10;
    double c01;
    double c11;
    double c20;
    double c21;
    double c30;
} dps310_compensation_coefficients_t;

typedef struct {
    double psf;
    double tsf;
} dps310_scale_factors_t;

typedef struct {
    I2CDevice_t i2c_interface;
    dps310_compensation_coefficients_t coes;
    dps310_scale_factors_t sf;
} dps310_device_t;

/*****************************************************************************
 * Barometer Init.
 * Don't use FIFO, must work in background mode
 * set default pressure measurement rate, pressure oversampling rate,
 * temperature measurement rate, temperature oversampling rate.
 *****************************************************************************/
dps310_device_t * dps310_init_device(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t freq, uint8_t device_addr);
void dps310_deinit_device(dps310_device_t * device);
esp_err_t dps310_check_chip_id(dps310_device_t * device);
esp_err_t dps310_get_compensation_coefficients(dps310_device_t * device);
esp_err_t dps310_get_status(dps310_device_t * device, uint8_t * status);

/**************************************************************************//**
 * Get measurement result.
 *****************************************************************************/
esp_err_t dps310_fetch_result(dps310_device_t * device, double * temperature, double * pressure);

/**************************************************************************//**
 * Calculate altitude by pressure and temperature
 *****************************************************************************/
double dps310_calculate_altitude(double reference_pressure, double pressure, double temperature);
