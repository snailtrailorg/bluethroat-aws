#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "core2forAWS.h"
#include "bluetooth.h"
#include "speaker.h"
#include "qmp6988.h"
#include "sht3x.h"
#include "sin_table.h"
#include "vario.h"
#include "esp_log.h"
#include "esp_err.h"
#include "screen.h"
#include "fir_filter.h"
#include "config.h"
#include "dps310.h"
#include "qmc5883l.h"

#define TAG "VARIO"

#ifdef CONFIG_VARIO_DEVICE_DEBUG_INFO
#define log_i(format...) ESP_LOGI(TAG, format)
#else
#define log_i(format...)
#endif

#ifdef CONFIG_VARIO_DEVICE_DEBUG_ERROR
#define log_e(format...) ESP_LOGE(TAG, format)
#else
#define log_e(format...)
#endif

#ifdef CONFIG_VARIO_DEVICE_DEBUG_REG
#define log_reg(buffer, buffer_len) ESP_LOG_BUFFER_HEX(TAG, buffer, buffer_len)
#else
#define log_reg(buffer, buffer_len)
#endif

static SemaphoreHandle_t vario_speed_mutex = NULL;
static int32_t vario_speed = 0;

void vario_set_speed(int32_t speed /*cm per second*/) {
    //log_i("vario_set_speed: %d", speed);
    xSemaphoreTake(vario_speed_mutex, portMAX_DELAY);
    vario_speed = speed;
    xSemaphoreGive(vario_speed_mutex);
}

int32_t vario_get_speed(void) {
    xSemaphoreTake(vario_speed_mutex, portMAX_DELAY);
    uint32_t speed = vario_speed;
    xSemaphoreGive(vario_speed_mutex);

    return speed;
}

static int16_t * sound_buffer = NULL;
static fir_filter_t * filter = NULL;

static speaker_device_t * speaker = NULL;
static TaskHandle_t speaker_task_handle = NULL;

static qmp6988_device_t * qmp6988 = NULL;
static TaskHandle_t qmp6988_task_handle = NULL;

static dps310_device_t * dps310 = NULL;
static TaskHandle_t dps310_task_handle = NULL;

static sht3x_device_t * sht3x = NULL;
static TaskHandle_t sht3x_task_handle = NULL;

static qmc5883l_device_t * qmc5883l = NULL;
static TaskHandle_t qmc5883l_task_handle = NULL;

//static uint8_t uart_buffer[UART_RX_BUF_SIZE+1];

void vario_start(void) {
    if (vario_speed_mutex == NULL) {
        vario_speed_mutex = xSemaphoreCreateMutex();
    }

    sound_buffer = malloc(sizeof(int16_t) * OVERALL_TONE_SAMPLE_RATE * OVERALL_TONE_LIFT_CYCLE_MAXIMUM / 1000);


    /*
        During the initialization of the I2S interface, several DMA buffers are allocated.
        It seems that the initialization and write of these buffers must be within one task,
        otherwise an error will occur and the system will crash.
    */
    //int32_t sampling_rate = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_SAMPLING_RATE);
    //int32_t sampling_bitwidth = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_SAMPLING_BITWIDTH);
    //speaker = Speaker_Init(I2S_NUM_0, GPIO_NUM_12, GPIO_NUM_0, GPIO_NUM_2, GPIO_NUM_34, sampling_rate, sampling_bit_depth);

    xTaskCreate(vario_speaker_loop, "SpeakerTask", 16384, NULL, tskIDLE_PRIORITY+3 , &speaker_task_handle);

    int32_t speed_altitude_window = config_get_integer(CONFIG_NAMESPACE_SPEED, CONFIG_SPEED_ALTITUDE_WINDOW);
    filter = init_fir_filter(speed_altitude_window, 0);

    qmp6988 = qmp6988_init_device(I2C_NUM_0, GPIO_NUM_32, GPIO_NUM_33, QMP6988_I2C_FAST_FREQUENCY, QMP6988_I2C_ADDRESS_SDO_LOW);
    if (qmp6988 != NULL) {
        //qmp6988_check_chip_id(qmp6988);
        qmp6988_set_standby(qmp6988, QMP6998_MEASUREMENT_STANDBY_5MS);
        qmp6988_set_oversampling(qmp6988, QMP6988_OVERSAMPLING_COUNT_04, QMP6988_OVERSAMPLING_COUNT_32);
        //qmp6988_set_iir_response_depth(qmp6988, QMP6988_IIR_RESPONSE_DEPTH_OFF);
        qmp6988_start_periodic_measure(qmp6988);
        xTaskCreate(vario_qmp6988_loop, "Qmp6998Task", 8192, NULL, tskIDLE_PRIORITY+5, &qmp6988_task_handle);
    }

//    dps310 = dps310_init_device(I2C_NUM_0, GPIO_NUM_32, GPIO_NUM_33, QMP6988_I2C_FAST_FREQUENCY, DPS310_I2C_SLAVE_ADDR);
    dps310 = dps310_init_device(I2C_NUM_1, GPIO_NUM_21, GPIO_NUM_22, QMP6988_I2C_FAST_FREQUENCY, 0x76);
    if (dps310 != NULL) {
        xTaskCreate(vario_dps310_loop, "Dps310Task", 8192, NULL, tskIDLE_PRIORITY+5, &dps310_task_handle);
    }

    qmc5883l = qmc5883l_init_device(I2C_NUM_0, GPIO_NUM_32, GPIO_NUM_33, QMP6988_I2C_FAST_FREQUENCY, 0x0d);
    if (qmc5883l != NULL) {
        xTaskCreate(vario_qmc5883l_loop, "QMC5883Task", 8192, NULL, tskIDLE_PRIORITY+5, &qmc5883l_task_handle);
    } else {
        ESP_LOGE("QMC5883L", "qmc5883l_init_device failed");
    }

    sht3x = sht3x_init_device(I2C_NUM_0, GPIO_NUM_32, GPIO_NUM_33, ESP32_I2C_HIGH_FREQUENCY, SHT3X_I2C_ADDRESS_PIN2_LOW);
    if (sht3x != NULL) {
        sht3x_start_periodic_measure(sht3x, SHT3X_ACQUISITION_FREQUENCY_TWO, SHT3X_REPEATABILITY_HIGH);
        sht3x_enable_art(sht3x);
        xTaskCreate(vario_sht3x_loop, "Sht3xTask", 8192, NULL, tskIDLE_PRIORITY+2, &sht3x_task_handle);
    }
}

void vario_stop(void) {
    if (sht3x_task_handle != NULL) {
        vTaskDelete(sht3x_task_handle);
        sht3x_task_handle = NULL;
    }

    if (sht3x != NULL) {
        sht3x_stop_periodic_measure(sht3x);
        sht3x_deinit_device(sht3x);
        sht3x = NULL;
    }

    if (qmp6988_task_handle != NULL) {
        vTaskDelete(qmp6988_task_handle);
        qmp6988_task_handle = NULL;
    }

    if (qmp6988 != NULL) {
        qmp6988_stop_periodic_measure(qmp6988);
        qmp6988_deinit_device(qmp6988);
        qmp6988 = NULL;
    }

    if (filter != NULL) {
        deinit_fir_filter(filter);
        filter = NULL;
    }
    
    if (speaker_task_handle != NULL) {
        vTaskDelete(speaker_task_handle);
        speaker_task_handle = NULL;
    }

    if (speaker != NULL) {
        Speaker_Deinit(speaker);
        speaker = NULL;
    }

    if (sound_buffer) {
        free(sound_buffer);
        sound_buffer = NULL;
    }
}

void vario_dps310_loop(void * arguments) {

    for ( ; ; ) {
        int32_t time_window = config_get_integer(CONFIG_NAMESPACE_SPEED, CONFIG_SPEED_ALTITUDE_WINDOW);

        uint8_t dps310_status;
        if (ESP_OK == dps310_get_status(dps310, &dps310_status)) {
            if ((dps310_status & MEAS_CFG_PRS_RDY_MASK) == MEAS_CFG_PRS_RDY) {
                double temperature;
                double pressure;
                if (ESP_OK == dps310_fetch_result(dps310, &temperature, &pressure)) {
                    static TickType_t last_ticks = 0;
                    TickType_t ticks = xTaskGetTickCount();
                    uint32_t current_delta_time = (pdTICKS_TO_MS(ticks - last_ticks));

                    static int32_t last_average_delta_time = 125;
                    int32_t average_delta_time = last_average_delta_time * (time_window - 1) / time_window + current_delta_time / time_window;

                    static double last_average_altitude = 0.0;
                    double current_altitude = 100000.0 * (pow(101325.0 / pressure, 1 / 5.257) - 1) * (temperature + 273.15) / 0.0065;
                    double average_altitude = fir_filter_process(filter, current_altitude);

                    int32_t speed = (average_altitude - last_average_altitude) / (double)average_delta_time;

                    //log_i("current_delta_time:%d, average_delta_time:%d, last_average_delta_time:%d, current altitude:%f, average altitude:%f, last average altitude:%f, speed:%d", current_delta_time, average_delta_time, last_average_delta_time, current_altitude, average_altitude, last_average_altitude, speed);

                    last_ticks = ticks;
                    last_average_delta_time = average_delta_time;
                    last_average_altitude = average_altitude;

                    vario_set_speed(speed);

                    ui_set_altitude(current_altitude / 100000.0);
                    ui_set_speed((double)speed / 100.0);
                    ui_set_pressure(pressure);
                    bluetooth_send_pressure((uint32_t)pressure);

                    int32_t temperature_adjustment = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_TEMPERATURE_ADJUSTMENT);
                    ui_set_temperature(temperature + (double)temperature_adjustment / 1000.0);
                } else {
                    log_e("Read dps310 error");
                }
            }
        } else {
            log_e("vario_dps310_loop->dps310_get_status failed");
        }

        uint8_t *data = heap_caps_malloc(UART_RX_BUF_SIZE+1, MALLOC_CAP_SPIRAM);
        if (data) {
            int data_len = Core2ForAWS_Port_C_UART_Receive(data);
            if (data_len > 0) {
                data[(data_len < UART_RX_BUF_SIZE) ? data_len : UART_RX_BUF_SIZE] = 0;
                log_e("%s", (char*)data);
            }
            free(data);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vario_qmp6988_loop(void * arguments) {
    //bluethroat_parameters_t bps;

    for ( ; ; ) {
        int32_t time_window = config_get_integer(CONFIG_NAMESPACE_SPEED, CONFIG_SPEED_ALTITUDE_WINDOW);

        static uint8_t last_qmp6988_status = QMP6988_DEVICE_STATUS_MEASURING;
        uint8_t qmp6988_status;
        if (ESP_OK == qmp6988_get_status(qmp6988, &qmp6988_status)) {
            if (qmp6988_status == QMP6988_DEVICE_STATUS_READY && last_qmp6988_status != QMP6988_DEVICE_STATUS_READY) {
                double temperature;
                double pressure;
                if (ESP_OK == qmp6988_fetch_result(qmp6988, &temperature, &pressure)) {
                    static TickType_t last_ticks = 0;
                    TickType_t ticks = xTaskGetTickCount();
                    uint32_t current_delta_time = (pdTICKS_TO_MS(ticks - last_ticks));

                    static int32_t last_average_delta_time = 80;
                    int32_t average_delta_time = last_average_delta_time * (time_window - 1) / time_window + current_delta_time / time_window;

                    static double last_average_altitude = 0.0;
                    double current_altitude = 100000.0 * (pow(101325.0 / pressure, 1 / 5.257) - 1) * (temperature + 273.15) / 0.0065;
                    double average_altitude = fir_filter_process(filter, current_altitude);

                    int32_t speed = (average_altitude - last_average_altitude) / (double)average_delta_time;

                    //log_i("current_delta_time:%d, average_delta_time:%d, last_average_delta_time:%d, current altitude:%f, average altitude:%f, last average altitude:%f, speed:%d", current_delta_time, average_delta_time, last_average_delta_time, current_altitude, average_altitude, last_average_altitude, speed);

                    last_ticks = ticks;
                    last_average_delta_time = average_delta_time;
                    last_average_altitude = average_altitude;

                    vario_set_speed(speed);

                    ui_set_altitude(current_altitude / 100000.0);
                    ui_set_speed((double)speed / 100.0);
                    ui_set_pressure(pressure);
                    bluetooth_send_pressure((uint32_t)pressure);

                    int32_t temperature_adjustment = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_TEMPERATURE_ADJUSTMENT);
                    ui_set_temperature(temperature + (double)temperature_adjustment / 1000.0);
                } else {
                    log_e("Read qmp6988 error");
                }
            }
            last_qmp6988_status = qmp6988_status;
        } else {
            log_e("vario_qmp6988_loop->qmp6988_get_status failed");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vario_sht3x_loop(void * arguments) {
    for ( ; ; ) {
        double temperature = 0.0f;
        double humidity = 0.0f;

        esp_err_t ret = sht3x_fetch_result(sht3x, &temperature, &humidity);
        if (ret == ESP_OK) {
            ui_set_humidity(humidity);
        } else {
            log_i("vario_sht3x_loop->sht3x_fetch_result failed, return %x", ret);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vario_speaker_loop(void * arguemnts) {
    //bluethroat_parameters_t bps;
/*
    uint32_t lift_freq;
    uint32_t lift_freq_phase = 0;
    uint32_t lift_cycle;
    uint32_t lift_duty;

    uint32_t sink_basefreq;
    uint32_t sink_harmonic;
    uint32_t sink_cycle;
    uint32_t sink_basefreq_phase = 0;
    uint32_t sink_harmonic_phase = 0;
*/

    int32_t sampling_rate = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_SAMPLING_RATE);
    int32_t sampling_bitwidth = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_SAMPLING_BITWIDTH);
    speaker = Speaker_Init(I2S_NUM_0, GPIO_NUM_12, GPIO_NUM_0, GPIO_NUM_2, GPIO_NUM_34, sampling_rate, sampling_bitwidth);

    for ( ; ; ) {
        int32_t lift_freq_min = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_FREQUENCY_MINIMUM);
        int32_t lift_freq_max = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_FREQUENCY_MAXIMUM);
        int32_t lift_freq_factor = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_FREQUENCY_FACTOR);
        int32_t lift_cycle_min = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_CYCLE_MINIMUM);
        int32_t lift_cycle_max = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_CYCLE_MAXIMUM);
        int32_t lift_cycle_factor = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_CYCLE_FACTOR);
        int32_t lift_duty_min = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_DUTY_MINMUM);
        int32_t lift_duty_max = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_DUTY_MAXMUM);
        int32_t lift_duty_factor = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_LIFT_DUTY_FACTOR);
        int32_t sink_freq_min = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_SINK_FREQUENCY_MINIMUM);
        int32_t sink_freq_max = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_SINK_FREQUENCY_MAXIMUM);
        int32_t sink_freq_factor = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_SINK_FREQUENCY_FACTOR);
        int32_t sink_diff_percent = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_SINK_DUAL_TONE_FACTOR);
        int32_t lift_start = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_THRESHOLD_LIFT_START_SPEED);
        int32_t lift_stop = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_THRESHOLD_LIFT_STOP_SPEED);
        int32_t sink_start = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_THRESHOLD_SINK_START_SPEED);
        int32_t sink_stop = config_get_integer(CONFIG_NAMESPACE_SOUND, CONFIG_SOUND_THRESHOLD_SINK_STOP_SPEED);
        int32_t volume = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_VOLUME);
        int32_t auto_poweroff_timeout = config_get_integer(CONFIG_NAMESPACE_SYSTEM, CONFIG_SYSTEM_AUTO_POWEROFF_TIMEOUT);
    
        static int32_t last_speed = 0;
        int32_t speed = (vario_get_speed() & 0xfffffff0);
    
        uint32_t data_length = 0;

        //log_i("current speed is: %d, last speed is: %d", speed, last_speed);
        typedef enum { VARIO_STATUS_LIFTING, VARIO_STATUS_GLIDING, VARIO_STATUS_SINKING } vario_status_t;
        static vario_status_t status = VARIO_STATUS_GLIDING;
        if (speed != last_speed) {
            if (status == VARIO_STATUS_GLIDING) {
                if (speed >= lift_start) {
                    status = VARIO_STATUS_LIFTING;
                } else if (speed <= sink_start) {
                    status = VARIO_STATUS_SINKING;
                }
            } else if (status == VARIO_STATUS_LIFTING) {
                if (speed <= sink_start) {
                    status = VARIO_STATUS_SINKING;
                } else if (speed < lift_stop) {
                    status = VARIO_STATUS_GLIDING;
                }
            } else {
                if (speed >= lift_start) {
                    status = VARIO_STATUS_LIFTING;
                } else if (speed > sink_stop) {
                    status = VARIO_STATUS_GLIDING;
                }
            }

            if (status == VARIO_STATUS_LIFTING) {
                int32_t lift_freq = lift_freq_max - (lift_freq_max - lift_freq_min) * lift_freq_factor / (speed + lift_freq_factor);
                int32_t lift_cycle = lift_cycle_min + (lift_cycle_max - lift_cycle_min) * lift_cycle_factor / (speed + lift_cycle_factor);
                int32_t lift_duty = lift_duty_min + (lift_duty_max - lift_duty_min) * lift_duty_factor / (speed + lift_duty_factor);

                lift_freq = lift_freq * SIN_TABLE_DATA_COUNT / sampling_rate;
                lift_cycle = lift_cycle * sampling_rate / 1000;
                lift_duty = lift_duty * sampling_rate / 1000;

                register_t i = 0;
                static int32_t lift_freq_phase = 0;

                for ( ; i<lift_duty; i++) {
                    lift_freq_phase = (lift_freq_phase + lift_freq) % SIN_TABLE_DATA_COUNT;
                    sound_buffer[i] = volume * sin_table[lift_freq_phase] / 100;
                }

                for ( ; i<lift_cycle; i++) {
                    sound_buffer[i] = 0;
                }

                data_length = lift_cycle;

            } else if (status == VARIO_STATUS_SINKING) {
                int32_t sink_basefreq = sink_freq_min + (sink_freq_max - sink_freq_min) * sink_freq_factor / (sink_freq_factor - speed);
                int32_t sink_harmonic = sink_basefreq + sink_basefreq * sink_diff_percent / 100;

                sink_basefreq = sink_basefreq * SIN_TABLE_DATA_COUNT / sampling_rate;
                sink_harmonic = sink_harmonic * SIN_TABLE_DATA_COUNT / sampling_rate;
                int32_t sink_cycle = 500 * sampling_rate / 1000;

                static int32_t sink_basefreq_phase = 0;
                static int32_t sink_harmonic_phase = 0;

                for (register_t i = 0; i<sink_cycle; i++) {
                    sink_basefreq_phase = (sink_basefreq_phase + sink_basefreq) % SIN_TABLE_DATA_COUNT;
                    sink_harmonic_phase = (sink_basefreq_phase + sink_harmonic) % SIN_TABLE_DATA_COUNT;
                    sound_buffer[i] = volume * (sin_table[sink_basefreq_phase] / 2 + sin_table[sink_harmonic_phase] / 2) / 100;
                }

                data_length = sink_cycle;
            }

            last_speed = speed;
        }

        typedef enum { VARIO_SOUND_STATE_OFF, VARIO_SOUND_STATE_ON } sound_state_t;
        static sound_state_t sound_state = VARIO_SOUND_STATE_OFF;

        static TickType_t last_ticks = 0;
        TickType_t ticks = xTaskGetTickCount();

        if (status != VARIO_STATUS_GLIDING) {
            last_ticks = ticks;
            if (sound_state == VARIO_SOUND_STATE_OFF) {
                Core2ForAWS_Speaker_Enable(1);
                sound_state = VARIO_SOUND_STATE_ON;
            }
            //log_i("write data begin for status: %d, speed: %d, length: %d", status, speed, data_length);
            Speaker_WriteBuff(speaker, (uint8_t *)sound_buffer, data_length * sizeof(uint16_t), portMAX_DELAY);
            //log_i("write data end for status: %d, speed: %d, length: %d", status, speed, data_length);
        } else {
            if (pdTICKS_TO_MS(ticks - last_ticks) > auto_poweroff_timeout) {
                ESP_LOGI("VARIO", "Scheduled power off after %dms", auto_poweroff_timeout);
                Axp192_PowerOff();
            } else if (pdTICKS_TO_MS(ticks - last_ticks) > 10000) {
                if (sound_state == VARIO_SOUND_STATE_ON) {
                    Core2ForAWS_Speaker_Enable(0);
                    sound_state = VARIO_SOUND_STATE_OFF;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vario_qmc5883l_loop(void * arguments) {
    #define _PI_ (3.1415926535897932384626);
    for ( ; ; ) {
        double x, y, z;
        if (ESP_OK == qmc5883l_fetch_result(qmc5883l, &x, &y, &z)) {
            double angle = atan2(y, -x) * 180.0 / _PI_;
            //ESP_LOGE("QMC5883L", "qmc5883l_fetch_result get angle: %f", angle);
            rotate_compass(angle);
        } else {
            ESP_LOGE("QMC5883L", "qmc5883l_fetch_result return error");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
